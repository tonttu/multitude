#include "ProcessRunner.hpp"
#include "Trace.hpp"

namespace Radiant
{
  namespace
  {
    class ProcessRunnerWin32 : public ProcessRunner
    {
      Result run(const QString & path, const QStringList & args, double, const ProcessIO&, const ProcessNotifications&) override
      {
        QString run = path + " " + args.join(" ");
        Radiant::error("ProcessRunner is not implemented on Windows. Can't run: %s", run.toUtf8().data());
        Result result;
        result.exitCode = -1;
        result.status = Status::FailedToStart;
        return result;
      }
    };
  }  // unnamed namespace

  std::unique_ptr<ProcessRunner> newProcessRunner()
  {
    return std::unique_ptr<ProcessRunner>(new ProcessRunnerWin32());
  }
}
