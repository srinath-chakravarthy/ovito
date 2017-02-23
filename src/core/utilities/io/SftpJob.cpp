///////////////////////////////////////////////////////////////////////////////
// 
//  Copyright (2013) Alexander Stukowski
//
//  This file is part of OVITO (Open Visualization Tool).
//
//  OVITO is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  OVITO is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#include <core/Core.h>
#include <core/utilities/concurrent/Future.h>
#include <core/utilities/io/FileManager.h>
#include <core/app/Application.h>

#include "SftpJob.h"

namespace Ovito { OVITO_BEGIN_INLINE_NAMESPACE(Util) OVITO_BEGIN_INLINE_NAMESPACE(IO) OVITO_BEGIN_INLINE_NAMESPACE(Internal)

/// List SFTP jobs that are waiting to be executed.
QQueue<SftpJob*> SftpJob::_queuedJobs;

/// Keeps track of how many SFTP jobs are currently active.
int SftpJob::_numActiveJobs = 0;

enum { MaximumNumberOfSimulateousSftpJobs = 2 };

/******************************************************************************
* Constructor.
******************************************************************************/
SftpJob::SftpJob(const QUrl& url, const PromiseBasePtr& promise) :
		_url(url), _connection(nullptr), _promise(promise), _isActive(false)
{
	// Run all event handlers of this class in the main thread.
	moveToThread(QCoreApplication::instance()->thread());

	// Start download process in the main thread.
	QMetaObject::invokeMethod(this, "start", Qt::AutoConnection);
}

/******************************************************************************
* Opens the SSH connection.
******************************************************************************/
void SftpJob::start()
{
	if(!_isActive) {
		// Keep a counter of active jobs.
		// If there are too many jobs active simultaneously, queue them to be executed later.
		if(_numActiveJobs >= MaximumNumberOfSimulateousSftpJobs) {
			_queuedJobs.enqueue(this);
			return;
		}
		else {
			_numActiveJobs++;
			_isActive = true;
		}
	}

	// This background task started to run.
	_promise->setStarted();

	// Check if process has already been canceled.
	if(_promise->isCanceled()) {
		shutdown(false);
		return;
	}

	QSsh::SshConnectionParameters connectionParams;
	connectionParams.host = _url.host();
	connectionParams.userName = _url.userName();
	connectionParams.password = _url.password();
	if(connectionParams.userName.isEmpty() || connectionParams.password.isEmpty()) {
		QPair<QString,QString> credentials = Application::instance()->fileManager()->findCredentials(connectionParams.host);
		if(credentials.first.isEmpty() == false) {
			connectionParams.userName = credentials.first;
			connectionParams.password = credentials.second;
		}
	}
	connectionParams.port = _url.port(22);
	connectionParams.authenticationType = QSsh::SshConnectionParameters::AuthenticationTypeTryAllPasswordBasedMethods;
	connectionParams.options &= ~QSsh::SshEnableStrictConformanceChecks;
	connectionParams.timeout = 10;

	_promise->setProgressText(tr("Connecting to remote server %1").arg(_url.host()));

	// Open connection
	_connection = QSsh::acquireConnection(connectionParams);
	OVITO_CHECK_POINTER(_connection);

	// Listen for signals of the connection.
	connect(_connection, &QSsh::SshConnection::error, this, &SftpJob::onSshConnectionError);
	if(_connection->state() == QSsh::SshConnection::Connected) {
		onSshConnectionEstablished();
		return;
	}
	QObject::connect(_connection, &QSsh::SshConnection::connected, this, &SftpJob::onSshConnectionEstablished);

	// Start to connect.
	if(_connection->state() == QSsh::SshConnection::Unconnected)
		_connection->connectToHost();
}

/******************************************************************************
* Closes the SSH connection.
******************************************************************************/
void SftpJob::shutdown(bool success)
{
	if(_sftpChannel) {
		QObject::disconnect(_sftpChannel.data(), 0, this, 0);
		_sftpChannel->closeChannel();
		_sftpChannel.clear();
	}
	if(_connection) {
		QObject::disconnect(_connection, 0, this, 0);
		QSsh::releaseConnection(_connection);
		_connection = nullptr;
	}

	_promise->setFinished();

	// Update the counter of active jobs.
	if(_isActive) {
		_numActiveJobs--;
		_isActive = false;
	}

	// Schedule this object for deletion.
	deleteLater();

	// If there are now less jobs active simultaneously, execute one of the waiting jobs.
	if(_numActiveJobs < MaximumNumberOfSimulateousSftpJobs && !_queuedJobs.isEmpty()) {
		SftpJob* waitingJob = _queuedJobs.dequeue();
		if(waitingJob->_promise->isCanceled() == false) {
			waitingJob->start();
		}
		else {
			// Skip canceled jobs.
			waitingJob->_promise->setStarted();
			waitingJob->shutdown(false);
		}
	}
}

/******************************************************************************
* Handles SSH connection errors.
******************************************************************************/
void SftpJob::onSshConnectionError(QSsh::SshError error)
{
	// If authentication failed, ask the user to re-enter username/password.
	if(error == QSsh::SshAuthenticationError && !_promise->isCanceled()) {
		OVITO_ASSERT(!_sftpChannel);
		if(Application::instance()->fileManager()->askUserForCredentials(_url)) {
			// Start over with new login information.
			QObject::disconnect(_connection, 0, this, 0);
			QSsh::releaseConnection(_connection);
			_connection = nullptr;
			start();
			return;
		}
		else {
			_promise->cancel();
		}
	}
	else {
		try {
			throw Exception(tr("Cannot access URL\n\n%1\n\nSSH connection error: %2").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)).
				arg(_connection->errorString()));
		}
        catch(Exception&) {
			_promise->setException();
		}
	}
	shutdown(false);
}

