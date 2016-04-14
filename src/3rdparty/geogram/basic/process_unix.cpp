/*
 *  Copyright (c) 2012-2014, Bruno Levy
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *  this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *  this list of conditions and the following disclaimer in the documentation
 *  and/or other materials provided with the distribution.
 *  * Neither the name of the ALICE Project-Team nor the names of its
 *  contributors may be used to endorse or promote products derived from this
 *  software without specific prior written permission.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  If you modify this software, you should include a notice giving the
 *  name of the person performing the modification, the date of modification,
 *  and the reason for such modification.
 *
 *  Contact: Bruno Levy
 *
 *     Bruno.Levy@inria.fr
 *     http://www.loria.fr/~levy
 *
 *     ALICE Project
 *     LORIA, INRIA Lorraine, 
 *     Campus Scientifique, BP 239
 *     54506 VANDOEUVRE LES NANCY CEDEX 
 *     FRANCE
 *
 */

#include <geogram/basic/common.h>

#ifdef GEO_OS_UNIX

#include <geogram/basic/process.h>
#include <geogram/basic/atomics.h>
#include <geogram/basic/logger.h>
#include <geogram/basic/progress.h>
#include <geogram/basic/line_stream.h>

#include <sstream>
#include <pthread.h>
#include <unistd.h>
#include <limits.h>
#include <fenv.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <new>

#define GEO_USE_PTHREAD_MANAGER

namespace {

    using namespace GEO;

#ifdef GEO_OS_ANDROID

    /**
     * \brief Get the number of cores under Android
     * \retval the number of cores if the request succeeds
     * \retval -1 otherwise
     * \internal
     * sysconf(_SC_NPROCESSORS_ONLN) and sysconf(_SC_NPROCESSORS_CONF)
     * is bugged under Android, see:
     * https://code.google.com/p/android/issues/detail?id=26490
     */
    int android_get_number_of_cores() {
        FILE* fp;
        int res, i = -1, j = -1;
        /* open file */
        fp = fopen("/sys/devices/system/cpu/present", "r");
        if(fp == 0) {
            return -1; /* failure */
        }

        /* read and interpret line */
        res = fscanf(fp, "%d-%d", &i, &j);

        /* close file */
        fclose(fp);

        /* interpret result */
        if(res == 1 && i == 0) {
            /* single-core */
            return 1;
        }

        if(res == 2 && i == 0) {
            /* 2+ cores */
            return j + 1;
        }

        return -1; /* failure */
    }

#endif

#ifdef GEO_USE_PTHREAD_MANAGER

    /**
     * \brief POSIX Thread ThreadManager
     * \details
     * PThreadManager is an implementation of ThreadManager that uses POSIX
     * threads for running concurrent threads and control critical sections.
     */
    class GEOGRAM_API PThreadManager : public ThreadManager {
    public:
        /**
         * \brief Creates and initializes the POSIX ThreadManager
         */
        PThreadManager() {
            // For now, I do not trust pthread_mutex_xxx functions
            // under Android, so I'm using assembly functions
            // from atomics (I'm sure they got the right memory
            // barriers for SMP).
#ifdef GEO_OS_ANDROID
            mutex_ = 0;
#else
            pthread_mutex_init(&mutex_, 0);
#endif
            pthread_attr_init(&attr_);
            pthread_attr_setdetachstate(&attr_, PTHREAD_CREATE_JOINABLE);
        }

        /** \copydoc GEO::ThreadManager::maximum_concurrent_threads() */
        virtual index_t maximum_concurrent_threads() {
            return Process::number_of_cores();
        }

        /** \copydoc GEO::ThreadManager::enter_critical_section() */
        virtual void enter_critical_section() {
#ifdef GEO_OS_ANDROID
            lock_mutex_arm(&mutex_);
#else
            pthread_mutex_lock(&mutex_);
#endif
        }

        /** \copydoc GEO::ThreadManager::leave_critical_section() */
        virtual void leave_critical_section() {
#ifdef GEO_OS_ANDROID
            unlock_mutex_arm(&mutex_);
#else
            pthread_mutex_unlock(&mutex_);
#endif
        }

