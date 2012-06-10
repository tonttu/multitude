/* COPYRIGHT
 */

#include "ShowGL.hpp"
#include "VideoDisplay.hpp"

#include "AudioTransfer.hpp"
#include "VideoInFFMPEG.hpp"

#include <Radiant/ImageConversion.hpp>
#include <Radiant/Sleep.hpp>
#include <Radiant/PlatformUtils.hpp>
#include <Radiant/Trace.hpp>

#include <Poetic/GPUFont.hpp>
#include <Poetic/CPUFont.hpp>

#include <Luminous/RenderContext.hpp>
#include <Luminous/Utils.hpp>

namespace VideoDisplay {

  using namespace Nimble;

#define SHADER(str) #str
  static const char * rgbshader = SHADER(
      uniform sampler2D tex;
      uniform float contrast;
      void main (void) {
        vec4 color = texture2D(tex, gl_TexCoord[0].st);
        color.rgb = vec3(0.5, 0.5, 0.5) +
           contrast * (color.rgb - vec3(0.5, 0.5, 0.5));
        gl_FragColor = color * gl_Color;
      });

  static const char * shadersource = SHADER(
      uniform sampler2D ytex;
      uniform sampler2D utex;
      uniform sampler2D vtex;
      uniform mat4 zm;
      void main (void) {
        vec4 ycolor = texture2D(ytex, gl_TexCoord[0].st);
        vec4 ucolor = texture2D(utex, gl_TexCoord[0].st);
        vec4 vcolor = texture2D(vtex, gl_TexCoord[0].st);
        vec4 yuv = vec4(ycolor.r, ucolor.r - 0.5, vcolor.r - 0.5, 1.0);
        yuv.rgb = (zm * yuv).rgb;
        gl_FragColor = yuv * gl_Color;
      });

  ShowGL::YUVProgram::YUVProgram(Luminous::RenderContext * resources)
      : Luminous::GLSLProgramObject(resources)
  {
    for(uint i = 0; i < PARAM_SIZEOF; i++)
      m_uniforms[i] = -1;
    init();
  }

  ShowGL::YUVProgram::~YUVProgram()
  {
    clear();
  }

  bool ShowGL::YUVProgram::init()
  {
    clear();

    Luminous::GLSLShaderObject * fragShader =
        new Luminous::GLSLShaderObject(GL_FRAGMENT_SHADER, context());

    fragShader->setSource(shadersource);
    if(!fragShader->compile()) {

      Radiant::error("ShowGL::YUVProgram::init # compile: %s",
                     fragShader->compilerLog());
      return false;
    }

    addObject(fragShader);

    return link();
  }

  void ShowGL::YUVProgram::bind(float contrast)
  {
    Luminous::GLSLProgramObject::bind();

    static const Nimble::Matrix4 yuv2rgb(
        1.0f,  0.0f,    1.403f, 0,
        1.0f, -0.344f, -0.714f, 0,
        1.0f,  1.77f,   0.0f,   0,
        0,     0,       0,      1);

    Nimble::Matrix4 m =
        Nimble::Matrix4::translate3D(Vector3(0.5f, 0.5f, 0.5f)) *
        Nimble::Matrix4::scale3D(Vector3(contrast, contrast, contrast)) *
        Nimble::Matrix4::translate3D(Vector3(-0.5f, -0.5f, -0.5f)) *
        yuv2rgb;

    glUniformMatrix4fv(m_uniforms[PARAM_MATRIX], 1, true, m.data());
    glUniform1i(m_uniforms[PARAM_YTEX], 0);
    glUniform1i(m_uniforms[PARAM_UTEX], 1);
    glUniform1i(m_uniforms[PARAM_VTEX], 2);
  }


