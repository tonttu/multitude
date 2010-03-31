/* COPYRIGHT
 *
 * This file is part of Applications/FireView.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Applications/FireView.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "MainWindow.hpp"

#include "CamView.hpp"

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>

#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QVBoxLayout>

#include <Radiant/Trace.hpp>
#include <Radiant/VideoCamera.hpp>
#include <Radiant/CameraDriver.hpp>

#include <assert.h>

namespace FireView {

  MainWindow::MainWindow(Radiant::FrameRate rate, 
                         float customFps, Radiant::VideoCamera::TriggerSource triggerSource, Radiant::VideoCamera::TriggerMode triggerMode,
			 bool format7)
    : m_mdi(0),
      m_rate(rate),
      m_customFps(customFps),
      m_triggerSource(triggerSource),
      m_triggerMode(triggerMode),
      m_format7(format7)
  {
    QMenuBar * bar = new QMenuBar(this);
    QMenu * menu = new QMenu("&File", this);
    menu->addAction("E&xit", QCoreApplication::instance(), SLOT(quit()),
                    Qt::CTRL | Qt::Key_Q);

    bar->addMenu(menu);
    
    setMenuBar(bar);
  }
  
  MainWindow::~MainWindow()
  {
    Radiant::debug("MainWindow::~MainWindow");
    
    for(std::set<QWidget *>::iterator it = m_displays.begin();
        it != m_displays.end(); it++) {
      delete (*it);
    }
  }

  bool MainWindow::init()
  {
    resize(400, 300);

    // m_mdi = new QMdiArea(this);

    // setCentralWidget(m_mdi);
    // setCentralWidget(this);

    checkCameras();

        /*
    QTimer * timer = new QTimer(this);

    connect(timer, SIGNAL(timeout()), this, SLOT(checkCameras()));

    timer->start(2000);
*/
    QTimer::singleShot(2000, this, SLOT(checkCameras()));
    return true;
  }

  void MainWindow::checkCameras()
  {
    std::vector<Radiant::VideoCamera::CameraInfo> infos;
    Radiant::CameraDriver * cd = Radiant::VideoCamera::drivers().getPreferredCameraDriver();
    if(cd) cd->queryCameras(infos);

    for(unsigned i = 0; i < infos.size(); i++) {
      uint64_t euid = infos[i].m_euid64;
      
      if(euid < 0x10000)
        ;
      else if(m_cameras.find(euid) == m_cameras.end()) {

	qDebug("Adding camera %d %llx",
	       (int) m_cameras.size() + 1, (long long) euid);

	QWidget * base = new QWidget();

	int loc = m_cameras.size() * 30;
	base->move(loc % 800 + 30, loc % 300 + 30);
	base->resize(640, 480);

	QMenuBar * mb = new QMenuBar(base);
	QMenu * menu = new QMenu(mb);
	CamView * cv = new CamView(base);

	menu->addAction("OpenGL Image Filtering", cv, SLOT(toggleFiltering()));
	menu->addAction("Parameters...", cv, SLOT(openParams()));
	menu->addAction("Show averages", cv, SLOT(showAverages()));
	menu->addAction("1/2\" -> 1/3\"", cv,
			SLOT(toggleHalfInchToThirdInch()));
	menu->addAction("Update Screen", cv, SLOT(updateScreen()));
	menu->setTitle("Configuration");
        QAction * q = new QAction("Quit", base);
        q->setShortcut(tr("Ctrl+Q"));
        connect(q, SIGNAL(triggered()),
                QCoreApplication::instance(), SLOT(quit()));
        menu->addAction(q);
	mb->addMenu(menu);
	
	QVBoxLayout * layout = new QVBoxLayout(base);

	layout->setSpacing(0);
#if QT_VERSION >= 0x040300
	layout->setContentsMargins(0, 0, 0, 0);
#endif
	layout->addWidget(mb);
	layout->addWidget(cv, 100);
	
	if(cv->start(euid, m_rate, m_customFps,
		     m_triggerSource, m_triggerMode, m_format7)) {

	  base->raise();
	  base->show();
          m_displays.insert(base);
	}
	else
	  delete base;

	m_cameras.insert(euid);
      }
    }
  }




}
