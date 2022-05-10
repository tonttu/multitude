#include "ProcessRunner.hpp"
#include "Trace.hpp"
#include "Sleep.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <cassert>
#include <poll.h>
#include <memory>
#include <map>

namespace Radiant
{
  namespace
  {
    struct PipeHolder
    {
      int fds[2] = { -1, -1 };

      ~PipeHolder()
      {
        closeFd(fds[0]);
        closeFd(fds[1]);
      }

      int& operator[](int index)
      {
        assert(index == 0 || index == 1);
        return fds[index];
      }

      static bool closeFd(int &fd)
      {
        if(fd == -1) {
          return true;
        }
        int res = 0;
        do {
          res = close(fd);
        } while(res == -1 && errno == EINTR);
        if(res == -1) {
          Radiant::error("ProcessRunner # Failed to close pipe. File descriptor: %d. Error: %s",
                         fd, strerror(errno));
          return false;
        }
        fd = -1;
        return true;
      }
    };

    int dupLoop(int oldfd, int newfd)
    {
      int res = 0;
      do {
        res = ::dup2(oldfd, newfd);
      } while(res == -1 && (errno == EINTR || errno == EBUSY));
      return res;
    }

    int openLoop(const char *path, int flags, int mode = 0)
    {
      int res = 0;
      do {
        res = ::open(path, flags, mode);
      } while(res == -1 && errno == EINTR);
      return res;
    }

    int readLoop(int fd, QByteArray &output)
    {
      char buf[1024];
      int initialSize = output.size();
      while(true) {
        ssize_t bytes = ::read(fd, buf, 1024);
        if(bytes == -1 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
          break;
        }
        if(bytes == -1 && errno == EINTR) {
          continue;
        }
        if(bytes == -1) {
          Radiant::error("ProcessRunner # error reading from redirect pipe: %s",
                         strerror(errno));
          return -1;
        }
        if(bytes == 0) {
          // closed - got eof
          break;
        }
        output.append(buf, bytes);
      }
      return output.size() - initialSize;
    }

    pid_t waitLoop(pid_t pid, int *status, int flags = 0)
    {
      pid_t res = 0;
      do {
        res = ::waitpid(pid, status, flags);
      } while(res == -1 && errno == EINTR);
      return res;
    }

    ProcessRunner::Result computeExitStatus(int status)
    {
      bool exited = WIFEXITED(status);
      if(exited) {
        return { ProcessRunner::Status::Success, WEXITSTATUS(status) };
      } else {
        return { ProcessRunner::Status::Error, -1 };
      }
    }

    class PipeHandler
    {
    public:
      PipeHandler(const ProcessOutputHandler &handler, QByteArray & buffer)
        : m_handler(handler),
          m_buffer(&buffer),
          m_lastSize(buffer.size()) { }

      PipeHandler(PipeHandler &&other) noexcept
        : m_handler(other.m_handler),
          m_buffer(other.m_buffer),
          m_lastSize(other.m_lastSize) { }

      PipeHandler& operator=(PipeHandler &&other) noexcept
      {
        if(&other == this) {
          return *this;
        }
        m_handler = std::move(other.m_handler);
        m_buffer = other.m_buffer;
        m_lastSize = other.m_lastSize;
        return *this;
      }

      void haveNewData()
      {
        if(m_handler && buffer().size() > m_lastSize) {
          m_handler(buffer(), buffer().size() - m_lastSize);
        }
        m_lastSize = buffer().size();
      }

      void signalEnd()
      {
        if(buffer().size() > m_lastSize) {
          haveNewData();
        }
        if(m_handler) {
          m_handler(buffer(), 0);
        }
      }

    private:
      QByteArray & buffer() { return * m_buffer; }

      ProcessOutputHandler m_handler;
      QByteArray * m_buffer;
      int m_lastSize;
    };

    struct PipeData
    {
      QByteArray & output;
      PipeHandler handler;

      PipeData(QByteArray & buffer, const ProcessOutputHandler & handler)
        : output(buffer), handler(handler, buffer) { }
    };

    class Pipes
    {
    public:
      std::vector<pollfd> pollfds;
      typedef std::map<int, PipeData>::iterator iterator;

