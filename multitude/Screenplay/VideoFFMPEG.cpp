/* COPYRIGHT
 *
 * This file is part of Screenplay.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Screenplay.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#include "VideoFFMPEG.hpp"

#include <Radiant/Mutex.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/Types.hpp>

#include <string.h>

#include <strings.h>
#include <cassert>


extern "C" {
# include <libavformat/avformat.h>
# include <libavcodec/avcodec.h>
# include <libavutil/avutil.h>
}

namespace Screenplay {

  using namespace Radiant;

  static Radiant::MutexStatic __openmutex;
  static Radiant::MutexStatic __countermutex;
  int    __instancecount = 0;

  int VideoInputFFMPEG::m_debug = 1;

  VideoInputFFMPEG::VideoInputFFMPEG()
    : m_acodec(0),
    m_aindex(-1),
    m_acontext(0),
    m_audioFrames(0),
    m_audioChannels(0),
    m_capturedAudio(0),
    m_capturedVideo(0),
    m_vcodec(0),
    m_vindex(-1),
    m_vcontext(0),
    m_frame(0),
    m_ic(0),
    m_pkt(0),
    m_flags(0),
    m_lastPts(0)
  {
    static bool once = false;

    if(!once) {
      debug("Initializing AVCODEC 1");
      avcodec_init();
      debug("Initializing AVCODEC 2");
      avcodec_register_all();
      debug("Initializing AVCODEC 3");
      av_register_all();
      debug("Initializing AVCODEC 4");
      once = true;
    }

    m_lastSeek = 0;

    m_pkt = new AVPacket();
    av_init_packet(m_pkt);

    int tmp = 0;
    {
      Radiant::GuardStatic g(__countermutex);
      __instancecount++;
      tmp = __instancecount;
    }
    debug("VideoInputFFMPEG::VideoInputFFMPEG # Instance count at %d", tmp);
  }

  VideoInputFFMPEG::~VideoInputFFMPEG()
  {
    close();
    av_free_packet(m_pkt);
    delete m_pkt;

    int tmp = 0;
    {
      Radiant::GuardStatic g(__countermutex);
      __instancecount--;
      tmp = __instancecount;
    }
    debug("VideoInputFFMPEG::~VideoInputFFMPEG # Instance count at %d", tmp);
  }

  const Radiant::VideoImage * VideoInputFFMPEG::captureImage()
  {
    assert(this != 0);

    static const char * fname = "VideoInputFFMPEG::captureImage";

    int got = false;

    int vlen, got_picture;

    while(!got) {

      // printf(","); fflush(0);

      av_free_packet(m_pkt);

      int ret = av_read_packet(m_ic, m_pkt);

      if(ret < 0) {

        /* In the following we do not free the packet, since the read failed. Hope
           this is the correct behavior. */
        debug("VideoInputFFMPEG::captureImage ret < 0 %x", m_flags);

        if(! (m_flags & DO_LOOP)) {
          // av_free_packet(m_pkt);
          return 0;
        }
        else {
          debug("VideoInputFFMPEG::captureImage # Looping %s", m_fileName.c_str());
          m_offsetTS = m_lastTS;
          av_seek_frame(m_ic, -1, (int64_t) 0, 0);

          // av_free_packet(m_pkt);
          ret = av_read_packet(m_ic, m_pkt);
          if(ret < 0) {
            // av_free_packet(m_pkt);
            return 0;
          }
        }
      }

      // trace("TS NOW %ld", (long) m_ic->timestamp);

      // trace2("%s # %d %d", fname, ret, m_pkt.stream_index);

      got_picture = 0;

      if (m_pkt->stream_index == m_vindex) {
        vlen = avcodec_decode_video(m_vcontext,
                                    m_frame, & got_picture,
                                    m_pkt->data, m_pkt->size);
        // printf("|");fflush(0);

        if (got_picture) {
          got = true;

          int64_t pts = m_pkt->dts;
          if(pts <= 0)
            pts = m_frame->pts;
          if(pts <= 0)
            pts = m_pkt->pts;
          if(pts <= 0)
            pts = 0;

          AVRational time_base = m_vcontext->time_base;

          if(!time_base.num || !time_base.den)
            time_base = m_ic->streams[m_vindex]->time_base;
          if(pts != m_frame->pts && m_ic->streams[m_vindex]->time_base.num)
            time_base = m_ic->streams[m_vindex]->time_base;

          double rate = av_q2d(time_base);
          double secs = pts * rate; // x av_q2d(time_base);
          /*
             printf("VideoInputFFMPEG::captureImage # ls = %d, "
             "pts = %ld (%ld/%ld) secs %.2lf\n",
             (int) m_frame->linesize[0], (long) pts,
             (long) time_base.num, (long) time_base.den,
             secs);
             */
          m_lastPts = pts;

          if((m_lastPts == 0 && m_capturedVideo > 2) || (secs == 0)) {

            // int dpn = m_frame->display_picture_number;
            // int dpn = m_vcontext->frame_number;
            // int dpn = m_vcontext->real_pict_num;
            // trace("VideoInputFFMPEG::captureImage # DPN = %d", dpn);

            /* Video time-code is broken, just assume fixed frame
               rate. We could of-course figure this out on the fly. */
            m_lastTS = Radiant::TimeStamp::createSecondsD
                       (m_capturedVideo / 30.0 + m_lastSeek);
          }
          else
            m_lastTS = Radiant::TimeStamp::createSecondsD(secs);

          debug("VideoInputFFMPEG::captureImage # pts = %d %d %d lts = %lf\n",
                (int) m_frame->pts, (int) m_pkt->pts, (int) m_pkt->dts,
                m_lastTS.secondsD());

          m_lastTS += m_offsetTS;

          if(m_capturedVideo == 0)
            m_firstTS = m_lastTS;
        }

        if(m_sinceSeek == 0) {
          /*
          Radiant::TimeStamp sought = Radiant::TimeStamp::createSecondsD(m_lastSeek);
          if(Radiant::TimeStamp(sought - m_lastTS).secondsD() > 1.1f)
            m_offsetTS = sought;
          else
            m_offsetTS = 0;
      */
        }

        // m_lastTS += m_offsetTS;

        m_sinceSeek++;
      }
      if (m_pkt->stream_index == m_aindex && (m_flags & Radiant::WITH_AUDIO)
        && m_acodec) {

        int index = m_audioFrames * actualChannels();

        int aframes = (m_audioBuffer.size() - index) * 2;

        if(aframes < AVCODEC_MAX_AUDIO_FRAME_SIZE) {
          m_audioBuffer.resize(m_audioBuffer.size() + AVCODEC_MAX_AUDIO_FRAME_SIZE * m_audioChannels);
          aframes = (m_audioBuffer.size() - index) * 2;
          if(m_audioBuffer.size() > 1000000) {
            info("VideoInputFFMPEG::captureImage # %p Audio buffer is very large now: %d (%d)",
                 this, (int) m_audioBuffer.size(), m_capturedVideo);
          }
        }

        avcodec_decode_audio2(m_acontext,
                              & m_audioBuffer[index],
                              & aframes, m_pkt->data, m_pkt->size);

        aframes /= (2 * m_audioChannels);
        int64_t pts = m_pkt->pts;

        if(m_flags & Radiant::MONOPHONIZE_AUDIO) {
          // Force to mono sound.
          for(int a = 0; a < aframes; a++) {
            int sum = 0;
            int base = index + a * m_audioChannels;
            for(int chan = 0; chan < m_audioChannels; chan++) {
              sum += m_audioBuffer[base + chan];
            }
            // Scale down to avoid clipping.
            m_audioBuffer[index + a] = sum / m_audioChannels;
          }
        }

        if(pts <= 0)
          pts = m_pkt->dts;
        if(pts <= 0)
          pts = m_acontext->frame_number;

        AVRational time_base = m_acontext->time_base;

        if(!time_base.num || !time_base.den)
          time_base = m_ic->streams[m_aindex]->time_base;
        if(pts != m_frame->pts && m_ic->streams[m_aindex]->time_base.num)
          time_base = m_ic->streams[m_aindex]->time_base;

        double rate = av_q2d(time_base);
        double secs = pts * rate;


        debug("VideoInputFFMPEG::captureImage # af = %d ab = %d ppts = %d, pdts = %d afr = %d secs = %lf tb = %ld/%ld",
              aframes, m_audioFrames, (int) m_pkt->pts, (int) m_pkt->dts,
              (int) m_acontext->frame_number, secs,
              (long) time_base.num, (long) time_base.den);

        if(aframes > 10000)
          pts = m_capturedAudio;

        if(m_audioFrames == 0) {
          if(secs > 0.0001)
            m_audioTS = TimeStamp::createSecondsD(secs);
          else if(pts)
            m_audioTS = TimeStamp::createSecondsD(pts / 44100.0);
          else
            m_audioTS = TimeStamp::createSecondsD(m_capturedAudio / 44100.0);

          m_audioTS += m_offsetTS;
        }

        debug("Decoding audio # %d %lf", aframes, m_audioTS.secondsD());

        m_audioFrames   += aframes;
        m_capturedAudio += aframes;

        if((uint)(m_audioFrames * m_audioChannels) >= m_audioBuffer.size()) {
          Radiant::error("VideoInputFFMPEG::captureImage # %p Audio trouble %d %d (%d)",
                         this, aframes, m_audioFrames, m_capturedVideo);
        }
        // printf("_"); fflush(0);
      }
    }

    if(!m_acodec && (m_flags & Radiant::WITH_AUDIO)) {
      // Produce silent audio to fool the playback engine.

      double secs = TimeStamp(m_lastTS - m_firstTS).secondsD();
      double frames = secs * 44100;

      int perFrame = (int) (frames - m_capturedAudio);

      if(perFrame > 20000) {
        debug("VideoInputFFMPEG::captureImage # Large audio generated");
        perFrame = 20000;
      }

      debug("VideoInputFFMPEG::captureImage # %lf %d %d %d aufr in total %d vidfr",
            secs, perFrame, (int) m_audioFrames, (int) m_capturedAudio, (int) m_capturedVideo);

      m_audioTS = m_lastTS;

      m_audioFrames   += perFrame;
      m_capturedAudio += perFrame;

      if((uint)(m_audioFrames * m_audioChannels) >= m_audioBuffer.size()) {
        error("VideoInputFFMPEG::captureImage # Audio trouble B %d %d %lf",
              perFrame, m_audioFrames, secs);
        assert(perFrame > 0);
      }
    }

    m_capturedVideo++;

    /* trace2("VideoInputFFMPEG::captureImage # m_vcontext->pix_fmt = %d",
       (int) m_vcontext->pix_fmt); */

    PixelFormat avcfmt = m_vcontext->pix_fmt;

    if(avcfmt == PIX_FMT_YUV420P) {
      m_image.setFormatYUV420P();
      if(m_debug && m_capturedVideo < 10)
        debug("%s # PIX_FMT_YUV420P", fname);
    }
    else if(avcfmt == PIX_FMT_YUVJ420P) {
      m_image.setFormatYUV420P();
      if(m_debug && m_capturedVideo < 10)
        debug("%s # PIX_FMT_YUV420P", fname);
    }
    else if(avcfmt == PIX_FMT_YUVJ422P) {
      m_image.setFormatYUV422P();
      if(m_debug && m_capturedVideo < 10)
        debug("%s # PIX_FMT_YUV422P", fname);
    }
    else if(avcfmt == PIX_FMT_RGB24) {
      m_image.setFormatRGB();
      if(m_debug && m_capturedVideo < 10)
        debug("%s # PIX_FMT_RGB24", fname);
    }
    else {
      Radiant::error("%s # unsupported FFMPEG pixel format %d", fname, (int) avcfmt);
    }

    m_image.m_width = width();
    m_image.m_height = height();

    m_image.m_planes[0].m_data = m_frame->data[0];
    m_image.m_planes[0].m_linesize = m_frame->linesize[0];

    m_image.m_planes[1].m_data = m_frame->data[1];
    m_image.m_planes[1].m_linesize = m_frame->linesize[1];

    m_image.m_planes[2].m_data = m_frame->data[2];
    m_image.m_planes[2].m_linesize = m_frame->linesize[2];

    if(!m_image.m_width) {
      error("Captured image has zero width %d %d %d",
            m_image.m_planes[0].m_linesize,
            m_image.m_planes[1].m_linesize,
            m_image.m_planes[2].m_linesize);
    }

    /* for(uint p = 0; p < 3; p++) {
       printf("ls[%u] = %d  ", p, (int) m_image.m_planes[p].m_linesize);
       }*/

    av_free_packet(m_pkt);

    return & m_image;
  }

  /** This function does not decode anything, it just returns data
    decoded in the "captureFrame". */

  const void * VideoInputFFMPEG::captureAudio(int * frameCount)
  {
    /* trace2("VideoInputFFMPEG::captureAudio # %d %d",
     * frameCount, m_audioFrames); */

    if(!m_audioBuffer.size()) {
      * frameCount = 0;
      return 0;
    }

    * frameCount = m_audioFrames;
    // void * ptr = & m_audioBuffer[m_audioFrames * m_audioChannels];
    m_audioFrames = 0;
    return & m_audioBuffer[0];
  }

  void VideoInputFFMPEG::getAudioParameters(int * channels,
                                            int * sample_rate,
                                            Radiant::AudioSampleFormat * format)
  {
    if(!m_acontext){

      if(m_flags & WITH_AUDIO) {
        * channels = 2;
        * sample_rate = 44100;
        * format = Radiant::ASF_INT16;
      }
      else {
        * channels = 0;
        * sample_rate = 0;
        * format = Radiant::ASF_INT16;
      }
    }
    else {
      *channels = actualChannels();

      int sr = m_acontext->sample_rate;
      * sample_rate = sr;
      * format = Radiant::ASF_INT16;

    }
  }

  int VideoInputFFMPEG::width() const
  {
    return m_vcontext ? m_vcontext->width : 0;
  }

  int VideoInputFFMPEG::height() const
  {
    return m_vcontext ? m_vcontext->height : 0;
  }

  float VideoInputFFMPEG::fps() const
  {
    if(!m_vcontext)
      return 0;

    AVRational time_base = m_vcontext->time_base;

    if(!time_base.num || !time_base.den) {

      time_base = m_ic->streams[m_vindex]->time_base;

      if(!time_base.num || !time_base.den) {

        if(m_capturedVideo && m_capturedAudio) {
          int sr = m_acontext->sample_rate;
          float seconds = m_capturedAudio / (float) sr;
          return m_capturedVideo / seconds;
        }
        else
          return 30.0f;
      }
    }

    double r = (double) time_base.den /
               ((double) time_base.num);

    if(m_lastPts != 0) {
      r *= (double) (m_capturedVideo - 1) / (double) m_lastPts;
    }

    Radiant::debug("VideoInputFFMPEG::fps # %d %d -> %.2lf",
                   time_base.den, time_base.num, r);

    return float(r);
    // return 30;
  }

  Radiant::ImageFormat VideoInputFFMPEG::imageFormat() const
  {
    return m_image.m_format;
  }

  unsigned int VideoInputFFMPEG::size() const
  {
    return m_image.size();
  }

  bool VideoInputFFMPEG::open(const char * filename,
                              int flags)
  {

    if(m_vcodec)
      close();

    assert(filename != 0 && m_vcodec == 0);

    m_flags = 0;
    m_lastPts = 0;
    m_fileName = filename;
    m_audioTS = 0;
    m_audioFrames = 0;

    if(flags & Radiant::MONOPHONIZE_AUDIO)
      m_flags |= Radiant::MONOPHONIZE_AUDIO;

    const char * fname = "VideoInputFFMPEG::open";

    int i;

    AVInputFormat * iformat = 0;
    AVFormatParameters params, *ap = & params;

    m_capturedVideo = 0;
    m_lastTS = 0;
    m_firstTS = 0;
    m_offsetTS = 0;
    m_sinceSeek = 0;

    bzero( & params, sizeof(params));

    GuardStatic g( & __openmutex);

    int err = av_open_input_file( & m_ic, filename, iformat, 0, ap);

    if(err < 0) {
      error("%s # Could not open file \"%s\" %s",
            fname, filename, strerror(-err));
      return false;
    }

    /* trace2("%s # Opened %s with %d Hx, %d channels, %dx%d",
       fname, filename, ap->sample_rate, ap->channels,
       ap->width, ap->height); */

    av_read_play(m_ic);

    for(i = 0; i < (int) m_ic->nb_streams; i++) {

      AVCodecContext *enc = m_ic->streams[i]->codec;

      if(enc->codec_type == CODEC_TYPE_VIDEO) {

        m_vindex = i;
        m_vcodec = avcodec_find_decoder(enc->codec_id);
        m_vcontext = enc;

        AVRational fr = m_ic->streams[i]->r_frame_rate;

        debug("%s # Got frame rate of %d %d", fname, fr.num, fr.den);

        /* if(m_vcodec->supported_framerates) {
           for(int k = 0; m_vcodec->supported_framerates[k].num != 0; k++) {
           fr = m_vcodec->supported_framerates[k];
           trace2("%s # Supported frame rate of %d %d",
           fname, fr.num, fr.den);
           }
           }*/

        if(!m_vcodec || avcodec_open(enc, m_vcodec) < 0)
          ; // THROW1(Exception, "Could not get video codec")
        else if(flags & WITH_VIDEO)
          m_flags = m_flags | WITH_VIDEO;
      }
      else if(enc->codec_type == CODEC_TYPE_AUDIO) {

        m_aindex = i;
        m_acodec = avcodec_find_decoder(enc->codec_id);
        m_acontext = enc;

        if((!m_acodec || avcodec_open(enc, m_acodec) < 0) &&
           (flags & Radiant::WITH_AUDIO))
          ; // THROW1(Exception, "Could not get audio codec")
        else if(flags & WITH_AUDIO)
          m_flags = m_flags | WITH_AUDIO;
      }
    }

    if(flags & WITH_AUDIO)
      m_flags = m_flags | WITH_AUDIO;

    if(flags & DO_LOOP)
      m_flags |= DO_LOOP;

    m_audioFrames = 0;
    m_capturedAudio = 0;

    if(m_aindex >= 0 && m_acontext) {
      m_audioBuffer.resize(100000 * 2);
      m_audioChannels = m_acontext->channels;
      m_audioSampleRate = m_acontext->sample_rate;
    }
    else if(flags & Radiant::WITH_AUDIO) {
      m_audioBuffer.resize(100000 * 2);
      m_audioChannels = 2;
      m_audioSampleRate = 44100;
    }

    m_frame = avcodec_alloc_frame();

    m_image.m_width = width();
    m_image.m_height = height();

    const char * vcname = m_vcodec ? m_vcodec->name : 0;
    const char * acname = m_acodec ? m_acodec->name : 0;

    float ratio = m_vcontext ?
                  (float) av_q2d(m_vcontext->sample_aspect_ratio) : 0.0f;

    m_lastSeek = 0;

    if(!vcname) {
      error("%s # File %s has unsupported video codec.", fname, filename);
      return false;
    }

    if(!acname) {
      debug("%s # File %s has unsupported audio codec.", fname, filename);
      // return false;
    }

    info("%s # Opened file %s,  (%d x %d %s, %s %d chans @ %d Hz) %d (%d, %f)",
          fname, filename, width(), height(), vcname, acname, m_audioChannels,
          m_audioSampleRate,
          (int) m_image.m_format, (int) m_vcontext->pix_fmt, ratio);

    return true;
  }

  bool VideoInputFFMPEG::close()
  {
    //    if(!m_ic)
    //      return false;

    GuardStatic g( & __openmutex);

    if(m_frame)
      av_free(m_frame);

    if(m_acontext)
      avcodec_close(m_acontext);

    if(m_vcontext)
      avcodec_close(m_vcontext);

    if(m_ic)
      av_close_input_file(m_ic);

    m_audioBuffer.clear();
    m_audioFrames = 0;

    m_frame = 0;

    m_vcodec = 0;
    m_vindex = -1;
    m_vcontext = 0;

    m_acodec = 0;
    m_aindex = -1;
    m_acontext = 0;

    m_ic = 0;

    return true;
  }

  bool VideoInputFFMPEG::seekPosition(double timeSeconds)
  {
    debug("VideoInputFFMPEG::seekPosition # %lf", timeSeconds);

    if(m_vcontext)
      avcodec_flush_buffers(m_vcontext);

    if(m_acontext)
      avcodec_flush_buffers(m_acontext);

    if(timeSeconds == 0.0) {
      close();
      std::string tmp = m_fileName;
      open(tmp.c_str(), m_flags);
      timeSeconds = 0;
    }
    else {
      int err =
          av_seek_frame(m_ic, -1, (int64_t) (timeSeconds * AV_TIME_BASE), 0);

      if(err != 0) {
        error("VideoInputFFMPEG::seekPosition # Seek failed (%lf)",
              timeSeconds);
        return false;
      }
    }

    m_lastSeek = timeSeconds;
    m_capturedVideo = 0;
    m_capturedAudio = 0;

    m_sinceSeek = 0;

    return true;
  }

  double VideoInputFFMPEG::durationSeconds()
  {
    if(m_flags & Radiant::DO_LOOP)
      return 1.0e+9f;

    if(m_ic && m_vindex >= 0) {
      AVStream * s = m_ic->streams[m_vindex];
      return s->duration * av_q2d(s->time_base);
    }
    return 0.0;
  }

  bool VideoInputFFMPEG::start()
  {
    return ((m_vcodec == 0) ? false : true);
  }

  bool VideoInputFFMPEG::isStarted() const
  {
    return ((m_vcodec == 0) ? false : true);
  }

  bool VideoInputFFMPEG::stop()
  {
    return true;
  }

  void VideoInputFFMPEG::setDebug(int debug)
  {
    m_debug = debug;
  }

}
