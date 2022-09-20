/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "TemporaryDir.hpp"

#include <QCoreApplication>
#include <QDir>

#include <atomic>
#include <cassert>

#include "Trace.hpp"

namespace Radiant
{
  static std::atomic<int> s_counter{0};

  static QString createTemporaryDir()
  {
    const QString base = QString("%3/cs-temp-%1-%2").arg(QCoreApplication::applicationPid()).
        arg(s_counter++).arg(QDir::tempPath());
    QString test = base;
    for (int i = 0; QFile::exists(test) || !QDir().mkpath(test); ++i) {
      test = base + QString("-%1").arg(i);
    }
    return test;
  }

  TemporaryDir::TemporaryDir()
    : m_path(createTemporaryDir())
  {
  }

  TemporaryDir::~TemporaryDir()
  {
    QDir dir(m_path);

    // This shouldn't happen, but since we are deleting files recursively,
    // just be extra careful.
    if (m_path.isEmpty() || !dir.isAbsolute()) {
      assert(false && "~TemporaryDir");
      return;
    }

    if (!dir.removeRecursively())
      Radiant::error("Radiant::TemporaryDir # Failed to remove '%s'", m_path.toUtf8().data());
  }
} // namespace Radiant