      void add(int fd,
               QByteArray *buffer,
               const ProcessOutputHandler &handler)
      {
        assert(buffer != nullptr);
        pollfds.push_back({ fd, POLLIN, 0 });
        if(pipeData.find(fd) != pipeData.end()) {
          assert(false);
          Radiant::error("ProcessRunner # have two pipes with the same file descriptor");
        }
        pipeData.insert(std::make_pair(fd, PipeData(*buffer, handler)));
      }

      PipeData & data(int fd)
      {
        auto it = pipeData.find(fd);
        assert(it != pipeData.end());
        return it->second;
      }

      void stopPolling(int i)
      {
        assert(i >= 0 && i < static_cast<int>(pollfds.size()));
        pollfds.erase(pollfds.begin() + i);
      }

      iterator begin() { return pipeData.begin(); }
      iterator end() { return pipeData.end(); }

      size_t countAllPipes() const { return pipeData.size(); }
      size_t countPollPipes() const { return pollfds.size(); }

    private:
      std::map<int, PipeData> pipeData;
    };

    class ProcessRunnerPosix : public ProcessRunner
    {
    public:
      Result run(const QString & path,
                 const QStringList & arguments,
                 double timeoutSeconds,
                 const ProcessIO & io,
                 const ProcessNotifications & notif);

      void execChild(const QString & path,
                     const QStringList & arguments,
                     const QProcessEnvironment & env,
                     PipeHolder & outPipe,
                     PipeHolder & errPipe,
                     PipeHolder & execErrorPipe,
                     const QString & inputRedirect);
    };

    void flushPipes(Pipes & pipes)
    {
      for(auto & pair : pipes) {
        int fd = pair.first;
        PipeData & data = pair.second;
        int res = readLoop(fd, data.output);
        if(res > 0) {
          data.handler.haveNewData();
        } else if(res < 0) {
          assert(false);
          Radiant::error("ProcessRunner # failed to flush redirect pipes: %s",
                         strerror(errno));
        }
      }
    }

    void flushPipesAndSignalEnd(Pipes & pipes)
    {
      flushPipes(pipes);
      for(auto & pair : pipes) {
        // call one last time with 0 new bytes to signal end of output
        pair.second.handler.signalEnd();
      }
    }

    // returns true if exec failed
    bool pollPipes(Pipes & pipes, int execErrorPipeFd, pid_t pid)
    {
      int pollRes = TEMP_FAILURE_RETRY(poll(pipes.pollfds.data(), pipes.countPollPipes(), 10));
      if(pollRes == -1 && errno != EINTR) {
        Radiant::error("ProcessRunner # Failed to poll for output: %s",
                       strerror(errno));
        assert(false);  // TODO - return? recover? ignore?
      }
      if(pollRes > 0) {
        // read available data
        for(size_t i = 0; i < pipes.countPollPipes(); ++i) {
          int revents = pipes.pollfds[i].revents;
          int fd = pipes.pollfds[i].fd;

          auto readPipeData = [&] {
            QByteArray & output = pipes.data(fd).output;
            int res = readLoop(fd, output);
            assert(res != -1);  // TODO - something better than crashing
            if(res == -1) {
              Radiant::error("ProcessRunner # Failed to read from redirect pipe: %s",
                             strerror(errno));
            }
            pipes.data(fd).handler.haveNewData();
          };

          // handle pipe errors
          if(revents & POLLERR || revents & POLLNVAL || revents & POLLHUP) {
            if(revents & POLLHUP) {
              // Remote end hang up. Flush the pipe.
              readPipeData();
            } else {
              Radiant::error("ProcessRunner # Failed to poll pipe. Revents is %d", revents);
            }
            pipes.stopPolling(i);
            --i;
            continue;
          }

          // read from the pipe and call output handler
          if(revents != 0) {
            readPipeData();
          }
        }

        // handle exec error pipe separately
        for(size_t i = 0; i < pipes.countPollPipes(); ++i) {
          int revents = pipes.pollfds[i].revents;
          int fd = pipes.pollfds[i].fd;
          if(revents != 0 && fd == execErrorPipeFd) {
            // Process failed to exec and will die soon or is already dead.
            // Wait for it to end and return.
            //
            // However, be careful not to do blocking wait since that can
            // deadlock. It is possible for the child to be blocking on writing
            // to a pipe, waiting for the parent to read, while the parent is
            // blocked waiting for the child to die.
            //
            // When the child calls exit, the pipes are flushed so it does
            // a blocking write then.
            int status = 0;
            pid_t res = 0;
            do {
              flushPipes(pipes);
              res = ::waitpid(pid, &status, WNOHANG);
              Radiant::Sleep::sleepMs(1);
            } while(res == 0 || (res == -1 && errno == EINTR));
            // TODO - handle res errors somehow?
            assert(res == pid);
            // might not have had the whole error message in the pipe previously
            flushPipesAndSignalEnd(pipes);
            return true;
          }
        }
      }
      return false;
    }

