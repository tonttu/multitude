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

#include "CamView.hpp"

#include "ParamView.hpp"

#include <Radiant/Sleep.hpp>
#include <Radiant/TimeStamp.hpp>

#include <Luminous/Utils.hpp>
#include <Luminous/PixelFormat.hpp>

#include <Radiant/StringUtils.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/CameraDriver.hpp>

#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>

#include <QtGui/QAction>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>

namespace FireView {

  using namespace Radiant;

  CamView::InputThread::InputThread()
      : m_camera(0),
      m_state(UNINITIALIZED),
      m_continue(false),
      m_frameCount(0),
      m_lastCheckFrame(0),
      m_lastCheckFps(0)
  {
  }

  CamView::InputThread::~InputThread()
  {}

  bool CamView::InputThread::start(uint64_t euid64, Radiant::FrameRate fps,
                                   float customFps,
                                   Radiant::VideoCamera::TriggerSource triggerSource, Radiant::VideoCamera::TriggerMode triggerMode,
                                   bool format7)
  {
    m_euid64 = euid64;
    /*
    Radiant::trace("CamView::InputThread::start # %f %f # %d %d",
       customFps, Radiant::asFloat(fps),
       triggerSource, triggerMode);
    */
    while(customFps > Radiant::asFloat(fps) && fps < Radiant::FPS_60) {
      // puts("Adjusting the core FPS");
      fps = (Radiant::FrameRate) (fps + 1);
    }
    m_fps = fps;
    m_customFps = customFps;
    m_triggerSource = triggerSource;
    m_triggerMode = triggerMode;
    m_format7 = format7;

    m_state = STARTING;
    m_continue = true;
    m_frameCount = 0;

    m_lastCheckTime = Radiant::TimeStamp::getTime();
    m_lastCheckFrame = 0;
    m_lastCheckFps = 0;

    if(!run())
      return false;

    while(m_state == STARTING) {
      Radiant::Sleep::sleepMs(20);
    }

    return m_state == RUNNING;
  }

  void CamView::InputThread::stop()
  {
    if(!m_continue)
      return;

    m_continue = false;
    waitEnd();
  }
  /*
  static const char * modeName(dc1394feature_mode_t mode)
  {
    if(mode == DC1394_FEATURE_MODE_MANUAL)
      return "manual";
    else if(mode == DC1394_FEATURE_MODE_AUTO)
      return "auto";
    else if(mode == DC1394_FEATURE_MODE_ONE_PUSH_AUTO)
      return "one-push-auto";

    return "unknown";
  }
*/
  using Radiant::StringUtils::yesNo;

  void CamView::InputThread::childLoop()
  {
    if(!openCamera())
      return;

    m_state = RUNNING;

    Radiant::TimeStamp starttime(Radiant::TimeStamp::getTime());
    Radiant::SleepSync sync;
    sync.resetTiming();

    debug("Capturing video");

    m_lastCheckTime = Radiant::TimeStamp::getTime();

    while(m_continue) {

      // printf("<"); fflush(0);

      if(m_customFps > 0.0f && !m_format7) {
        sync.sleepSynchroUs((long) (1000000 / m_customFps));
        m_camera->sendSoftwareTrigger();
      }

      int timeout = m_frameCount ? 8000 : 15000;
      m_camera->setCaptureTimeout(timeout);

      const Radiant::VideoImage * img = m_camera->captureImage();

      if (img == 0) {
        error("No video image after waiting %lf ms", timeout);

        m_camera->close();
        if(m_frameCount > 10) {
          // Wait ten seconds and try re-opening the camera

          Radiant::Sleep::sleepS(10);

          debug("Attempting to re-open camera.");

          m_frameCount = 0;

          if(!openCamera()) {
            m_continue = false;
            break;
          }
          else
            continue;
        }
        else {
          m_continue = false;
          // m_frameCount = m_frameCount;
          break;
        }
      }

      // qDebug("CamView::InputThread::childLoop # Captured");

      Radiant::Guard g( & m_mutex);

      m_frame.allocateMemory(*img);

      m_frame.copyData(*img);
      m_frameCount++;
      m_camera->doneImage();

      for(unsigned i = 0; i < m_features.size(); i++) {
        if(m_featureSend[i]) {
          m_camera->setFeatureRaw(m_features[i].id, m_features[i].value);
          m_featureSend[i] = false;
        }
        else if(m_autoSend[i]) {
          m_camera->setFeature(m_features[i].id, -1);
          m_autoSend[i] = false;
        }
      }

      Radiant::TimeStamp now = Radiant::TimeStamp::getTime();

      double dt = Radiant::TimeStamp(now - m_lastCheckTime).secondsD();

      if(dt > 3.0f) {
        int frames = m_frameCount - m_lastCheckFrame;
        m_lastCheckFps = frames / dt;
        m_lastCheckFrame = m_frameCount;
        m_lastCheckTime = now;

        // qDebug("FPS = %f", m_lastCheckFps);
      }

      // printf(">"); fflush(0);
      // qDebug("CamView::InputThread::childLoop # Frame");
    }

    // qDebug("CamView::InputThread::childLoop # DONE");

    float fps = m_frameCount /
                Radiant::TimeStamp(Radiant::TimeStamp::getTime() - starttime).secondsD();

    m_frame.freeMemory();


    qDebug("CamView::InputThread::childLoop # camid = %llx # EXIT (%.2f fps, %d frames)",
           (long long) m_camera->cameraInfo().m_euid64, fps, (int) m_frameCount);

    m_camera->stop();
    m_camera->close();

    m_state = UNINITIALIZED;

  }