  bool ShowGL::YUVProgram::link()
  {
    if(!Luminous::GLSLProgramObject::link()) {
      Radiant::error("ShowGL::YUVProgram::link # %s", linkerLog());
      return false;
    }

    const char * params [PARAM_SIZEOF] = {
      "ytex",
      "utex",
      "vtex",
      "zm"
    };

    bool ok = true;

    for(uint i = 0; i < PARAM_SIZEOF; i++) {
      int tmp = getUniformLoc(params[i]);
      m_uniforms[i] = tmp;
      ok = ok && (tmp >= 0);
      debugVideoDisplay("ShowGL::YUVProgram::link # %s -> %d", params[i], i);
    }

    return ok;
  }

  // static int __mytexcount = 0;

  ShowGL::MyTextures::MyTextures(Luminous::RenderContext * resources)
      : GLResource(resources)
  {
    m_frame = -1;

    memset(m_texSizes, 0, sizeof(m_texSizes));
    // __mytexcount++;
    // info("ShowGL::MyTextures::MyTextures # %d", __mytexcount);
  }

  ShowGL::MyTextures::~MyTextures()
  {
    // __mytexcount--;
    // info("ShowGL::MyTextures::~MyTextures # %d", __mytexcount);
  }

  void ShowGL::MyTextures::bind()
  {
    for(int i = 0; i < 3; i++) {
      glActiveTexture(GL_TEXTURE0 + i);
      glEnable(GL_TEXTURE_2D);
      m_texIds[i].bind();
    }
  }

  void ShowGL::MyTextures::unbind()
  {
    for(int i = 0; i < 3; i++) {
      glActiveTexture(GL_TEXTURE0 + i);
      glDisable(GL_TEXTURE_2D);
    }
    glActiveTexture(GL_TEXTURE0);
  }

  void ShowGL::MyTextures::doTextures(int frame, Radiant::VideoImage * img)
  {
    if(m_frame == frame)
      return;

    if(img->m_format < Radiant::IMAGE_RGB_24) {
      doTexturesYUV(img);
    } else {
      doTexturesRGB(img);
    }

    m_frame = frame;
  }

  Vector2i ShowGL::MyTextures::planeSize(const Radiant::VideoImage *img, size_t i)
  {
    Vector2i area(img->m_width, img->m_height);

    if(i) {
      if(img->m_format == Radiant::IMAGE_YUV_411P) {
        area.x /= 4;
      }
      else if(img->m_format == Radiant::IMAGE_YUV_420P) {
        area /= 2;
      }
      else if(img->m_format == Radiant::IMAGE_YUV_422P) {
        area.x /= 2;
      }
    }

    return area;
  }


  void ShowGL::MyTextures::doTexturesRGB(Radiant::VideoImage *img)
  {
    glActiveTexture(GL_TEXTURE0);

    glEnable(GL_TEXTURE_2D);
    Luminous::Texture2D * tex = & m_texIds[0];
    tex->bind();
    ImageFormat f = img->m_format;
    GLenum internalFormat = (f == IMAGE_RGBA || f == IMAGE_BGRA) ? GL_RGBA : GL_RGB;
    // use RGB as default
    Luminous::PixelFormat pf = Luminous::PixelFormat::rgbUByte();
    if (f == IMAGE_RGBA)
      pf = Luminous::PixelFormat::rgbaUByte();
    else if (f == IMAGE_BGRA)
      pf = Luminous::PixelFormat::bgraUByte();
    else if (f == IMAGE_BGR)
      pf = Luminous::PixelFormat::bgrUByte();

    tex->loadBytes(internalFormat, img->width(), img->height(), img->m_planes[0].m_data, pf);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  }

