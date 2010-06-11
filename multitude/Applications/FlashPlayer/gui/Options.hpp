#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include "FlashPlayer.hpp"

#include <QDialog>
#include <QCheckBox>

namespace Ui
{
  class Options;
}

namespace FlashPlayer
{
  class Options : public QDialog
  {
    Q_OBJECT

  public:
    explicit Options(Screens screens, QRect rect);
    ~Options();

    void setRect(QRect rect, bool updateText = true);

  public slots:
    void textChanged(QString);
    void listChanged(int);

  protected:
    void changeEvent(QEvent * e);

  private:
    Ui::Options * m_ui;
    Screens m_screens;
    QVector<QCheckBox*> m_checkboxes;
    QRect m_rect;
  };
}

#endif // OPTIONS_HPP
