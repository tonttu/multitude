#ifndef FLASH_PLAYER_HPP
#define FLASH_PLAYER_HPP

#include <QVector>
#include <QRect>

class Screens
{
public:
  bool update();
  QString id() const;

private:
  struct Screen
  {
    int screen;
    int x, y;
    int width, height;
    operator QRect() const
    {
      return QRect(x, y, width, height);
    }
  };

  QVector<Screen> m_screens;
  QRect m_view;
};

#endif