  void ShowGL::MyTextures::doTexturesYUV(Radiant::VideoImage *img)
  {

    for(uint i = 0; i < 3; i++) {

      glActiveTexture(GL_TEXTURE0 + i);
      glEnable(GL_TEXTURE_2D);
      Luminous::Texture2D * tex = & m_texIds[i];
      tex->bind();

      Vector2i area, real = planeSize(img, i);
      Vector2i & ts = m_texSizes[i];

      area = real;

      if(area.x & 0x3) {
        area.x -= area.x & 0x3;
      }
      if(area.y & 0x3)
        area.y -= area.y & 0x3;

      ts = area;

      if(m_frame < 0 || area != tex->size()) {

        debugVideoDisplay("ShowGL::YUVProgram::doTextures # area = [%d %d] ptr = %p",
              area.x, area.y, img->m_planes[i].m_data);

        tex->setWidth(area.x);
        tex->setHeight(area.y);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
                     area.x, area.y, 0,
                     GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

        Luminous::Utils::glCheck
            ("ShowGL::YUVProgram::doTextures # glTexImage2D");

      }

      // info("ShowGL::YUVProgram::doTextures # frame = %d, ts = [%d %d]",
      // frame, ts.x, ts.y);

      if(real.x & 0x3) {

        for(int y = 0; y < area.y; y++) {
          glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y,
                          ts.x, 1,
                          GL_LUMINANCE, GL_UNSIGNED_BYTE,
                          img->m_planes[i].m_data + y * real.x);

        }
      }
      else
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        ts.x, ts.y,
                        GL_LUMINANCE, GL_UNSIGNED_BYTE,
                        img->m_planes[i].m_data);

