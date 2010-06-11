#include "FlashPlayer.hpp"

#include <QApplication>
#include <QDir>
#include <QDomDocument>

#include <limits>
#include <iostream>

#include <X11/extensions/Xinerama.h>

namespace
{
  QString rectToId(const QRect & rect) {
    if(rect.isValid())
      return QString("%1x%2+%3+%4").arg(rect.width()).arg(rect.height()).arg(rect.x()).arg(rect.y());
    else
      return "";
  }
}

bool Screens::update()
{
  Display * display = XOpenDisplay(NULL);
  if(!display)
    return false;

  if(XineramaIsActive(display)) {
    int nscreens = 0;
    XineramaScreenInfo * screens = XineramaQueryScreens(display, &nscreens);
    for(int i = 0; i < nscreens; ++i) {
      Screen s = {screens[i].screen_number, screens[i].x_org, screens[i].y_org,
                  screens[i].width, screens[i].height};
      m_view |= s;
      m_screens << s;
    }
  }

  if(m_view.isNull()) {
    int nscreens = XScreenCount(display);
    for(int i = 0; i < nscreens; ++i) {
      Screen s = {i, m_view.isNull() ? 0 : m_view.width(), 0,
                  XDisplayWidth(display, i), XDisplayHeight(display, i)};
      m_view |= s;
      m_screens << s;
    }
  }

  XCloseDisplay(display);
  return m_view.isValid();
}

QString Screens::id() const
{
  return rectToId(m_view);
}

struct ConfigLine
{
  bool automatic;
  QString view;
  ConfigLine() : automatic(false) {}
};

class Config
{
public:
  Config(QString dir, QString file) : m_document("flash")
  {
    QDir qdir;
    qdir.mkpath(dir);
    QFile qfile(dir + "/" + file);
    if(qfile.open(QIODevice::ReadOnly)) {
      m_document.setContent(&qfile);
    }
  }
  ConfigLine operator[](QString id) const
  {
    QDomElement root = m_document.documentElement();

    QDomNode n = root.firstChild();
    while(!n.isNull()) {
      QDomElement e = n.toElement();
      if(!e.isNull() && e.tagName() == "config" && e.attribute("id") == id) {
        ConfigLine line;
        QString a = e.attribute("automatic");
        line.automatic = a == "yes" || a == "1" || a == "true" || a == "t";
        line.view = e.text();
        return line;
      }
      n = n.nextSibling();
    }
    return ConfigLine();
  }

private:
  QDomDocument m_document;
};

int main(int argc, char * argv[])
{
  bool fullscreen = false;
  char * view = 0;
  char * src = 0;

  {
    QApplication app(argc, argv);
    Screens screens;
    screens.update();
    QString id = screens.id();
    Config config(QDir::homePath() + "/.MultiTouch", "flash.xml");
    ConfigLine line = config[id];
    if(line.automatic) {
      view = strdup(line.view.toAscii().data());
    }
  }

  if(!src) {
    if(argc != 2) {
      std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
      return 1;
    }
    src = new char[strlen(argv[1])+5];
    strcpy(src, "src=");
    strcat(src, argv[1]);
  }

  if(fullscreen && view)
    return execlp("nspluginplayer-mt", "nspluginplayer-mt", "--fullscreen", "--view", view, src, NULL);
  else if(fullscreen)
    return execlp("nspluginplayer-mt", "nspluginplayer-mt", "--fullscreen", src, NULL);
  else if(view)
    return execlp("nspluginplayer-mt", "nspluginplayer-mt", "--view", view, src, NULL);
  else
    return execlp("nspluginplayer-mt", "nspluginplayer-mt", src, NULL);
}
