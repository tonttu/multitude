#include "Options.hpp"
#include "ui_Options.h"

#include <QRegExpValidator>

namespace FlashPlayer
{
  Options::Options(Screens screens, QRect rect)
    : m_ui(new Ui::Options),
    m_screens(screens)
  {
    m_ui->setupUi(this);
    m_ui->view->setValidator(new QRegExpValidator(QRegExp("\\d+x\\d+\\+\\d+\\+\\d+"), this));
    connect(m_ui->view, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));

    m_ui->screenlist->setRowCount(screens.size());
    for(size_t i = 0; i < screens.size(); ++i) {
      Screen s = screens[i];
      QCheckBox * cbox = new QCheckBox("Enable #" + QString::number(s.screen));
      connect(cbox, SIGNAL(stateChanged(int)), this, SLOT(listChanged(int)));
      m_checkboxes << cbox;
      //cbox->setTristate(true);
      m_ui->screenlist->setCellWidget(i, 0, cbox);

      QTableWidgetItem * item = new QTableWidgetItem(QString("%1x%2").arg(s.width).arg(s.height));
      item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
      m_ui->screenlist->setItem(i, 1, item);

      item = new QTableWidgetItem(QString("(%1,%2)").arg(s.x).arg(s.y));
      item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
      m_ui->screenlist->setItem(i, 2, item);
    }
    setRect(rect);
    // m_ui->screenlist->resizeColumnsToContents();
  }

  Options::~Options()
  {
    delete m_ui;
  }

  void Options::setRect(QRect rect, bool updateText)
  {
    m_rect = rect;
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
    if(updateText) m_ui->view->setText(Screen::rectToId(rect));
  }

  void Options::textChanged(QString txt)
  {
    QRect rect = Screen::idToRect(txt);
    if(rect == m_rect) return;
    setRect(rect, false);
  }

  void Options::listChanged(int)
  {
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
