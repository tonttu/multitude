#ifndef RADIANT_PROCESSRUNNER_HPP
#define RADIANT_PROCESSRUNNER_HPP


#include "Export.hpp"

#ifndef RADIANT_MOBILE

#include <QByteArray>
#include <QStringList>
#include <QString>
#include <QProcessEnvironment>
#include <memory>
#include <functional>
#include <sys/types.h>

namespace Radiant
{
  /// Carries information about output redirection. Can redirect to files or ByteArrays.
  class OutputRedirect
  {
  public:
    enum class Type
    {
      None,
      Buffer,
      File
    };

    OutputRedirect() { }
    OutputRedirect(QByteArray * buffer) : m_output(buffer) { }

    /// If we just have a simple overload for QString then it's very
    /// easy to accidentally pass a QByteArray reference instead of a
    /// pointer which gets converted to a QString.
    ///
    /// So use this instead to redirect to files
    static OutputRedirect toFile(const QString & file)
    {
      return OutputRedirect(file);
    }

    OutputRedirect(const OutputRedirect &other)
      : m_output(other.m_output),
        m_file(other.m_file),
        m_append(other.m_append) { }

    OutputRedirect& operator=(const OutputRedirect &other)
    {
      if(&other == this) {
        return *this;
      }
      m_output = other.m_output;
      m_file = other.m_file;
      m_append = other.m_append;
      return *this;
    }

    void setAppend(bool append) { m_append = append; }

    QByteArray * buffer() const { return m_output; }
    const QString & file() const { return m_file; }
    bool append() const { return m_append; }

    Type type() const
    {
      if(!m_file.isEmpty()) {
        return Type::File;
      }
      if(m_output != nullptr) {
        return Type::Buffer;
      }
      return Type::None;
    }

  private:
    // call OutputRedirect::toFile instead of this and don't make it public.
    OutputRedirect(const QString & file) : m_file(file) { }

    QByteArray * m_output = nullptr;
    QString m_file;
    bool m_append = true;
  };

  struct ProcessIO
  {
    OutputRedirect stdoutRedirect;
    OutputRedirect stderrRedirect;
    QString stdinRedirect;
    QProcessEnvironment environment;

    /// stdout and stderr redirect can point to the same buffer or file.
    /// This is sometimes useful when interleaving of errors with regular output is required
    /// in order to see the proper sequence of events.
    ProcessIO(const OutputRedirect &stdoutRedirect = OutputRedirect(),
              const OutputRedirect &stderrRedirect = OutputRedirect(),
              QString stdinRedirectFile = QString(),
              QProcessEnvironment env = QProcessEnvironment::systemEnvironment())
      : stdoutRedirect(stdoutRedirect),
        stderrRedirect(stderrRedirect),
        stdinRedirect(stdinRedirectFile),
        environment(env) { }
  };

#ifdef RADIANT_WINDOWS
  typedef uint32_t pid_t;
#endif

  /// Called after fork but maybe before exec
  typedef std::function<void(pid_t pid)> ProcessStartHandler;

  /// Gets a reference to the full output as well as the number of bytes written since
  /// the last notification. Will be called one final time when the process has exited
  /// with 0 countNewBytes
  typedef std::function<void(const QByteArray &fullOutput, int countNewBytes)> ProcessOutputHandler;

  struct ProcessNotifications
  {
    /// This is called after the child is forked but the child may or may not have called
    /// exec at this time. So if for example you are sending signals to the child
    /// from onStart they might get processed from the pre-exec process and do unexpected
    /// things.
    ///
    /// This problem occurs because we can't tell if exec has finished running or not in
    /// the child. At least, not for the general case when we don't know what binary is
    /// starting.
    ///
    /// Waiting for some output might work in some cases but not always.
    ///
    /// So probably the only general approach would be to store the pid in onStart and try
    /// to communicate with the child repeatedly on a timer.
    ///
    /// Will be called from the same thread that calls ProcessRunner::run
    ProcessStartHandler onStart;

