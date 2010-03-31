/* COPYRIGHT
 *
 * This file is part of Applications/MoviePlayer.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Applications/MoviePlayer.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef VIDEO_WINDOW_HPP
#define VIDEO_WINDOW_HPP

#ifdef WIN32
#define _WINSOCKAPI_	// timeval struct redefinition
#endif

#include <Poetic/GPUFont.hpp>


#include <Luminous/GLResources.hpp>

#include <Nimble/Random.hpp>

#include <Resonant/DSPNetwork.hpp>

#include <VideoDisplay/ShowGL.hpp>

#include <Radiant/ResourceLocator.hpp>

#include <QtOpenGL/QGLWidget>
#include <QtCore/QTimer>

class VideoWindow : public QGLWidget
{
  Q_OBJECT;
public:
  VideoWindow();
  virtual ~VideoWindow();

  /** Adds a new movie to the player. Calling open multiple times will
      open several videos inside the window.  */
  bool open(const char * filename, const char * audiodev);

  /** Performs a stress test, using all the movies that are available. */
  void stressTest();

  static void setContrast(float contrast)
  { m_contrast = contrast; }

public slots:

  /** Perform some random operation as part of the stress testing. */
  void randomOperation();

protected:

  class Item {
  public:
    Item() {}
    ~Item() {}

    VideoDisplay::ShowGL    m_show;
  };

  typedef std::list<Radiant::RefPtr<Item> > container;
  typedef container::iterator iterator;

  virtual void keyPressEvent(QKeyEvent * event);

  virtual void mousePressEvent(QMouseEvent * e);
  virtual void mouseReleaseEvent(QMouseEvent * e);

  virtual void initializeGL();
  virtual void paintGL();

  void toggleFullScreen();

  container m_movies;

  Resonant::DSPNetwork    m_dsp;

  QTimer m_timer;
  Radiant::TimeStamp m_lastActivity;
  Poetic::CPUFont  * m_subCPUFont;
  // Poetic::GPUFont * m_subGPUFont;

  Radiant::ResourceLocator   m_resourceLocator;

  Luminous::GLResources      m_glResources;
  Nimble::RandomUniform m_rand;
  static float m_contrast;
  bool   m_showProgress;
  bool   m_showSteps;
};

#endif

