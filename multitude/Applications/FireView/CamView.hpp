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

#ifndef FIREVIEW_CAMVIEW_HPP
#define FIREVIEW_CAMVIEW_HPP

#include <Luminous/Texture.hpp>

#include <Nimble/Rect.hpp>

#include <Radiant/Mutex.hpp>
#include <Radiant/Thread.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/VideoCamera.hpp>

#include <QGLWidget>
#include <QTimer>

namespace FireView {

  using namespace Nimble;

  class ParamView;

  class CamView : public QGLWidget
  {
    Q_OBJECT;
  public:
    CamView(QWidget * parent);
    virtual ~CamView();

    bool start(uint64_t euid64, Radiant::FrameRate fps, float customFps = 0.0f,
         Radiant::VideoCamera::TriggerSource triggerSource = Radiant::VideoCamera::TriggerSource(-1), Radiant::VideoCamera::TriggerMode triggerMode = Radiant::VideoCamera::TriggerMode(-1),
	       bool format7 = false);

    std::vector<Radiant::VideoCamera::CameraFeature> & features()
    { return m_thread.m_features; }

    void updateParam(int i) { m_thread.m_featureSend[i] = true;}
    void autoParam(int i) { m_thread.m_autoSend[i] = true;}

    static void setVerbose(bool verbose) { m_verbose = verbose; }
    static bool verbose() { return m_verbose; }

    static void setTriggerPolarity(Radiant::VideoCamera::TriggerPolarity p)
    { m_triggerPolarity = p; }

    static Radiant::VideoCamera::TriggerPolarity triggerPolarity() { return m_triggerPolarity; }

    static void setFormat7area(int x1, int y1, int x2, int y2)
    { m_format7rect.low().make(x1, y1); m_format7rect.high().make(x2, y2);  }

    static void setFormat7mode(int mode)
    { m_format7mode = mode; }

    static Nimble::Recti format7Area()
    { return m_format7rect; }

    static int format7Mode()
    { return m_format7mode; }

  public slots:

    void openParams();
    void showAverages();
    void toggleHalfInchToThirdInch();
    void locate();
    void triggerAnalysis();
    void updateScreen();
    void toggleFiltering()
    { m_filtering = !m_filtering; }

  protected:
    
    enum HalfToThird {
      AS_HALF,
      AS_VGA_THIRD,
      AS_WIDE_VGA_THIRD,
      AS_COUNT
    };

    virtual void mouseMoveEvent(QMouseEvent * e);
    virtual void mousePressEvent(QMouseEvent * e);

    virtual void hideEvent ( QHideEvent * e );
    virtual void closeEvent ( QCloseEvent * e );
    virtual void keyPressEvent ( QKeyEvent * e );
    virtual void paintGL();

    void grabImageLuminosity(int screenx, int screeny);
    Nimble::Vector2i screenToImage(int screenx, int screeny) const;
    Nimble::Vector2i imageToScreen(Nimble::Vector2i) const;
    
    Rect getEffectiveArea();
    void doAnalysis();

    class InputThread : public Radiant::Thread
    {
    public:

      enum State {
	UNINITIALIZED,
	STARTING,
	FAILED,
	RUNNING
      };
      
      friend class CamView;
      InputThread();
      virtual ~InputThread();

      bool start(uint64_t euid64, Radiant::FrameRate fps, 
     float customFps, Radiant::VideoCamera::TriggerSource triggerSource, Radiant::VideoCamera::TriggerMode triggerMode,
		 bool format7);
      void stop();

      bool isRunning() const { return m_state == RUNNING; }
      
    protected:

      virtual void childLoop();

      bool openCamera();

      Radiant::VideoCamera * m_camera;
      Radiant::MutexAuto m_mutex;
      Radiant::VideoImage m_frame;
      Radiant::FrameRate m_fps;
      float           m_customFps;
      Radiant::VideoCamera::TriggerSource m_triggerSource;
      Radiant::VideoCamera::TriggerMode m_triggerMode;
      bool            m_format7;

      std::vector<Radiant::VideoCamera::CameraFeature> m_features;
      std::vector<bool> m_featureSend;
      std::vector<bool> m_autoSend;
      
      volatile State m_state;
      volatile bool  m_continue;
      volatile int   m_frameCount;

      Radiant::TimeStamp m_lastCheckTime;
      int                m_lastCheckFrame;
      float              m_lastCheckFps;

      uint64_t m_euid64;
    };

    class Analysis
    {
    public:

      float average;
      Vector2 center;
    };

    enum {
      AREA_DIVISION = 5,
      AREA_COUNT = AREA_DIVISION * AREA_DIVISION
    };

    int m_texFrame;
    bool m_filtering;

    Luminous::Texture2D * m_tex;

    InputThread m_thread;
    QTimer      m_timer;

    ParamView  *m_params;

    bool        m_showAverages;
    HalfToThird m_halfToThird;
    bool        m_doAnalysis;
    float       m_imageScale;

    QString     m_text;
    Nimble::Vector2i m_textLoc;
    float       m_textColor;

    Analysis   m_averages[AREA_COUNT]; // Grid.
    QImage     m_foo;
    static bool m_verbose;
    static Radiant::VideoCamera::TriggerPolarity m_triggerPolarity;

    static Nimble::Recti m_format7rect;
    static int           m_format7mode;
  };

}

#endif
