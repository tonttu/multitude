#ifndef FLASH_PLAYER_HPP
#define FLASH_PLAYER_HPP

#include <QVector>
#include <QRect>

namespace FlashPlayer
{
  struct Screen
  {
    int screen;
    int x, y;
    int width, height;
    operator QRect() const
    {
      return QRect(x, y, width, height);
    }
    static QString rectToId(const QRect & rect);
    static QRect idToRect(const QString & id);
  };

  class Screens
  {
  public:
    bool update();
    QString id() const;
    size_t size() const { return m_screens.size(); }
    Screen operator[] (int num) const { return m_screens[num]; }

  private:
    QVector<Screen> m_screens;
    QRect m_view;
  };
}

#endif