  bool CamView::InputThread::openCamera()
  {
    bool ok;

    m_camera = Radiant::VideoCamera::drivers().createPreferredCamera();
    if(!m_camera) return false;

    if(!m_format7) {
      ok = m_camera->open(m_euid64, 640, 480, Radiant::IMAGE_UNKNOWN, m_fps);
    }
    else {

      if(m_customFps <= 3) {
        m_customFps = 15;
      }

      Nimble::Recti r = CamView::format7Area();
      ok = m_camera->openFormat7(m_euid64, r, m_customFps, CamView::format7Mode());
    }

    if(!ok) {
      m_state = FAILED;
      return false;
    }

    if(m_verbose) {
      // Try to print some information
      std::vector<VideoCamera::CameraFeature> features;
      m_camera->getFeatures( & features);

      for(uint i = 0; i < features.size(); i++) {
        VideoCamera::CameraFeature & info = features[i];

        if(!info.available)
          continue;
        printf(" Feature %u = %s: \n"
               "  Capabilities:\n"
               "   absolute = %s\n   readout = %s\n"
               "   on-off = %s\n   polarity = %s\n"
               "  On = %s\n",
               i, Radiant::VideoCamera::featureName(info.id),
               yesNo(info.absolute_capable),
               yesNo(info.readout_capable),
               yesNo(info.on_off_capable),
               yesNo(info.polarity_capable),
               yesNo(info.is_on));

        printf("  Value = %u in [%u %u]\n",
               info.value, info.min, info.max);
        printf("  Abs value = %f in [%f %f]\n",
               info.abs_value, info.abs_min, info.abs_max);

      }
      fflush(0);
    }

    // Set trigger mode if needed
    if(m_triggerSource != Radiant::VideoCamera::TRIGGER_SOURCE_MAX) {

      if(!m_camera->enableTrigger(m_triggerSource)) {
        Radiant::error("CamView::InputThread::openCamera # failed to enable trigger (source %d)", m_triggerSource);
        m_state = FAILED;
        return false;
      }

      debug("Enabled trigger (source %d).", m_triggerSource);

      if(m_triggerMode != Radiant::VideoCamera::TRIGGER_MODE_MAX) {
        if(!m_camera->setTriggerMode(m_triggerMode)) {
          Radiant::error("CamView::InputThread::openCamera # failed to set trigger mode %d", m_triggerMode);
          m_state = FAILED;
          return false;
        }

        debug("Set trigger mode %d.", m_triggerMode);
      }

    } else {
      m_camera->disableTrigger();
      debug("Disabled trigger.");
    }

    m_camera->setTriggerPolarity(CamView::triggerPolarity());
    debug("Set trigger polarity to %d", CamView::triggerPolarity());

    debug("Getting features");

    m_camera->getFeatures( & m_features);

    {
      m_featureSend.resize(m_features.size());

      for(unsigned i = 0; i < m_features.size(); i++) {
        Radiant::VideoCamera::CameraFeature & info = m_features[i];
        if(info.id == Radiant::VideoCamera::GAMMA &&
           info.value > ((info.max * 3 + info.min) / 4)) {
          /* If gamma appears to be too high, bring it down. This is
       done because some cameras when powered up (Unibrain 521b
       for example), initialize to maximum gamma value, which
       makes the image look strange, sometimes even fully
       white/gray.
    */
          info.value = (info.max + info.min) / 2;
          m_featureSend[i] = true;
        }
        else
          m_featureSend[i] = false;
      }

      m_autoSend = m_featureSend;
    }

    debug("Starting video capture");
    if(!m_camera->start()) {
      m_state = UNINITIALIZED;
      Radiant::error("Could not start video capture");
      return false;
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  bool CamView::m_verbose = false;
  Radiant::VideoCamera::TriggerPolarity CamView::m_triggerPolarity = Radiant::VideoCamera::TriggerPolarity(-1);
  int CamView::m_format7mode = 1;

  Nimble::Recti CamView::m_format7rect(0, 0, 2000, 1500);

  static int __interval = 50;

  CamView::CamView(QWidget * parent)
      : QGLWidget(parent),
      m_tex(0),
      m_params(0),
      m_showAverages(false),
      m_halfToThird(AS_HALF),
      m_doAnalysis(false),
      m_imageScale(1),
      m_foo(300, 100, QImage::Format_ARGB32)
  {
    // QTimer::singleShot(1000, this, SLOT(locate()));
    connect( & m_timer, SIGNAL(timeout()), this, SLOT(updateGL()));
    bzero(m_averages, sizeof(m_averages));
    setFocusPolicy(Qt::ClickFocus);

    // setAttribute(Qt::WA_DeleteOnClose, true);

    QTimer * t = new QTimer(this);

    connect(t, SIGNAL(timeout()), this, SLOT(triggerAnalysis()));
    t->start(1000);
  }

  CamView::~CamView()
  {
    delete m_params;
    m_thread.stop();
  }

  bool CamView::start(uint64_t euid64, Radiant::FrameRate fps,
                      float customFps,
                      Radiant::VideoCamera::TriggerSource triggerSource, Radiant::VideoCamera::TriggerMode triggerMode, bool format7)
  {
    Radiant::VideoCamera::CameraInfo info;

    // Find the camera info by guid
    std::vector<VideoCamera::CameraInfo> cameras;
    Radiant::CameraDriver * cd = Radiant::VideoCamera::drivers().getPreferredCameraDriver();
    if(cd) cd->queryCameras(cameras);

    for(size_t i = 0; i < cameras.size(); i++)
      if((uint64_t) cameras[i].m_euid64 == euid64) { info = cameras[i]; break; }

    QString title;

    title.sprintf("%s: %s (%llx)",
                  info.m_vendor.c_str(), info.m_model.c_str(),
                  (long long) euid64);

    ((QWidget *) parent())->setWindowTitle(title);

    m_texFrame = -1;
    m_filtering = false;

    bool ok = m_thread.start(euid64, fps, customFps,
                        triggerSource, triggerMode, format7);

    if(ok) {
      Radiant::Guard g( & m_thread.m_mutex);
      Radiant::VideoImage frame = m_thread.m_frame;
      move(100, 100);
      resize(frame.width(), frame.height());
      m_timer.start(__interval);
    }
    else
      m_timer.stop();

    return ok;
  }

  void CamView::openParams()
  {
    if(!m_params) {
      m_params = new ParamView(this);
      m_params->init();
    }

    m_params->raise();
    m_params->show();
  }

  void CamView::showAverages()
  {
    m_showAverages = !m_showAverages;
    m_doAnalysis = true;
  }

  void CamView::toggleHalfInchToThirdInch()
  {
    m_halfToThird = (HalfToThird) ((m_halfToThird + 1) % AS_COUNT);
    m_doAnalysis = true;
  }

  void CamView::locate()
  {
    QWidget * p = dynamic_cast<QWidget *> (parent());
    if(!p)
      p = this;
    // p->raise();
    // p->show();
    Radiant::VideoImage frame = m_thread.m_frame;
    p->resize(frame.width(), frame.height());
  }

  void CamView::triggerAnalysis()
  {
    if(m_showAverages)
      m_doAnalysis = true;
  }

  void CamView::updateScreen()
  {
    if(m_timer.isActive())
      m_timer.stop();
    else
      m_timer.start(__interval);
  }

  void CamView::mouseMoveEvent(QMouseEvent * e)
  {
    mousePressEvent(e);
  }

  void CamView::mousePressEvent(QMouseEvent * e)
  {
    grabImageLuminosity(e->x(), e->y());
  }


  void CamView::hideEvent ( QHideEvent * event )
  {
    // qDebug("CamView::hideEvent");
    QGLWidget::hideEvent(event);
    // close();
  }

  void CamView::closeEvent ( QCloseEvent * event )
  {
    // qDebug("CamView::closeEvent");
    QGLWidget::closeEvent(event);
  }

  void CamView::keyPressEvent ( QKeyEvent * e )
  {
    if(e->key() == Qt::Key_Space) {
      m_doAnalysis = true;
    }
    else
      e->ignore();
  }

  void CamView::paintGL()
  {
    using Luminous::PixelFormat;

    if(!m_tex)
      m_tex = new Luminous::Texture2D;

    if(m_thread.m_frameCount && m_texFrame != m_thread.m_frameCount) {

      Radiant::Guard g( & m_thread.m_mutex);
      Radiant::VideoImage frame = m_thread.m_frame;

      if((m_tex->width()  != frame.width()) || (m_tex->height() != frame.height())) {

          m_tex->loadBytes(GL_LUMINANCE, frame.width(), frame.height(), frame.m_planes[0].m_data, PixelFormat(PixelFormat::LAYOUT_LUMINANCE, PixelFormat::TYPE_UBYTE), false);

      }
      else {
        m_tex->bind();

        // puts("subimage");

        //  if(!frame.m_planes.empty())
        //    bzero(frame.m_planes[0].m_data, 640 * 20); // black strip

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        frame.width(), frame.height(),
                        GL_LUMINANCE, GL_UNSIGNED_BYTE,
                        frame.m_planes[0].m_data);
      }

      m_texFrame = m_thread.m_frameCount;

      if(m_doAnalysis)
        doAnalysis();
    }


    // trace("Frame = %d", (int) m_texFrame);

    int dw = width();
    int dh = height();

    glViewport (0, 0, dw, dh);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();

    gluOrtho2D(0, dw, dh, 0);

    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();

    if(m_texFrame >= 0) {
      glEnable(GL_TEXTURE_2D);
      m_tex->bind();
      glColor3f(1.0f, 1.0f, 1.0f);

      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

      float aspect = m_tex->width() / (float) m_tex->height();
      float myaspect = width() / (float) height();
      float imw, imh;

      if(myaspect < aspect) {
        imw = width();
        imh = imw / aspect;
      }
      else {
        imh = height();
        imw = imh * aspect;
      }

      m_imageScale = imw / m_tex->width();

      if(m_filtering) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      }
      else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      }

      Luminous::Utils::glTexRect(0, 0, imw, imh);
    }

