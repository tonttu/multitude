/* COPYRIGHT
 */

#include "VideoFFMPEG.hpp"
#include "Screenplay.hpp"

#include <Radiant/Mutex.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/Types.hpp>

#include <string.h>

#include <strings.h>
#include <cassert>

extern "C" {

  typedef uint64_t UINT64_C;
  typedef int64_t INT64_C;

# include <libavformat/avformat.h>
# include <libavcodec/avcodec.h>
# include <libavutil/avutil.h>
}

namespace Screenplay {

  using namespace Radiant;

  // FFMPEG is not thread-safe
  static Radiant::Mutex s_ffmpegMutex(true);

  int VideoInputFFMPEG::m_debug = 0;

  VideoInputFFMPEG::VideoInputFFMPEG()
    : m_acodec(0),
    m_aindex(-1),
    m_acontext(0),
    m_resample_ctx(0),
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
    m_lastPts(0),
    m_mutex(true)
  {
    static volatile bool ffmpegInitialized = false;

    // while used instead of if to break out early
    while(!ffmpegInitialized) {

      Radiant::Guard g(s_ffmpegMutex);

      if(ffmpegInitialized)
        break;

      debugScreenplay("Initializing AVCODEC 1");
      avcodec_init();
      debugScreenplay("Initializing AVCODEC 2");
      avcodec_register_all();
      debugScreenplay("Initializing AVCODEC 3");
      av_register_all();
      debugScreenplay("Initializing AVCODEC 4");
      ffmpegInitialized = true;
    }

    m_lastSeek = 0;
  }

  VideoInputFFMPEG::~VideoInputFFMPEG()
  {
    close();
  }