    /// Called whenever there is output to stdout. Only works if the output is collected to
    /// a byte array - if OutputRedirect::type() returns Buffer. Will be called one final
    /// time when the process has exited with 0 countNewBytes
    ///
    /// TODO - make this work even if output is not already redirected to a buffer
    ///
    /// Will be called from the same thread that calls ProcessRunner::run
    ProcessOutputHandler onOutput;

    /// Called whenever there is output to stderr. Only works if the output is collected to
    /// a byte array - if OutputRedirect::type() returns Buffer. Additionally, if stderr
    /// is redirected to stdout (by specifying the same buffer or filename), then only
    /// onOutput notifications will be triggered. Will be called one final time when the
    /// process has exited with 0 countNewBytes
    ///
    /// TODO - make this work even if output is not already redirected to a buffer
    ///
    /// Will be called from the same thread that calls ProcessRunner::run
    ProcessOutputHandler onError;
  };

  /// ProcessRunner interface. Will run a process synchronously. Will block until the process
  /// exits or the timeout is reached and will return a result indicating the status and
  /// exit code. Does not run the binary in a shell.
  ///
  /// You need to obtain an implementation to use it. newProcessRunner() returns the standard
  /// implementation.
  ///
  /// Example usage with redirecting stdout and stderr to the same QByteArray and timeout of
  /// 10 seconds:
  ///
  /// 	@code{.cpp}
  /// 	auto runner = newProcessRunner();
  /// 	QByteArray output;
  ///		OutputRedirect redirect(&output);
  /// 	ProcessIO io(redirect, redirect);  // both stdout and stderr to the same buffer
  /// 	QStringList args = QStringList() << "-ne" << "arg1" << "arg2\n";
  /// 	Result result = runner->run("echo", args, 10, io);
  ///		if(result.ok()) { be_happy(); }
  /// 	@endcode
  ///
  class RADIANT_API ProcessRunner
  {
  public:
    enum class Status {
      Success,  // means we could run the process. The process itself might have
                // failed whatever it was trying to do. Exit code can be non-zero.
      FailedToStart,  // Could not find file or don't have right to run it.
      Timedout,
      Error  // Process crashed or could not read / write or an unexpected error occured.
    };

    struct Result
    {
      Status status;
      int exitCode;

      bool ok() const { return status == Status::Success && exitCode == 0; }
      QString stringStatus();
      QString toString();
    };

    /// Runs the given binary in a subprocess. Blocks until the process exits or the timeout
    /// is reached. When the timeout is reached, the process is killed. On posix, using the
    /// standard implementation, it is sent a SIGTERM signal and then the runner blocks until
    /// the process exits.
    ///
    /// Does not run the binary in a shell, so it will not do output redirection with '>',
    /// parameter glob expansion, piping and so on.
    ///
    /// Some simple io redirection is supported via the 'io' parameter.
    ///
    /// @param path path to binary
    /// @param arguments command line arguments for the binary. Does not split on spaces and does
    /// not do glob expansion or output redirection.
    /// @param timeoutSeconds timeout until the process is killed
    /// @param io configuration for input and output redirection as well as environment variables
    /// passed to the subprocess
    /// @param notifications optional callbacks that are fired when certain events occur. Can
    /// be used to get output from the subprocess as soon as it happens instead of when it ends.
    virtual Result run(const QString &path,
                       const QStringList &arguments,
                       double timeoutSeconds,
                       const ProcessIO &io = ProcessIO(),
                       const ProcessNotifications &notifications = ProcessNotifications()) = 0;
    virtual ~ProcessRunner() { }
  };

  /// Returns the standard process runner implementation. Will never return null
  RADIANT_API std::unique_ptr<ProcessRunner> newProcessRunner();

  /// Normally the ProcessOutputHandler is called with every new chunk of output, which
  /// might not be a full line. This creates a wrapper that buffers input and only calls
  /// the inner worker when a full line is available and at the end of the process.
  ///
  /// The QByteArray will include the newline except at the end if there isn't one.
  /// @param lineEnd is exclusive.
  typedef std::function<void(const QByteArray &buffer, int lineStart, int lineEnd)> LineHandler;
  RADIANT_API ProcessOutputHandler lineByLineHandler(const LineHandler & lineHandler);
}

#endif

#endif // RADIANT_PROCESSRUNNER_HPP