    protected:
        /** \brief PThreadManager destructor */
        virtual ~PThreadManager() {
            pthread_attr_destroy(&attr_);
#ifndef GEO_OS_ANDROID
            pthread_mutex_destroy(&mutex_);
#endif
        }

        /**
         * \brief Pthread_create callback for running a thread
         * \details This function is passed a void pointer \p thread to a
         * Thread and invokes the Thread function run().
         * \param[in] thread_in void pointer to the Thread to be executed.
         * \return always null pointer.
         * \see Thread::run()
         */
        static void* run_thread(void* thread_in) {
            Thread* thread = reinterpret_cast<Thread*>(thread_in);
            // Sets the thread-local-storage instance pointer, so
            // that Thread::current() can retreive it.
            set_current_thread(thread);
            thread->run();
            return nil;
        }

        /** \copydoc GEO::ThreadManager::run_concurrent_threads() */
        virtual void run_concurrent_threads(
            ThreadGroup& threads, index_t max_threads
        ) {
            // TODO: take max_threads into account
            geo_argused(max_threads);

            thread_impl_.resize(threads.size());
            for(index_t i = 0; i < threads.size(); i++) {
                Thread* T = threads[i];
                set_thread_id(T,i);
                pthread_create(
                    &thread_impl_[i], &attr_, &run_thread, T
                );
            }
            for(index_t i = 0; i < threads.size(); ++i) {
                pthread_join(thread_impl_[i], nil);
            }

        }

    private:
#ifdef GEO_OS_ANDROID
        arm_mutex_t mutex_;
#else
        pthread_mutex_t mutex_;
#endif
        pthread_attr_t attr_;
        std::vector<pthread_t> thread_impl_;
    };

#endif

    /**
     * \brief Abnormal termination handler
     * \details If \p message is
     * non null, the following message is printed before exiting.
     * <em>Abnormal program termination: message</em>
     * \param[in] message optional message to print
     */
    void abnormal_program_termination(const char* message = nil) {
        if(message != nil) {
            // Do not use Logger here!
            std::cout
                << "Abnormal program termination: "
                << message << std::endl;
        }
        exit(1);
    }

    /**
     * \brief Signal handler
     * \details The handler exits the application
     * \param[in] signal signal number
     */
    void signal_handler(int signal) {
        const char* sigstr = strsignal(signal);
        std::ostringstream os;
        os << "received signal " << signal << " (" << sigstr << ")";
        abnormal_program_termination(os.str().c_str());
    }

    /**
     * \brief Floating point error handler
     * \details The handler exits the application
     * \param[in] signal signal number
     * \param[in] si signal information structure
     * \param[in] data additional data (unused)
     */
    void fpe_signal_handler(int signal, siginfo_t* si, void* data) {
        geo_argused(signal);
        geo_argused(data);
        const char* error;
        switch(si->si_code) {
            case FPE_INTDIV:
                error = "integer divide by zero";
                break;
            case FPE_INTOVF:
                error = "integer overflow";
                break;
            case FPE_FLTDIV:
                error = "floating point divide by zero";
                break;
            case FPE_FLTOVF:
                error = "floating point overflow";
                break;
            case FPE_FLTUND:
                error = "floating point underflow";
                break;
            case FPE_FLTRES:
                error = "floating point inexact result";
                break;
            case FPE_FLTINV:
                error = "floating point invalid operation";
                break;
            case FPE_FLTSUB:
                error = "subscript out of range";
                break;
            default:
                error = "unknown";
                break;
        }

        std::ostringstream os;
        os << "floating point exception detected: " << error;
        abnormal_program_termination(os.str().c_str());
    }

    /**
     * \brief Interrupt signal handler
     * \details The handler cancels the current task if any or exits the
     * program.
     */
    void sigint_handler(int) {
        if(Progress::current_task() != nil) {
            Progress::cancel();
        } else {
            exit(1);
        }
    }

    /**
     * Catches unexpected C++ exceptions
     */
    void unexpected_handler() {
        abnormal_program_termination("function unexpected() was called");
    }