/******************************************************************************
* Is called when the SSH connection has been established.
******************************************************************************/
void SftpJob::onSshConnectionEstablished()
{
	if(_promise->isCanceled()) {
		shutdown(false);
		return;
	}

	// After successful login, store login information in cache.
	QSsh::SshConnectionParameters connectionParams = _connection->connectionParameters();
	Application::instance()->fileManager()->cacheCredentials(connectionParams.host, connectionParams.userName, connectionParams.password);

	_promise->setProgressText(tr("Opening SFTP file transfer channel"));

	_sftpChannel = _connection->createSftpChannel();
	connect(_sftpChannel.data(), &QSsh::SftpChannel::initialized, this, &SftpJob::onSftpChannelInitialized);
	connect(_sftpChannel.data(), &QSsh::SftpChannel::channelError, this, &SftpJob::onSftpChannelError);
	_sftpChannel->initialize();
}

/******************************************************************************
* Is called when the SFTP channel could not be created.
******************************************************************************/
void SftpJob::onSftpChannelError(const QString& reason)
{
	_promise->setException(std::make_exception_ptr(Exception(tr("Cannot access URL\n\n%1\n\nSFTP error: %2").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)).arg(reason))));
	shutdown(false);
}

/******************************************************************************
* Closes the SSH connection.
******************************************************************************/
void SftpDownloadJob::shutdown(bool success)
{
	if(_timerId)
		killTimer(_timerId);

	if(_localFile && success)
		static_cast<Promise<QString>*>(_promise.get())->setResult(_localFile->fileName());
	else
		_localFile.reset();

	SftpJob::shutdown(success);

	Application::instance()->fileManager()->fileFetched(_url, _localFile.take());
}

/******************************************************************************
* Is called when the SFTP channel has been created.
******************************************************************************/
void SftpDownloadJob::onSftpChannelInitialized()
{
	if(_promise->isCanceled()) {
		shutdown(false);
		return;
	}

	connect(_sftpChannel.data(), &QSsh::SftpChannel::finished, this, &SftpDownloadJob::onSftpJobFinished);
	connect(_sftpChannel.data(), &QSsh::SftpChannel::fileInfoAvailable, this, &SftpDownloadJob::onFileInfoAvailable);
	try {

		// Set progress text.
		_promise->setProgressText(tr("Fetching remote file %1").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

		// Create temporary file.
		_localFile.reset(new QTemporaryFile());
		if(!_localFile->open())
			throw Exception(tr("Failed to create temporary file: %1").arg(_localFile->errorString()));
		_localFile->close();

		// Request file info.
		_sftpChannel->statFile(_url.path());

		// Start to download file.
		_downloadJob = _sftpChannel->downloadFile(_url.path(), _localFile->fileName(), QSsh::SftpOverwriteExisting);
		if(_downloadJob == QSsh::SftpInvalidJob)
			throw Exception(tr("Failed to download remote file %1.").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

		// Start timer to monitor download progress.
		_timerId = startTimer(500);
	}
    catch(Exception&) {
		_promise->setException();
		shutdown(false);
	}
}

/******************************************************************************
* Is called after the file has been downloaded.
******************************************************************************/
void SftpDownloadJob::onSftpJobFinished(QSsh::SftpJobId jobId, const QString& errorMessage) {
	if(jobId != _downloadJob)
		return;

	if(_promise->isCanceled()) {
		shutdown(false);
		return;
	}
    if(!errorMessage.isEmpty()) {
		_promise->setException(std::make_exception_ptr(Exception(tr("Cannot access URL\n\n%1\n\nSFTP error: %2")
					.arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded))
					.arg(errorMessage))));
		shutdown(false);
		return;
    }
    shutdown(true);
}

