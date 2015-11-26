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

