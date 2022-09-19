/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "ScreenDetectorQt.hpp"

#include <QApplication>
#include <QDesktopWidget>

namespace Luminous
{
  void ScreenDetectorQt::detect(QList<ScreenInfo> & result)
  {
    QDesktopWidget * desktop = QApplication::desktop();
    int screens = desktop ? desktop->screenCount() : 0;
    for (int screen = 0; screen < screens; ++screen) {
      ScreenInfo info;
      info.setGpu("default");
      info.setConnection(QString("screen%1").arg(screen));
      info.setNumId(screen);
      info.setGeometry(desktop->screenGeometry(screen));
      result.push_back(info);
    }
  }
} // namespace Luminous

