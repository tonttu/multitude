#ifndef OPTIONS_HPP
#define OPTIONS_HPP

#include "FlashPlayer.hpp"

#include <QDialog>
#include <QCheckBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>

namespace Ui
{
  class Options;
}

namespace FlashPlayer
{
  class ScreenView : public QGraphicsView
  {
    Q_OBJECT

  public:
    ScreenView(QWidget * parent = 0) : QGraphicsView(parent) {}
    virtual ~ScreenView() {}
    void resizeEvent(QResizeEvent *);

  signals:
    void resized();
  };

  class Options;
  class ScreenItem : public QGraphicsRectItem
  {
  public:
    ScreenItem(const QRectF & rect, Options & options, int id)
      : QGraphicsRectItem(rect), m_options(options), m_id(id) {}
    virtual ~ScreenItem() {}

  protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *);
    Options & m_options;
    int m_id;
  };

  class Options : public QDialog
  {
    Q_OBJECT

  public:
    explicit Options(Screens screens, QRect rect, bool automatic);
    virtual ~Options();

    void setRect(QRect rect, bool updateText = true);
    bool ok() const { return m_accepted; }
    bool automatic() const;
    QString view() const;

    void toggle(int id);

  public slots:
    void textChanged(QString);
    void listChanged(int);
    void viewResized();
    void accepted();

  protected:
    void changeEvent(QEvent * e);

  private:
    QGraphicsScene m_scene;
    QGraphicsRectItem * m_viewport;
    Ui::Options * m_ui;
    Screens m_screens;
    QVector<QCheckBox*> m_checkboxes;
    QRect m_rect;
    bool m_manualUpdate;
    bool m_accepted;
  };
}

#endif // OPTIONS_HPP
