#include "Options.hpp"
#include "ui_Options.h"

#include <QRegExpValidator>
#include <QGraphicsRectItem>

#include <cmath>

namespace FlashPlayer
{
  void ScreenView::resizeEvent(QResizeEvent *)
  {
    if(scene()) {
      fitInView(scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
      scale(0.95, 0.95);
      emit resized();
    }
  }

  void ScreenItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
  {
    m_options.toggle(m_id);
    setSelected(false);
  }

  Options::Options(Screens screens, QRect rect, bool automatic)
    : m_viewport(new QGraphicsRectItem(rect)),
    m_ui(new Ui::Options),
    m_screens(screens),
    m_manualUpdate(false),
    m_accepted(false)
  {
    m_ui->setupUi(this);
    m_ui->text->setValidator(new QRegExpValidator(QRegExp("\\d+x\\d+\\+\\d+\\+\\d+"), this));
    connect(m_ui->text, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
    connect(m_ui->view, SIGNAL(resized()), this, SLOT(viewResized()));
    connect(m_ui->buttons, SIGNAL(accepted()), this, SLOT(accepted()));

    m_ui->automatic->setChecked(automatic);

    m_ui->screenlist->setRowCount(screens.size());
    Qt::GlobalColor colors[] = {Qt::cyan, Qt::magenta, Qt::red, Qt::green, Qt::blue, Qt::yellow,
                                Qt::darkCyan, Qt::darkMagenta, Qt::darkRed, Qt::darkGreen, Qt::darkBlue, Qt::darkYellow};
    for(size_t i = 0; i < screens.size(); ++i) {
      Screen s = screens[i];
      QCheckBox * cbox = new QCheckBox("Screen #" + QString::number(s.screen));
      connect(cbox, SIGNAL(stateChanged(int)), this, SLOT(listChanged(int)));
      m_checkboxes << cbox;
      m_ui->screenlist->setCellWidget(i, 0, cbox);

      QTableWidgetItem * item = new QTableWidgetItem(QString("%1x%2").arg(s.width).arg(s.height));
      item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
      m_ui->screenlist->setItem(i, 1, item);

      item = new QTableWidgetItem(QString("(%1,%2)").arg(s.x).arg(s.y));
      item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
      m_ui->screenlist->setItem(i, 2, item);

      ScreenItem * monitor = new ScreenItem(static_cast<QRect>(s), *this, i);
      monitor->setFlag(QGraphicsItem::ItemIsSelectable);
      monitor->setBrush(QBrush(colors[i % (sizeof(colors)/sizeof(*colors))]));
      monitor->setPen(QPen(colors[i % (sizeof(colors)/sizeof(*colors))]));
      m_scene.addItem(monitor);
    }
    m_ui->screenlist->resizeColumnsToContents();

    m_viewport->setBrush(QColor(0, 0, 0, 10));
    m_viewport->setZValue(1);
    m_scene.addItem(m_viewport);
    setRect(rect);
    m_ui->view->setScene(&m_scene);
  }

  Options::~Options()
  {
    delete m_ui;
  }

  bool Options::automatic() const
  {
    return m_ui->automatic->isChecked();
  }

  QString Options::view() const
  {
    return m_ui->text->text();
  }

  void Options::toggle(int id)
  {
    m_checkboxes[id]->toggle();
  }

  void Options::viewResized()
  {
    QPen pen = m_viewport->pen();
    QPointF x = QPointF(1, 0) * m_ui->view->transform();
    QPointF y = QPointF(0, 1) * m_ui->view->transform();
    float scale = std::sqrt(x.x()*x.x() + x.y()*x.y()) * 0.5f +
                  std::sqrt(y.x()*y.x() + y.y()*y.y()) * 0.5f;
    pen.setWidth(3.0f/scale);
    pen.setJoinStyle(Qt::MiterJoin);
    m_viewport->setPen(pen);
  }

  void Options::accepted()
  {
    m_accepted = true;
  }

  void Options::setRect(QRect rect, bool updateText)
  {
    m_rect = rect;
    m_viewport->setRect(rect);
    for(size_t i = 0; i < m_screens.size(); ++i) {
      Screen s = m_screens[i];
      if(rect.contains(s)) {
        m_checkboxes[i]->setTristate(false);
        m_checkboxes[i]->setChecked(true);
      } else if (rect.intersects(s)) {
        m_checkboxes[i]->setTristate(true);
        m_checkboxes[i]->setCheckState(Qt::PartiallyChecked);
      } else {
        m_checkboxes[i]->setTristate(false);
        m_checkboxes[i]->setChecked(false);
      }
      m_checkboxes[i]->repaint();
    }
    m_ui->view->resizeEvent(0);
    if(updateText) m_ui->text->setText(Screen::rectToId(rect));
  }

  void Options::textChanged(QString txt)
  {
    QRect rect = Screen::idToRect(txt);
    if(rect == m_rect) return;
    m_manualUpdate = true;
    setRect(rect, false);
    m_manualUpdate = false;
  }

  void Options::listChanged(int)
  {
    if(m_manualUpdate) return;
    QRect rect;
    for(size_t i = 0; i < m_screens.size(); ++i) {
      if(m_checkboxes[i]->isChecked()) {
        rect |= m_screens[i];
      }
    }
    setRect(rect);
  }

  void Options::changeEvent(QEvent * e)
  {
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
      m_ui->retranslateUi(this);
      break;
    default:
      break;
    }
  }
}