      Luminous::Utils::glCheck
          ("ShowGL::YUVProgram::doTextures # glTexSubImage2D");
    }
  }
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ShowGL::ShowGL()
      : m_video(0),
      m_frame(0),
      m_audio(0),
      m_targetChannel(-1),
      m_gain(1.0f),
      m_videoFrame(-1),
      m_count(0),
      m_state(PAUSE),
      m_updates(0),
      m_seeking(false),
      m_contrast(this, "contrast", 1.0f),
      m_fps(-1),
      m_syncToTime(true),
      m_outOfSync(0),
      m_outOfSyncTotal(0),
      m_syncing(false),
      m_frames(0)
  {
    eventAddOut("videoatend");
    debugVideoDisplay("ShowGL::ShowGL # %p", this);

    clearHistogram();

    m_dsp = Resonant::DSPNetwork::instance();
  }

  ShowGL::~ShowGL()
  {
    debugVideoDisplay("ShowGL::~ShowGL # %p", this);
    stop();
    delete m_video;
  }

  bool ShowGL::loadSubTitles(const char * filename, const char * )
  {
    return m_subTitles.readSrt(filename);
  }

  bool ShowGL::init(const char * /*filename */,
                    float /*previewpos*/,
                    int /*targetChannel*/,
                    int /*flags*/)
  {
    return false;
#if 0
    debugVideoDisplay("ShowGL::init # %s", filename);

    assert(filename != 0);

    m_fps = -1;

    if(m_state == PLAY) {
      stop();
    }

    m_filename = filename;

    m_targetChannel = targetChannel;

    VideoInFFMPEG * ffmpg = new VideoInFFMPEG();

    bool ok = ffmpg->init(filename, 0, flags);

    if(!ok) {
      Radiant::error("ShowGL::open # Could not open %s", filename);
      delete ffmpg;
      return false;
    }

    if(m_frame && (m_frame != & m_preview)) {
      Radiant::Guard g(m_video->mutex());

      m_preview.m_image.allocateMemory(m_frame->m_image);
      m_preview.m_image.copyData(m_frame->m_image);
      m_preview.m_time = m_frame->m_time;
      m_frame = & m_preview;
      debugVideoDisplay("Captured preview frame %p", m_frame);
    }

    delete m_video;
    m_video = ffmpg;

    m_position = 0;
    m_duration = Radiant::TimeStamp::createSecondsD(m_video->durationSeconds());
    m_seeking = true;

    debugVideoDisplay("ShowGL::init # Opened %s (%lf secs)",
                      filename, m_duration.secondsD());

    return true;
#endif
  }


  void ShowGL::setGain(float gain)
  {
    m_gain = gain;
    if(m_audio)
      m_audio->setGain(gain);
  }

  bool ShowGL::start(bool fromOldPos)
  {
    static int __count = 1;

    debugVideoDisplay("ShowGL::start # %p", this);

    if(m_state == PLAY) {
      if(!fromOldPos)
        seekTo(0);
      return true;
    } else if(!m_video) {
      return false;
    }

    AudioTransfer * au = new AudioTransfer(0, m_video);

    char buf[128];
    sprintf(buf, "showgl-audiotransfer-%p-%.4d", this, __count++);

    au->setId(buf);
    au->setGain(m_gain);

    m_dspItem = Resonant::DSPNetwork::Item();
    m_dspItem.setModule(au);
    m_dspItem.setTargetChannel(m_targetChannel);

    m_dsp->addModule(m_dspItem);

    m_audio = au;
    m_audio->setGain(m_gain);

    m_started = Radiant::TimeStamp::getTime();
    if(fromOldPos) {
      if(!m_video->atEnd())
        m_started -= m_video->displayFrameTime();
      m_video->play();
    } else {
      m_video->play(0);
    }

    m_state = PLAY;
    m_frames = 0;

    return true;
  }

  bool ShowGL::stop()
  {
    debugVideoDisplay("ShowGL::stop # %p", this);

    if(m_state != PLAY)
      return false;

    int i = 0;

    while(!m_audio->stopped() && !m_audio->started() && i < 10) {
      Radiant::Sleep::sleepMs(5);
      i++;
    }

    /* The order is important. The DSP network is supposed to delete the item,
       which may well happen before we hit the forgetVideo call. */
    m_audio->forgetVideo();
    m_dsp->markDone(m_dspItem);

    m_audio = 0;

    m_video->setAudioListener(0);
    m_video->stop();
    m_frames = 0;

    m_seeking = false;
    m_state = PAUSE;

    return true;
  }

  bool ShowGL::togglePause()
  {
    if (!stop()) {
      start();
      return false;
    }
    return true;
  }

  bool ShowGL::pause()
  {
    return stop();
  }

  bool ShowGL::unpause()
  {
    return start();
  }

  /*
  void ShowGL::enableLooping(bool enable)
  {
    m_video->enableLooping(enable);
  }
  */
  void ShowGL::update()
  {
    if(!m_video)
      return;

    int videoFrame;

    if(m_audio) {
      if(m_syncToTime) {
        if(m_video->fps() <= 0.0f) {
          m_frames = 0;
          return;
        }
        if(m_frames > 10 && m_videoFrame > 1 && m_fps < 0) {
          VideoIn::Frame * f = m_video->getFrame(m_videoFrame-1, false);
          VideoIn::Frame * f2 = m_video->getFrame(m_videoFrame, false);

          if(f && f2) {
            // Timestamps can be equal for first frames!
            float tmp = 1.0f / (f2->m_absolute.secondsD() - f->m_absolute.secondsD());

            // Check if our estimated fps is even remotely feasible
            if(tmp > 1.f && tmp < 100.f)
              m_fps = tmp;
          }
        }
        float fps = m_fps > 0 ? m_fps : m_video->fps();
        int videoFrameFromTime = m_started.sinceSecondsD() * fps;
        int videoFrameFromAudio = m_audio->videoFrame();
        if(videoFrameFromAudio < 0) return;

        if(++m_frames < 10) {
          m_started = Radiant::TimeStamp::getTime() - Radiant::TimeStamp::createSecondsD(videoFrameFromAudio / fps);
        }

        int diff = videoFrameFromTime - videoFrameFromAudio;
        int adiff = Nimble::Math::Abs(diff);
        // Radiant::error("ShowGL::update # diff %d %d (fps %f)", syncing, diff, fps);

        if(adiff > (m_syncing ? 0 : 2) && ++m_outOfSync > (m_syncing ? 10 : 60)) {
          if(m_outOfSyncTotal > 120 || adiff > 10) {
            Radiant::error("ShowGL::update # Video out of sync, resyncing. %d (fps %f)", diff, fps);
            m_started = Radiant::TimeStamp::getTime() - Radiant::TimeStamp::createSecondsD(videoFrameFromAudio / fps);
          } else {
            // Radiant::error("ShowGL::update # Video out of sync, adjusting. %d (fps %f)", diff, fps);
            m_started += Radiant::TimeStamp::createSecondsD((diff > 0 ? 1.0f : -1.0f) / fps);

          }
          m_syncing = true;
          m_outOfSync = 0;
          videoFrameFromTime = videoFrameFromAudio;
        } else if(adiff == 0) {
          //if(outOfSync > 0) Radiant::info("Aborting sync correction %d", outOfSync);
          m_syncing = false;
          m_outOfSync = 0;
          m_outOfSyncTotal = 0;
        }
        if(m_syncing) ++ m_outOfSyncTotal;

        videoFrame = m_videoFrame > videoFrameFromTime + 20 ?
              videoFrameFromTime : Nimble::Math::Max(videoFrameFromTime, m_videoFrame);
      } else videoFrame = m_audio->videoFrame();
      if(m_audio->atEnd()) {
        debugVideoDisplay("ShowGL::update # At end");
        stop();

        Radiant::BinaryData bd;
        eventSend("videoatend", bd);
      }
      // info("Audio reports frame %d", videoFrame);

    }
    else {
      m_video->freeUnusedMemory();
      // videoFrame = m_videoFrame;
      if(m_seeking)
        videoFrame = (int) m_video->latestFrame();
      else
        videoFrame = m_videoFrame;
      // info("Video has frame %d", videoFrame);
    }

    if(videoFrame < 0)
      return;

    Radiant::Guard g(m_video->mutex());

    VideoIn::Frame * f = m_video->getFrame(videoFrame, true);

    if(!f) {
      debugVideoDisplay("ShowGL::update # NO FRAME %d", videoFrame);
      return;
    }

    m_position = f->m_absolute;

    m_histogram[m_updates % HISTOGRAM_POINTS] = videoFrame - m_videoFrame;

    m_updates++;

    m_frame = f;

    if(m_videoFrame != videoFrame) {
      debugVideoDisplay("ShowGL::update # Move %d -> %d (%lf, %d x %d)",
            m_videoFrame, videoFrame, m_position.secondsD(),
            m_frame->m_image.m_width, m_frame->m_image.m_height);
      m_count++;
      m_videoFrame = videoFrame;
    }

    debugVideoDisplay("ShowGL::update # %p f = %p index = %d", this, m_frame, videoFrame);

    // m_subTitles.update(m_position);
  }

  static Luminous::Collectable yuvkey;
  static Luminous::Collectable rgbkey;
  // static int yuvkeyaa = 0;

  void ShowGL::render(Luminous::RenderContext * resources,
                      Vector2 topleft, Vector2 bottomright,
                      Radiant::Color baseColor,
                      const Nimble::Matrix3f * transform,
                      Poetic::GPUFont * subtitleFont,
                      float subTitleSpace)
  {

    debugVideoDisplay("ShowGL::render");
    GLRESOURCE_ENSURE(MyTextures, textures, this, resources);

    Luminous::GLSLProgramObject * shader = 0;

    Vector2i s = size();

    if(bottomright == topleft) {
      bottomright.x = topleft.x + s.x;
      bottomright.y = topleft.y + s.y;
    }

    debugVideoDisplay("ShowGL::render # %p f = %p", this, m_frame);

    Luminous::Utils::glCheck("ShowGL::render # entry");

    if(m_frame) {

      // debugVideoDisplay("ShowGL::render # %p %p", this, m_frame);

      textures->doTextures(m_count, & m_frame->m_image);
      textures->bind();

      if(m_frame->m_image.m_format < Radiant::IMAGE_RGB_24) {
        GLRESOURCE_ENSURE(YUVProgram, yuv2rgb, & yuvkey, resources);
        yuv2rgb->bind(m_contrast);
        shader = yuv2rgb;
      }
      else {
        GLRESOURCE_ENSURE(Luminous::GLSLProgramObject, rgb2rgb, & rgbkey, resources);
        if(rgb2rgb->shaderObjectCount() == 0) {
          rgb2rgb->loadStrings(0, rgbshader);
          debugVideoDisplay("Loaded rgb2rgb shader");
        }

        rgb2rgb->bind();
        glUniform1f(rgb2rgb->getUniformLoc("contrast"), m_contrast);

        shader = rgb2rgb;
        // info("No shader needed, plain RGB video.");
      }
    }

    Luminous::Utils::glCheck("ShowGL::render # half");

    glEnable(GL_BLEND);

    if(transform) {
      Luminous::Utils::glTexRectAA(bottomright - topleft, *transform,
                                   baseColor.data());
    }
    else {
      Nimble::Rect r(topleft, bottomright);
      Luminous::Utils::glTexRectAA(r, baseColor.data());
    }

    // Then a thin strip around to anti-alias:

    if(shader)
      shader->unbind();

    textures->unbind();

    const SubTitles::Text * sub = m_subTitles.current();

    if(!subtitleFont && sub) {
      Radiant::error("ShowGL::render # Missing the subtitle font");
    }

    if(subtitleFont && sub) {

      glEnable(GL_BLEND);
      glEnable(GL_TEXTURE_2D);

      // puts("RENDERING SUBS");

      Poetic::CPUFont * cpuFont = subtitleFont->cpuFont();

      float fontH = cpuFont->lineHeight();
      float subH = fontH * 2.2f;

      int linecount = sub->lineCount();

      bool below = false;

      if(subTitleSpace <= 0)
        subTitleSpace = bottomright.y;
      else if((subTitleSpace - subH) <  bottomright.y)
        subTitleSpace = bottomright.y;
      else {
        subTitleSpace = bottomright.y + subH;
        below = true;
      }
      Nimble::Vector2f loc(topleft.x + fontH, subTitleSpace - fontH * 0.2f);

      if(linecount == 1) {
        if(below)
          loc.y -= fontH;
        subtitleFont->render(sub->m_lines[0], loc);
      }
      else if(linecount  == 2) {
        subtitleFont->render(sub->m_lines[1], loc);
        loc.y -= fontH;
        subtitleFont->render(sub->m_lines[0], loc);
      }
    }

    // info("The video frame is %d", m_videoFrame);

    Luminous::Utils::glCheck("ShowGL::render");
    debugVideoDisplay("ShowGL::render # EXIT");
  }

  Nimble::Vector2i ShowGL::size() const
  {
    if(!m_video)
      return Nimble::Vector2i(640, 480);

    assert(m_video != 0);

    return m_video->vdebug().m_videoFrameSize;
  }

  void ShowGL::seekTo(Radiant::TimeStamp time)
  {
    if(!m_video)
      return;

    time = Nimble::Math::Clamp<Radiant::TimeStamp>(time, 0, m_duration);
    debugVideoDisplay("ShowGL::seekTo # %lf", time.secondsD());
    m_position = time;
    m_started = Radiant::TimeStamp::getTime() - time;

    m_video->seek(time);
    m_seeking = true;
  }

  void ShowGL::seekToRelative(double relative)
  {
    if(!m_video)
      return;

    seekTo(TimeStamp(duration() * relative));
  }

  void ShowGL::panAudioTo(Nimble::Vector2 location)
  {

    if(!m_video)
      return;

    if(!m_audio)
      return;

    debugVideoDisplay("ShowGL::panAudioTo # %p %p %p [%.2f %.2f]", this, m_video, m_audio,
          location.x, location.y);

    char buf[128];

    Radiant::BinaryData control;

    control.writeString("panner/setsourcelocation");

    snprintf(buf, sizeof(buf), "%s-%d", m_audio->id().toUtf8().data(), (int) 0);

    control.writeString(buf);
    control.writeVector2Float32(location); // sound source location

    m_dsp->send(control);
  }

  void ShowGL::setSyncToTime(bool flag)
  {
    m_syncToTime = flag;
  }

  void ShowGL::clearHistogram()
  {
    memset(m_histogram, 0, sizeof(m_histogram));
  }

  Radiant::TimeStamp ShowGL::firstFrameTime() const
  {
    if(m_video)
      return m_video->firstFrameTime();

    return 0;
  }
}