    /**
     * Catches uncaught C++ exceptions
     */
    void terminate_handler() {
        abnormal_program_termination("function terminate() was called");
    }

    /**
     * Catches allocation errors
     */
    void memory_exhausted_handler() {
        abnormal_program_termination("memory exhausted");
    }
}

/****************************************************************************/

namespace GEO {

    namespace Process {

        bool os_init_threads() {
#ifdef GEO_USE_PTHREAD_MANAGER
            Logger::out("Process")
                << "Using posix threads"
                << std::endl;
            set_thread_manager(new PThreadManager);
            return true;
#else
            return false;
#endif
        }

        void os_brute_force_kill() {
            kill(getpid(), SIGKILL);
        }

        index_t os_number_of_cores() {
#ifdef GEO_OS_ANDROID
            int nb_cores = android_get_number_of_cores();
            geo_assert(nb_cores > 0);
            return index_t(nb_cores);
#else
            return index_t(sysconf(_SC_NPROCESSORS_ONLN));
#endif
        }

        size_t os_used_memory() {
            // The following method seems to be more 
            // reliable than  getrusage() under Linux.
            // It works for both Linux and Android.
            size_t result = 0;
            LineInput in("/proc/self/status");
            while(!in.eof() && in.get_line()) {
                in.get_fields();
                if(in.field_matches(0,"VmSize:")) {
                        result = size_t(in.field_as_uint(1)) * size_t(1024);
                    break;
                }
            }
            return result;
        }

        size_t os_max_used_memory() {
            // The following method seems to be more 
            // reliable than  getrusage() under Linux.
            // It works for both Linux and Android.
            size_t result = 0;
            LineInput in("/proc/self/status");
            while(!in.eof() && in.get_line()) {
                in.get_fields();
                if(in.field_matches(0,"VmPeak:")) {
                    result = size_t(in.field_as_uint(1)) * size_t(1024);
                    break;
                }
            }
            return result;
        }

        bool os_enable_FPE(bool flag) {
            int excepts = 0
                // | FE_INEXACT   // inexact result
                   | FE_DIVBYZERO   // division by zero
                   | FE_UNDERFLOW // result not representable due to underflow
                   | FE_OVERFLOW    // result not representable due to overflow
                   | FE_INVALID     // invalid operation
                   ;
            if(flag) {
                feenableexcept(excepts);
            } else {
                fedisableexcept(excepts);
            }
            return true;
        }

        bool os_enable_cancel(bool flag) {
            if(flag) {
                signal(SIGINT, sigint_handler);
            } else {
                signal(SIGINT, SIG_DFL);
            }
            return true;
        }

        /**
         * \brief Installs signal handlers
         * \details
         * On Unix, this installs handlers for the standard signals.
         * On Windows, this also installs all kind of exception handling
         * routines that prevent the application from being blocked by a bad
         * assertion, a runtime check or runtime error.
         */
        void os_install_signal_handlers() {

            // Install signal handlers
            signal(SIGSEGV, signal_handler);
            signal(SIGILL, signal_handler);
            signal(SIGBUS, signal_handler);

            // Use sigaction for SIGFPE as it provides more details 
            // about the error.
            struct sigaction sa, old_sa;
            sa.sa_flags = SA_SIGINFO;
            sa.sa_sigaction = fpe_signal_handler;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGFPE, &sa, &old_sa);

            // Install unexpected and uncaught c++ exception handlers
            std::set_unexpected(unexpected_handler);
            std::set_terminate(terminate_handler);

            // Install memory allocation handler
            std::set_new_handler(memory_exhausted_handler);
        }


        /**
         * \brief Gets the full path to the current executable.
         */
        std::string os_executable_filename() {
            char buff[PATH_MAX];
            ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
            if (len != -1) {
                buff[len] = '\0';
                return std::string(buff);
            }
            return std::string("");
        }        
        
    }
}

#else 

// Declare a dummy variable so that
// MSVC does not complain that it 
// generated an empty object file.
int dummy_process_unix_compiled = 1;

#endif

