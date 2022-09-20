/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

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
