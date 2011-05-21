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

#ifndef SCREENPLAY_VIDEO_FFMPEG_HPP
#define SCREENPLAY_VIDEO_FFMPEG_HPP

#include <Radiant/TimeStamp.hpp>
#include <Radiant/VideoInput.hpp>

#include <Screenplay/Export.hpp>

#include <QString>

extern "C" {

  struct AVCodec;
  struct AVCodecContext;
  struct AVFormatContext;
  struct AVFrame;
  struct AVPacket;
}

namespace Screenplay {

  /** Video decoder based on the FFMPEG library. */
  class SCREENPLAY_API VideoInputFFMPEG : public Radiant::VideoInput
  {
  public:

    VideoInputFFMPEG();
    virtual ~VideoInputFFMPEG();

    /// Reads and decodes the current frame from the video
    virtual const Radiant::VideoImage * captureImage();
    /// The time-stamp of the latest video frame
    Radiant::TimeStamp frameTime() { return m_lastTS; }
    /// The time stamp of curren audio buffer
    /** This timestamp is timed to match the beginning of the current
    audio buffer. This is absolute time within the time-system of
    the video file.  */
    Radiant::TimeStamp audioTime() const { return m_audioTS; }

    /** This function does not actually decode anything, it just returns data
      decoded in the "captureFrame". */
    virtual const void * captureAudio(int * frameCount);
    /** Get audio parameters from the video */
    virtual void getAudioParameters(int * channels,
                    int * sample_rate,
                    Radiant::AudioSampleFormat * format);

    /// The width of the video stream images.
    virtual int width() const;
    /// The height of the video stream images.
    virtual int height() const;
    /// The frame rate of the video stream.
    virtual float fps() const;
    /// Native image format of the stream.
    virtual Radiant::ImageFormat imageFormat() const;
    /// Returns the dimensions of a single frame
    virtual unsigned int size() const;

    /// Opens a video file
    bool open(const char * filename,
          int flags = Radiant::WITH_VIDEO);

    virtual bool start();
    /// Check if the video has been started
    virtual bool isStarted() const;
    virtual bool stop();
    virtual bool close();

    // void configure(const char * filename, const char * codecname = 0);

    /// Returns the total number of decoded audio frames
    long capturedAudio() { return m_capturedAudio; }
    /// Returns the total number of decoded video frames
    long capturedVideo() { return m_capturedVideo; }

    /// Seeks to a given position in the file
    /** Seek operation may not be entirely accurate, since the encoding strategy
        of the file may prevent proper seeking. */
    bool seekPosition(double timeSeconds);
    /// The duration of the video in seconds
    double durationSeconds();

    /// Returns true if there is an audio codec for this video
    bool hasAudioCodec() const { return m_acodec != 0; }
    /// Returns true if there is a video codec for this video
    bool hasVideoCodec() const { return m_vcodec != 0; }

    /// Returns the audio sampling rate of this video, if known.
    int audioSampleRate() const { return m_audioSampleRate; }

    /*
    void enableLooping(bool enable)
    {
      if(enable)
    m_flags |= Radiant::DO_LOOP;
      else
    m_flags &= ~Radiant::DO_LOOP;
    }
    */
    /// Turn on/off the printing of debug messages
    static void setDebug(int debug);

  private:
    int actualChannels() const
    {
      return (m_flags & Radiant::MONOPHONIZE_AUDIO) ?
          1 : m_audioChannels;
    }

    QString      m_fileName;
    QString      m_codecName;

    AVCodec        * m_acodec;
    int              m_aindex; // Audio index
    AVCodecContext * m_acontext;
    std::vector<int16_t> m_audioBuffer;
    int                  m_audioFrames;
    int                  m_audioChannels;
    int                  m_audioSampleRate;

    long m_capturedAudio;
    long m_capturedVideo;

    AVCodec        * m_vcodec;
    int              m_vindex; // Video index
    AVCodecContext * m_vcontext;
    AVFrame        * m_frame;

    AVFormatContext *m_ic;

    AVPacket       * m_pkt;

    enum { BUFFER_SIZE = 4096 };

    Radiant::VideoImage m_image;

    int              m_flags;
    int64_t          m_lastPts;
    Radiant::TimeStamp  m_audioTS;
    Radiant::TimeStamp  m_lastTS;
    Radiant::TimeStamp  m_firstTS;
    double           m_lastSeek;

    int              m_sinceSeek;
    Radiant::TimeStamp  m_offsetTS;

    static int       m_debug;
  };

}

#endif