    glDisable(GL_TEXTURE_2D);

    if(m_halfToThird) {
      Rect sq = getEffectiveArea();

      float phase = (m_texFrame % 60) / 60.0f;
      float green = sinf(phase * Math::TWO_PI) * 0.5f + 0.5f;
      glColor3f(green * 0.5f, green, 0.0f);

      glLineWidth(1);
      glBegin(GL_LINE_STRIP);

      glVertex2fv(sq.low().data());
      glVertex2f(sq.high().x, sq.low().y);
      glVertex2fv(sq.high().data());
      glVertex2f(sq.low().x, sq.high().y);
      glVertex2fv(sq.low().data());

      glEnd();
    }

    if(m_showAverages || m_texFrame < 0) {

      QPainter foo( & m_foo);
      QString tmp;
      // QFont fnt = font();

      for(int i = 0; i < AREA_COUNT && m_showAverages; i++) {

        Analysis & an = m_averages[i];
        tmp.sprintf("%.1f", an.average);

        if(an.average < 128)
          glColor3f(1.0f, 1.0f, 1.0f);
        else
          glColor3f(0.0f, 0.0f, 0.0f);

        float w = foo.boundingRect(0, 0, 500, 500, Qt::AlignLeft, tmp).width();
        renderText((int) (an.center.x - w * 0.5f), (int) an.center.y, tmp);
      }

      if(m_texFrame < 0) {
        glColor3f(1.0f, 1.0f, 1.0f);
        const char * warningtext = "Waiting for camera frames";
        float w = foo.boundingRect(0, 0, 500, 500,
                                   Qt::AlignLeft, warningtext).width();
        renderText((int) (width() * 0.5f - w * 0.5f), height() / 2,
                   warningtext);
      }
    }
    glColor3f(1.0f, 1.0f, 1.0f);