    bool isStderrToStdout(const ProcessIO &io)
    {
      return io.stdoutRedirect.type() == io.stderrRedirect.type()
          && io.stdoutRedirect.type() != OutputRedirect::Type::None
          && io.stdoutRedirect.file() == io.stderrRedirect.file()
          && io.stdoutRedirect.buffer() == io.stderrRedirect.buffer();
    }

    ProcessRunner::Result ProcessRunnerPosix::run(const QString & path,
                                                  const QStringList & arguments,
                                                  double timeoutSeconds,
                                                  const ProcessIO & io,
                                                  const ProcessNotifications & notifs)
    {
      PipeHolder outPipe;
      PipeHolder errPipe;
      PipeHolder execErrorPipe;
      int res = pipe(execErrorPipe.fds);
      if(res == -1) {
        Radiant::error("ProcessRunner # Failed to open execErrorPipe: %s",
                       strerror(errno));
        return { ProcessRunner::Status::FailedToStart, -1 };
      }

      bool errToOut = isStderrToStdout(io);
      int outFileFlags = O_WRONLY | O_CREAT;
      if(!io.stdoutRedirect.append() || (errToOut && !io.stderrRedirect.append())) {
        outFileFlags |= O_TRUNC;
      }
      // output pipe
      if(io.stdoutRedirect.type() == OutputRedirect::Type::File) {
        const char *fname = io.stdoutRedirect.file().toUtf8().data();
        outPipe.fds[1] = openLoop(fname, outFileFlags, S_IRUSR | S_IWUSR);
        if(outPipe.fds[1] == -1) {
          Radiant::error("ProcessRunner # Failed to open file %s for stdout redirection: %s",
                         fname, strerror(errno));
          return { ProcessRunner::Status::FailedToStart, -1 };
        }
      } else if(io.stdoutRedirect.type() == OutputRedirect::Type::Buffer) {
        int res = pipe(outPipe.fds);
        if(res == -1) {
          Radiant::error("ProcessRunner # Failed to open pipe for stdout redirection: %s",
                         strerror(errno));
          return { ProcessRunner::Status::FailedToStart, -1 };
        }
      }
      // err pipe
      if(errToOut) {
        // do nothing, will pass outPipe to exec twice
      } else if(io.stderrRedirect.type() == OutputRedirect::Type::File) {
        int flags = O_WRONLY | O_CREAT | (io.stderrRedirect.append() ? O_TRUNC : 0);
        const char *fname = io.stderrRedirect.file().toUtf8().data();
        errPipe[1] = openLoop(fname, flags, S_IRUSR | S_IWUSR);
        if(errPipe[1] == -1) {
          Radiant::error("ProcessRunner # Failed to open file %s for stderr redirection: %s",
                         fname, strerror(errno));
          return { ProcessRunner::Status::FailedToStart, -1 };
        }
      } else if(io.stderrRedirect.type() == OutputRedirect::Type::Buffer) {
        int res = pipe(errPipe.fds);
        if(res == -1) {
          Radiant::error("ProcessRunner # Failed to open pipe for stderr redirection: %s",
                         strerror(errno));
          return { ProcessRunner::Status::FailedToStart, -1 };
        }
      }

      // fork
      pid_t pid = fork();
      if(pid == -1) {
        Radiant::error("ProcessRunner # Failed to fork child process: %s",
                       strerror(errno));
        return { ProcessRunner::Status::FailedToStart, -1 };
      }

      // close duplicate fds
      if(pid == 0) {
        PipeHolder::closeFd(outPipe[0]);
        PipeHolder::closeFd(errPipe[0]);
        PipeHolder::closeFd(execErrorPipe[0]);
      } else {
        PipeHolder::closeFd(outPipe[1]);
        PipeHolder::closeFd(errPipe[1]);
        PipeHolder::closeFd(execErrorPipe[1]);
      }

      if(pid == 0) {
        // child
        execChild(path,  arguments, io.environment,
                  outPipe, errToOut ? outPipe : errPipe, execErrorPipe, io.stdinRedirect);
        ::exit(-1);
      }

      // kid start notification
      if(notifs.onStart) {
        notifs.onStart(pid);
      }

      // make pipes non-blocking
      int makeNonBlocking[3] = { outPipe[0], errPipe[0], execErrorPipe[0] };
      for(size_t i = 0; i < sizeof(makeNonBlocking) / sizeof(int); ++i) {
        if(makeNonBlocking[i] == -1) {
          continue;
        }
        int flags = ::fcntl(makeNonBlocking[i], F_GETFL);
        int res = ::fcntl(makeNonBlocking[i], F_SETFL, flags | O_NONBLOCK);
        assert(res == 0);
        if(res != 0) {
          Radiant::error("ProcessRunner # Failed to make redirection pipes non-blocking: %s",
                         strerror(errno));
        }
      }

      // Prepare data structures for polling the pipes.
      // Pipes have a target QByteArray for output and a handler
      // that might call some notifications.
      Pipes pipes;
      if(outPipe[0] != -1) {
        assert(io.stdoutRedirect.buffer());
        pipes.add(outPipe[0], io.stdoutRedirect.buffer(), notifs.onOutput);
      }
      if(errPipe[0] != -1) {
        assert(io.stderrRedirect.buffer());
        pipes.add(errPipe[0], io.stderrRedirect.buffer(), notifs.onError);
      }
      QByteArray execError;
      pipes.add(execErrorPipe[0], &execError, ProcessOutputHandler());

      // Collect output and error and wait for the process to die.
      //
      // Do a busyloop instead of proper timeout for waitpid. Merge
      // reading output and polling for waitpid in a single loop.
      Radiant::Timer timer;
      while(true) {
        // maybe process is dead
        int status = 0;
        pid_t res = waitLoop(pid, &status, WNOHANG);
        if(res == pid) {
          // done, read all data from pipes and return
          flushPipesAndSignalEnd(pipes);
          // We might have received the execError on flushPipes after the
          // previous time we called pollPipes
          if (!execError.isEmpty()) {
            Radiant::error("ProcessRunner # Got an error from the child process while trying to run '%s': %s",
                           path.toUtf8().data(),
                           execError.data());
            return { ProcessRunner::Status::FailedToStart, -1 };
          }
          return computeExitStatus(status);
        } else if(res == -1) {
          Radiant::error("ProcessRunner # waitpid failed: %s", strerror(errno));
        } else if(res != 0) {
          Radiant::error("ProcessRunner # got unexpected waitpid result: %d", res);
        }
        assert(res != -1);

        // poll output pipes. Even if we're not doing output redirection there
        // is still the execStartPipe. However, if we get a POLLHUP we will remove
        // the item from the set of fds, so might need to sleep instead of poll
        if(pipes.countPollPipes() > 0) {
          bool execFailed = pollPipes(pipes, execErrorPipe[0], pid);
          if(execFailed) {
            flushPipesAndSignalEnd(pipes);
            Radiant::error("ProcessRunner # Got an error from the child process while trying to run '%s': %s",
                           path.toUtf8().data(),
                           execError.data());
            return { ProcessRunner::Status::FailedToStart, -1 };
          }
        } else {
          // still need to sleep else we would be using too much CPU time
          Radiant::Sleep::sleepMs(1);
        }

        // check for timeout
        if(timer.time() > timeoutSeconds) {
          // timedout. Kill process and return
          int killRes = kill(pid, SIGTERM);
          bool timedout = true;
          if(killRes == -1 && errno == ESRCH) {
            // process is already dead, ignore
            timedout = false;
            Radiant::warning("ProcessRunner # got ESRCH when trying to kill timedout process. "
                             "Assuming process ended in time.");
          } else if(killRes == -1) {
            Radiant::error("ProcessRunner # failed to send SIGINT to timedout process: %s",
                           strerror(errno));
            assert(false);
          }
          int status = 0;
          pid_t pidRes = waitLoop(pid, &status);
          (void)pidRes;
          assert(pid == pidRes);
          flushPipesAndSignalEnd(pipes);
          if(timedout) {
            return { ProcessRunner::Status::Timedout, -1 };
          } else {
            return computeExitStatus(status);
          }
        }
      }
    }

