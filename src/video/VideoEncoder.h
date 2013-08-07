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

/**
 * \file VideoEncoder.h
 * \brief Contains the definition of the Ovito::VideoEncoder class.
 */

#ifndef __OVITO_VIDEO_ENCODER_H
#define __OVITO_VIDEO_ENCODER_H

#include <base/Base.h>

#ifdef OVITO_VIDEO_LIBRARY
#  define OVITO_VIDEO_EXPORT Q_DECL_EXPORT
#else
#  define OVITO_VIDEO_EXPORT Q_DECL_IMPORT
#endif

extern "C" {
	struct AVFormatContext;
	struct AVOutputFormat;
	struct AVCodec;
	struct AVStream;
	struct AVCodecContext;
	struct AVFrame;
	struct SwsContext;
};

namespace Ovito {

/**
 * \brief Wrapper class for the FFmpeg video encoding library.
 */
class OVITO_VIDEO_EXPORT VideoEncoder : public QObject
{
public:

	/**
	 * Describes an output format supported by the video encoding engine.
	 */
	class Format {
	public:
		QByteArray name;
		QString longName;
		QStringList extensions;

		AVOutputFormat* avformat;
	};

public:

	/// Constructor.
	VideoEncoder(QObject* parent = nullptr);

	/// Destructor.
	virtual ~VideoEncoder() { closeFile(); }

	/// Opens a video file for writing.
	void openFile(const QString& filename, int width, int height, int fps, VideoEncoder::Format* format = nullptr, int bitrate = 0);

	/// Writes a single frame into the video file.
	void writeFrame(const QImage& image);

	/// This closes the written video file.
	void closeFile();

	/// Returns the list of supported output formats.
	static QList<Format> supportedFormats();

private:

	/// Initializes libavcodec, and register all codecs and formats.
	static void initCodecs();

	/// Returns the error string for the given error code.
	static QString errorMessage(int errorCode);

	std::shared_ptr<AVFormatContext> _formatContext;
	std::unique_ptr<quint8[]> _pictureBuf;
	std::vector<quint8> _outputBuf;
	std::shared_ptr<AVFrame> _frame;
	AVStream* _videoStream;
	AVCodecContext* _codecContext;
	SwsContext* _imgConvertCtx;
	bool _isOpen;

	/// The list of supported video formats.
	static QList<Format> _supportedFormats;

	Q_OBJECT
};

};	// End of namespace

#endif // __OVITO_VIDEO_ENCODER_H