    char state[64];
    sprintf(state, "%.4f FPS %d frames", m_thread.m_lastCheckFps,
            m_thread.m_frameCount);

    renderText(5, 18, state);

    if(!m_text.isEmpty()) {
      glEnable(GL_TEXTURE_2D);
      Nimble::Vector2i sp = imageToScreen(m_textLoc);
      Luminous::Utils::glGrayf(m_textColor);
      renderText(sp.x + 10, sp.y, m_text);
    }
  }

  void CamView::grabImageLuminosity(int screenx, int screeny)
  {
    if(m_thread.m_frameCount < 2)
      return;

    Nimble::Vector2i p = screenToImage(screenx, screeny);

    // Radiant::Guard g( & m_thread.m_mutex);

    Radiant::VideoImage frame = m_thread.m_frame;

    if((uint) p.x >= (uint) frame.width() ||
       (uint) p.y >= (uint) frame.height()) {
      m_text.clear();
      return;
    }

    int lumi = frame.m_planes[0].line(p.y)[p.x];

    char buf[64];

    sprintf(buf, "%d [%d %d]", lumi, p.x, p.y);

    m_text = buf;
    m_textLoc = p;
    m_textColor = lumi < 128 ? 1.0f : 0.0f;
  }

  Nimble::Vector2i CamView::screenToImage(int screenx, int screeny) const
  {
    return Nimble::Vector2i((int) (screenx / m_imageScale),
                            (int) (screeny / m_imageScale));
  }

  Nimble::Vector2i CamView::imageToScreen(Nimble::Vector2i p) const
  {
    return Nimble::Vector2i((int) (p.x * m_imageScale),
                            (int) (p.y * m_imageScale));
  }

  /** Returns the effective imaging area. The area includes the border
      pixels. */
  Rect CamView::getEffectiveArea()
  {
    if(!m_tex->width() || !m_tex->height())
      return Rect(0, 0, 0, 0);

    int w = m_tex->width() - 1;
    int h = m_tex->height() - 1;

    if(m_halfToThird == AS_VGA_THIRD) {
      /* We assume that the pixels in half inch sensor are 9.9f / 7.4f times
   the size of pixels in third-inch sensor. Half-inch sensor has
   9.9um pixels (for VGA) and third-inch comes with 7.4um
   pixels.*/
      float scale = 9.9f / 7.4f;
      float remove = (scale - 1.0f) * 0.5f;
      float keep = 1.0f - remove;

      int lx = Nimble::Math::Round(remove * w);
      int ly = Nimble::Math::Round(remove * h);
      int hx = Nimble::Math::Round(keep * w);
      int hy = Nimble::Math::Round(keep * h);

      return Rect(lx, ly, hx, hy);
    }
    else if(m_halfToThird == AS_WIDE_VGA_THIRD) {
      /* Assume that one is using a camera with 1/2 inch sensor (Sony
   IXC 414) or the like, and the we wish to show the area with
   wide VGA sensor.

   0,0061875um pixels in the wide VGA sensor, resolution
   750x480.
      */
      float scale_y = 9.9f / 6.1875f;
      float remove_y = (scale_y - 1.0f) * 0.5f;
      float keep_y = 1.0f - remove_y;

      float scale_x = 9.9f / 6.1875f * (750.0f / 640.0f);
      float remove_x = (scale_x - 1.0f) * 0.5f;
      float keep_x = 1.0f - remove_x;

      int lx = Nimble::Math::Round(remove_x * w);
      int ly = Nimble::Math::Round(remove_y * h);
      int hx = Nimble::Math::Round(keep_x * w);
      int hy = Nimble::Math::Round(keep_y * h);

      return Rect(lx, ly, hx, hy);
    }

    return Rect(0, 0, w, h);
  }

  void CamView::doAnalysis()
  {
    Rect a = getEffectiveArea();

    Vector2 span = a.span();

    Radiant::VideoImage frame = m_thread.m_frame; // Already mutex locked, safe access

    float s = 1.0f / AREA_DIVISION;

    for(int i = 0; i < AREA_DIVISION; i++) {

      int ly = (int) (a.low().y + span.y * (i * s));
      int hy = (int) (a.low().y + span.y * ((i + 1) * s));

      for(int j = 0; j < AREA_DIVISION; j++) {

        int lx = (int) (a.low().x + span.x * (j * s));
        int hx = (int) (a.low().x + span.x * ((j + 1) * s));

        int sum = 0;

        for(int y = ly; y <= hy; y++) {
          const uint8_t * pixel = frame.m_planes[0].line(y) + lx;
          const uint8_t * sentinel = pixel + hx - lx;

          while(pixel <= sentinel)
            sum += *pixel++;
        }

        int xx = hx - lx + 1;
        int yy = hy - ly + 1;

        Analysis & an = m_averages[i * AREA_DIVISION + j];

        an.average = sum / (float) (xx * yy);
        an.center.make((lx + hx) * 0.5, (ly + hy) * 0.5f);
      }
    }

    m_doAnalysis = false;
  }
}