  const Radiant::VideoImage * VideoInputFFMPEG::captureImage()
  {
    Radiant::Guard g(m_mutex);

    /// @todo this effectively prevents multi-threaded video decoding. Someone
    /// can figure out a fix if this becomes a problem.
    Radiant::Guard g2(s_ffmpegMutex);

    assert(this != 0);

    static const char * fname = "VideoInputFFMPEG::captureImage";

    int got = false;

    int vlen, got_picture;

    while(!got) {

      // printf(","); fflush(0);

      av_free_packet(m_pkt);

      int ret = av_read_frame(m_ic, m_pkt);

      if(ret < 0) {

        /* In the following we do not free the packet, since the read failed. Hope
           this is the correct behavior. */
        debugScreenplay("VideoInputFFMPEG::captureImage ret < 0 %x", m_flags);

        if(! (m_flags & DO_LOOP)) {
          // av_free_packet(m_pkt);
          return 0;
        }
        else {
          debugScreenplay("VideoInputFFMPEG::captureImage # Looping %s", m_fileName.c_str());
          m_offsetTS = m_lastTS;
          // Reset counters
          m_capturedAudio = 0;
          m_capturedVideo = 0;
          av_seek_frame(m_ic, -1, (int64_t) 0, 0);

          // av_free_packet(m_pkt);
          ret = av_read_frame(m_ic, m_pkt);
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

        vlen = avcodec_decode_video2(m_vcontext,
                                     m_frame, & got_picture,
                                     m_pkt);
        // printf("|");fflush(0);

        if (got_picture) {
          got = true;

          int64_t pts = 0;

          if(m_pkt->dts != (int64_t) AV_NOPTS_VALUE) {
            pts = m_pkt->dts;
          } else if(m_pkt->pts != (int64_t) AV_NOPTS_VALUE) {
            pts = m_pkt->pts;
          } else {
            pts = 0;
          }

          m_lastPts = pts;

          /// @todo crashes here once, trying find why
          assert(m_vindex >= 0);

          AVRational time_base = m_ic->streams[m_vindex]->time_base;

          double rate = av_q2d(time_base);
          double secs = pts * rate; // x av_q2d(time_base);

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

          debugScreenplay("VideoInputFFMPEG::captureImage # pts = %d %d %d lts = %lf\n",
                (int) m_frame->pts, (int) m_pkt->pts, (int) m_pkt->dts,
                m_lastTS.secondsD());

          m_lastTS += m_offsetTS;

          if(m_capturedVideo == 0 && m_firstTS == 0)
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

        int aframesOut = 0;

        // decode to audiobuffer if sample rate is 44.1khz
        if(m_audioSampleRate == 44100 || !m_resample_ctx) {


          int aframes = ((int)m_audioBuffer.size() - index) * 2;

          if(aframes < AVCODEC_MAX_AUDIO_FRAME_SIZE) {

            m_audioBuffer.resize(m_audioBuffer.size() + AVCODEC_MAX_AUDIO_FRAME_SIZE * m_audioChannels);
            aframes = ((int) m_audioBuffer.size() - index) * 2;
            if(m_audioBuffer.size() > 1000000) {
              info("VideoInputFFMPEG::captureImage # %p Audio buffer is very large now: %d (%ld)",
                   this, (int) m_audioBuffer.size(), m_capturedVideo);
            }
          }

          avcodec_decode_audio3(m_acontext,
                                & m_audioBuffer[index],
                                & aframes, m_pkt);

          aframes /= (2 * m_audioChannels);
          aframesOut = aframes;

        } else {

          // decode into a temporay audio buffer, later will be resampled
          int aBytesIn = AVCODEC_MAX_AUDIO_FRAME_SIZE * m_audioChannels + FF_INPUT_BUFFER_PADDING_SIZE;   // in bytes
          int aFramesIn = aBytesIn / sizeof(AudioBuffer::value_type);

          m_resampleBuffer.resize(aFramesIn);
          int usedBytes = avcodec_decode_audio3(m_acontext,
                                & m_resampleBuffer[0],
                                & aBytesIn, m_pkt);

          if (m_acodec->id == CODEC_ID_VORBIS)
          {
            // From Vorbis documentation:
            // Data is not returned from the first frame; it must be used to
            // ’prime’ the decode engine. The encoder accounts for this priming
            // when calculating PCM offsets; after the first frame, the proper
            // PCM output offset is ’0’ (as no data has been returned yet).

            if (usedBytes > 0 && aBytesIn == 0)
              continue;
          }

          // Resample
          int srcChannelBytes = aBytesIn / m_audioChannels;
          int srcChannelSamples = srcChannelBytes / sizeof(AudioBuffer::value_type);
          int destChannelBytes = Nimble::Math::Ceil((float)srcChannelBytes * 44100 / m_audioSampleRate);
          int destSamples = (destChannelBytes * m_audioChannels) / sizeof(AudioBuffer::value_type);

          // Check if we have enough space left in the audio buffer
          int freeSamples = ((int) m_audioBuffer.size() - index);                                       // Free space left in audiobuffer
          if(freeSamples < destSamples) {
            m_audioBuffer.resize(index + destSamples);

            if(m_audioBuffer.size() > 1000000) {
              info("VideoInputFFMPEG::captureImage # %p Audio buffer is very large now: %d (%ld)",
                   this, (int) m_audioBuffer.size(), m_capturedVideo);
            }
          }

          int resampled = audio_resample(m_resample_ctx,
                                         & m_audioBuffer[index], // out buf
                                         & m_resampleBuffer[0],  // in buf
                                         srcChannelSamples);     // nb samples per channel

          if(!(resampled > 0))
            error("%s: Failed to resample", fname);

          debugScreenplay("resampled: %d; inrate: %d; outrate: %d", resampled, m_audioSampleRate, 44100);

          aframesOut = resampled;
        }

        if(m_flags & Radiant::MONOPHONIZE_AUDIO) {
          // Force to mono sound.
          for(int a = 0; a < aframesOut; a++) {
            int sum = 0;
            int base = index + a * m_audioChannels;
            for(int chan = 0; chan < m_audioChannels; chan++) {
              sum += m_audioBuffer[base + chan];
            }
            // Scale down to avoid clipping.
            m_audioBuffer[index + a] = sum / m_audioChannels;
          }
        }

        int64_t pts = 0;

        if (m_pkt->dts != (int64_t) AV_NOPTS_VALUE) {
          pts = m_pkt->dts;
        } else if(m_pkt->pts != (int64_t) AV_NOPTS_VALUE) {
          pts = m_pkt->pts;
        } else {
          pts = 0;
        }

        /*AVRational time_base = m_acontext->time_base;

        if(!time_base.num || !time_base.den)
          time_base = m_ic->streams[m_aindex]->time_base;
        if(pts != m_frame->pts && m_ic->streams[m_aindex]->time_base.num)
          time_base = m_ic->streams[m_aindex]->time_base;*/

        AVRational time_base = m_ic->streams[m_aindex]->time_base;

        double rate = av_q2d(time_base);
        double secs = pts * rate;

        debugScreenplay("VideoInputFFMPEG::captureImage # af = %d ab = %d ppts = %d, pdts = %d afr = %d secs = %lf tb = %ld/%ld",
              aframesOut, m_audioFrames, (int) m_pkt->pts, (int) m_pkt->dts,
              (int) m_acontext->frame_number, secs,
              (long) time_base.num, (long) time_base.den);

        if(aframesOut > 10000)
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

        debugScreenplay("Decoding audio # %d %lf", aframesOut, m_audioTS.secondsD());

        m_audioFrames   += aframesOut;
        m_capturedAudio += aframesOut;

        if((uint)(m_audioFrames * m_audioChannels) >= m_audioBuffer.size()) {
          Radiant::error("VideoInputFFMPEG::captureImage # %p Audio trouble %d %d (%ld)",
                         this, aframesOut, m_audioFrames, m_capturedVideo);
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
        debugScreenplay("VideoInputFFMPEG::captureImage # Large audio generated");
        perFrame = 20000;
      }

      debugScreenplay("VideoInputFFMPEG::captureImage # firstTS %lf lastTS %lf; %lf %d %d %d aufr in total %d vidfr",
            m_firstTS.secondsD(), m_lastTS.secondsD(), secs, perFrame, (int) m_audioFrames, (int) m_capturedAudio, (int) m_capturedVideo);

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
        debugScreenplay("%s # PIX_FMT_YUV420P", fname);
    }
    else if(avcfmt == PIX_FMT_YUVJ420P) {
      m_image.setFormatYUV420P();
      if(m_debug && m_capturedVideo < 10)
        debugScreenplay("%s # PIX_FMT_YUV420P", fname);
    }
    else if(avcfmt == PIX_FMT_YUVJ422P) {
      m_image.setFormatYUV422P();
      if(m_debug && m_capturedVideo < 10)
        debugScreenplay("%s # PIX_FMT_YUV422P", fname);
    }
    else if(avcfmt == PIX_FMT_RGB24) {
      m_image.setFormatRGB();
      if(m_debug && m_capturedVideo < 10)
        debugScreenplay("%s # PIX_FMT_RGB24", fname);
    }
    else if(avcfmt == PIX_FMT_BGR24) {
      m_image.setFormatBGR();
      if(m_debug && m_capturedVideo < 10)
        debugScreenplay("%s # PIX_FMT_BGR24", fname);
    }
    else if(avcfmt == PIX_FMT_RGBA) {
      m_image.setFormatRGBA();
      if(m_debug && m_capturedVideo < 10)
        debugScreenplay("%s # PIX_FMT_RGBA", fname);
    }
    else if(avcfmt == PIX_FMT_BGRA) {
      m_image.setFormatBGRA();
      if(m_debug && m_capturedVideo < 10)
        debugScreenplay("%s # PIX_FMT_BGRA", fname);
    }
    else {
      Radiant::error("%s # unsupported FFMPEG pixel format %d", fname, (int) avcfmt);
      av_free_packet(m_pkt);
      return 0;
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

    // can't free here, still need m_data later. m_pkt is freed on each
    // new frame and when video closes
    //av_free_packet(m_pkt);

    return & m_image;
  }

  const void * VideoInputFFMPEG::captureAudio(int * frameCount)
  {
    Radiant::Guard g(m_mutex);

    /* trace2("VideoInputFFMPEG::captureAudio # %d %d",
     * frameCount, m_audioFrames); */

    if(m_audioBuffer.empty()) {
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
                                            Radiant::AudioSampleFormat * format) const
  {
    Radiant::Guard g(m_mutex);

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
    Radiant::Guard g(m_mutex);

    return m_vcontext ? m_vcontext->width : 0;
  }

  int VideoInputFFMPEG::height() const
  {
    Radiant::Guard g(m_mutex);

    return m_vcontext ? m_vcontext->height : 0;
  }

  float VideoInputFFMPEG::fps() const
  {
    Radiant::Guard g(m_mutex);

    if(!m_vcontext)
      return 0;

    double fps = 1/(m_vcontext->ticks_per_frame * av_q2d(m_vcontext->time_base));

    // If we get some wild values use "guess"
    if (fps >= 100) {
      if (!m_ic || m_vindex < 0) {
        warning("VideoInputFFMPEG::fps # Could not get fps");
        return 0;
      }
      assert((int) m_ic->nb_streams > m_vindex && m_vindex >= 0);
      if((int) m_ic->nb_streams <= m_vindex)
        return 0;
      fps = av_q2d(m_ic->streams[m_vindex]->r_frame_rate);
    }

    return float(fps);
  }

  Radiant::ImageFormat VideoInputFFMPEG::imageFormat() const
  {
    Radiant::Guard g(m_mutex);
    return m_image.m_format;
  }

  unsigned int VideoInputFFMPEG::size() const
  {
    Radiant::Guard g(m_mutex);
    return m_image.size();
  }

  bool VideoInputFFMPEG::open(const char * filename,
                              int flags)
  {
    Radiant::Guard g(m_mutex);

    if(m_vcodec)
      close();

    Radiant::Guard g2(s_ffmpegMutex);

    if(!m_pkt) {
      m_pkt = new AVPacket();
      av_init_packet(m_pkt);
    }

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

    int err = av_open_input_file( & m_ic, filename, iformat, 0, ap);

    if(err < 0) {
      error("%s # Could not open file \"%s\" %s",
            fname, filename, strerror(-err));
      return false;
    }

    /// @todo workaround for libav matroska seek bug (2256)
    err = av_find_stream_info(m_ic);
    if(err < 0) {
      error("%s # Could not find stream info for %s", fname, filename);
    }

    /* trace2("%s # Opened %s with %d Hx, %d channels, %dx%d",
       fname, filename, ap->sample_rate, ap->channels,
       ap->width, ap->height); */

    av_read_play(m_ic);

    for(i = 0; i < (int) m_ic->nb_streams; i++) {

      AVCodecContext *enc = m_ic->streams[i]->codec;

      if(enc->codec_type == AVMEDIA_TYPE_VIDEO) {

        m_vindex = i;
        m_vcodec = avcodec_find_decoder(enc->codec_id);
        m_vcontext = enc;

        int64_t start_time = m_ic->streams[i]->start_time;
        if (start_time != (int64_t) AV_NOPTS_VALUE) {
          debugScreenplay("%s # Stream %d does not contain start time.", fname, i);
          m_firstTS = Radiant::TimeStamp::createSecondsD(start_time * av_q2d(m_ic->streams[i]->time_base));
        }

        AVRational fr = m_ic->streams[i]->r_frame_rate;

        debugScreenplay("%s # Got frame rate of %d %d", fname, fr.num, fr.den);

        /* if(m_vcodec->supported_framerates) {
           for(int k = 0; m_vcodec->supported_framerates[k].num != 0; k++) {
           fr = m_vcodec->supported_framerates[k];
           trace2("%s # Supported frame rate of %d %d",
           fname, fr.num, fr.den);
           }
           }*/

        if(!m_vcodec || avcodec_open(enc, m_vcodec) < 0) {
          // Unsupported video codec
          Radiant::warning("VideoInputFFMPEG::open # unsupported video codec.");
          return false;
        } else if(flags & WITH_VIDEO)
          m_flags = m_flags | WITH_VIDEO;
      }
      else if(enc->codec_type == AVMEDIA_TYPE_AUDIO) {

        m_aindex = i;
        m_acodec = avcodec_find_decoder(enc->codec_id);
        m_acontext = enc;

        if((!m_acodec || avcodec_open(enc, m_acodec) < 0) && (flags & Radiant::WITH_AUDIO)) {
          // Unsupported audio codec
          Radiant::warning("VideoInputFFMPEG::open # unsupported audio codec. Trying to decode without audio stream...");
          m_flags &= ~WITH_AUDIO;
        } else if(flags & WITH_AUDIO)
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

    if(m_audioSampleRate != 44100) {
      // resample to 44100HZ
      m_resample_ctx = av_audio_resample_init(m_audioChannels,  // nb output channels
                                              m_audioChannels,  // nb input channels
                                              44100,             // out rate
                                              m_audioSampleRate, // in rate
                                              SAMPLE_FMT_S16,
                                              SAMPLE_FMT_S16,
                                              16,                // filter length
                                              10,                // phase count
                                              0,                 // linear FIR filter
                                              0.8);              // cutoff frequency


      if(!m_resample_ctx)
        error("%s: Failed to create resampling context", fname);
    }

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
      debugScreenplay("%s # File %s has unsupported audio codec.", fname, filename);
      // return false;
    }

    debugScreenplay("%s # Opened file %s,  (%d x %d %s, %s %d chans @ %d Hz) %d (%d, %f)",
          fname, filename, width(), height(), vcname, acname, m_audioChannels,
          m_audioSampleRate,
          (int) m_image.m_format, (int) m_vcontext->pix_fmt, ratio);

    return true;
  }

  bool VideoInputFFMPEG::close()
  {
    Radiant::Guard g(m_mutex);
    //    if(!m_ic)
    //      return false;

    Guard g2( s_ffmpegMutex);

    if(m_frame)
      av_free(m_frame);

    if(m_acontext)
      avcodec_close(m_acontext);

    if(m_vcontext)
      avcodec_close(m_vcontext);

    if(m_ic)
      av_close_input_file(m_ic);

    if(m_pkt) {
      av_free_packet(m_pkt);
      delete m_pkt;
      m_pkt = 0;
    }

    if(m_resample_ctx) {
      audio_resample_close(m_resample_ctx);
      m_resample_ctx = 0;
    }

    // Cleanup audio buffers
    AudioBuffer tmp, tmp2;
    m_audioBuffer.swap(tmp);
    m_resampleBuffer.swap(tmp2);

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
    Radiant::Guard g(m_mutex);
    Radiant::Guard g2(s_ffmpegMutex);

    debugScreenplay("VideoInputFFMPEG::seekPosition # %lf", timeSeconds);

    if(m_vcontext)
      avcodec_flush_buffers(m_vcontext);

    if(m_acontext)
      avcodec_flush_buffers(m_acontext);

    if(timeSeconds <= 1e-10) {
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

  double VideoInputFFMPEG::durationSeconds() const
  {
    Radiant::Guard g(m_mutex);

    if(m_ic && m_vindex >= 0) {
      AVStream * s = m_ic->streams[m_vindex];

      if (s->duration != (int64_t) AV_NOPTS_VALUE) {
        return s->duration * av_q2d(s->time_base);
      } else if (m_ic->duration != (int64_t) AV_NOPTS_VALUE) {
        // If video stream doesn't have duration, check the container for valid duration info.
        // Could also iterate over all other streams as well.
        debugScreenplay("VideoInputFFMPEG::durationSeconds # Could not get video stream duration. Using container duration.");
        AVRational av_time_base = {1, AV_TIME_BASE};
        return m_ic->duration * av_q2d(av_time_base);
      }
    }

    return 0.0;
  }

  double VideoInputFFMPEG::runtimeSeconds() const
  {
    if(m_flags & Radiant::DO_LOOP)
      return 1.0e+9f;

    return durationSeconds();
  }

  bool VideoInputFFMPEG::start()
  {
    Radiant::Guard g(m_mutex);
    return ((m_vcodec == 0) ? false : true);
  }

  bool VideoInputFFMPEG::isStarted() const
  {
    Radiant::Guard g(m_mutex);
    return ((m_vcodec == 0) ? false : true);
  }

  bool VideoInputFFMPEG::stop()
  {
    Radiant::Guard g(m_mutex);
    return true;
  }

  void VideoInputFFMPEG::setDebug(int debug)
  {
    m_debug = debug;
  }

}