    void reportExecFailureAndExit(int execErrPipe, const QString & error)
    {
      const QByteArray & data = error.toUtf8().data();
      auto ret = ::write(execErrPipe, data.data(), data.size());
      // Need this else we're running cleanup from the original process and we don't want to
      // do that since it might block or do any other random things.
      execlp("false", "false", static_cast<char*>(nullptr));
      QByteArray msg = QString("Failed to exec 'false'. Aborting").toUtf8();
      ret = ::write(execErrPipe, msg.data(), msg.size());
      (void)ret;
      ::abort();
    }

    void ProcessRunnerPosix::execChild(const QString & path, const QStringList & arguments,
                                       const QProcessEnvironment & qenv,
                                       PipeHolder & outPipe,
                                       PipeHolder & errPipe,
                                       PipeHolder & execErrorPipe,
                                       const QString & inputRedirect)
    {
      // we might be handling SIGPIPE
      signal(SIGPIPE, SIG_DFL);
      // TODO - sigprocmask? or pthread equivalent.
      // TODO - other signals?

      char** argv = new char*[arguments.size() + 2];
      argv[0] = ::strdup(path.toUtf8().data());
      for(int i = 0; i < arguments.size(); ++i) {
        argv[i + 1] = ::strdup(arguments[i].toUtf8().data());
      }
      argv[arguments.size() + 1] = 0;

      char** env = new char*[qenv.keys().size() + 1];
      int i = 0;
      for(const QString & key : qenv.keys()) {
        QString s = key + "=" + qenv.value(key);
        env[i++] = ::strdup(s.toUtf8().data());
      }
      env[qenv.keys().size()] = 0;

      // out redirect
      if(outPipe[1] != -1) {
        int res = dupLoop(outPipe[1], STDOUT_FILENO);
        if(res == -1) {
          const char * err = strerror(errno);
          reportExecFailureAndExit(execErrorPipe[1], QString("stdout dup failure: %1").arg(err));
        }
      }
      // err redirect
      if(errPipe[1] != -1) {
        int res = dupLoop(errPipe[1], STDERR_FILENO);
        if(res == -1) {
          const char * err = strerror(errno);
          reportExecFailureAndExit(execErrorPipe[1], QString("stderr dup failure: %1").arg(err));
        }
      }
      // in redirect
      if(!inputRedirect.isEmpty()) {
        int fd = openLoop(inputRedirect.toUtf8().data(), O_RDONLY);
        if(fd == -1) {
          const char * err = strerror(errno);
          QString msg = QString("failed to open '%1' for stdin redirection: %2").arg(inputRedirect, err);
          reportExecFailureAndExit(execErrorPipe[1], msg);
        }
        int res = dupLoop(fd, STDIN_FILENO);
        if(res == -1) {
          const char * err = strerror(errno);
          reportExecFailureAndExit(execErrorPipe[1], QString("stdin dup failure: %1").arg(err));
        }
      }
      execvpe(path.toUtf8().data(), argv, env);
      const char * err = strerror(errno);
      reportExecFailureAndExit(execErrorPipe[1], QString("exec failed: %1").arg(err));
    }
  }  // unnamed namespace

  std::unique_ptr<ProcessRunner> newProcessRunner()
  {
    return std::unique_ptr<ProcessRunner>(new ProcessRunnerPosix());
  }
}