/******************************************************************************
* Is called when the file info for the requested file arrived.
******************************************************************************/
void SftpDownloadJob::onFileInfoAvailable(QSsh::SftpJobId job, const QList<QSsh::SftpFileInfo>& fileInfoList)
{
	if(fileInfoList.empty() == false ) {
		if(fileInfoList[0].sizeValid) {
			_promise->setProgressMaximum(fileInfoList[0].size / 1000);
		}
	}
}

/******************************************************************************
* Is invoked when the QObject's timer fires.
******************************************************************************/
void SftpDownloadJob::timerEvent(QTimerEvent* event)
{
	SftpJob::timerEvent(event);

	if(_localFile) {
		qint64 size = _localFile->size();
		if(size >= 0 && _promise->progressMaximum() > 0) {
			_promise->setProgressValue(size / 1000);
		}
    	if(_promise->isCanceled())
    		shutdown(false);
	}
}

/******************************************************************************
* Is called when the SFTP channel has been created.
******************************************************************************/
void SftpListDirectoryJob::onSftpChannelInitialized()
{
	if(_promise->isCanceled()) {
		shutdown(false);
		return;
	}

	connect(_sftpChannel.data(), &QSsh::SftpChannel::finished, this, &SftpListDirectoryJob::onSftpJobFinished);
	connect(_sftpChannel.data(), &QSsh::SftpChannel::fileInfoAvailable, this, &SftpListDirectoryJob::onFileInfoAvailable);
	try {
		// Set progress text.
		_promise->setProgressText(tr("Listing remote directory %1").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));

		// Request file list.
		_listingJob = _sftpChannel->listDirectory(_url.path());
		if(_listingJob == QSsh::SftpInvalidJob)
			throw Exception(tr("Failed to list contents of remote directory %1.").arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded)));
	}
    catch(Exception&) {
		_promise->setException();
		shutdown(false);
	}
}

/******************************************************************************
* Is called after the file has been downloaded.
******************************************************************************/
void SftpListDirectoryJob::onSftpJobFinished(QSsh::SftpJobId jobId, const QString& errorMessage) {
	if(jobId != _listingJob)
		return;

	if(_promise->isCanceled()) {
		shutdown(false);
		return;
	}
    if(!errorMessage.isEmpty()) {
    	_promise->setException(std::make_exception_ptr(Exception(tr("Cannot access URL\n\n%1\n\nSFTP error: %2")
					.arg(_url.toString(QUrl::RemovePassword | QUrl::PreferLocalFile | QUrl::PrettyDecoded))
					.arg(errorMessage))));
		shutdown(false);
		return;
    }

	static_cast<Promise<QStringList>*>(_promise.get())->setResult(_fileList);
    shutdown(true);
}

/******************************************************************************
* Is called when the file info for the requested file arrived.
******************************************************************************/
void SftpListDirectoryJob::onFileInfoAvailable(QSsh::SftpJobId job, const QList<QSsh::SftpFileInfo>& fileInfoList)
{
	for(const QSsh::SftpFileInfo& fileInfo : fileInfoList) {
		if(fileInfo.type == QSsh::FileTypeRegular)
			_fileList.push_back(fileInfo.name);
	}
}

OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
OVITO_END_INLINE_NAMESPACE
}	// End of namespace
