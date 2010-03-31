/* COPYRIGHT
 *
 * This file is part of VideoDisplay.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "VideoDisplay.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */


#ifndef VIDEODISPLAY_VIDEO_IN_FFMPEG_HPP
#define VIDEODISPLAY_VIDEO_IN_FFMPEG_HPP

#include <VideoDisplay/VideoIn.hpp>

#include <Screenplay/VideoFFMPEG.hpp>

namespace VideoDisplay {

  /// Movie file decoder, that uses the Screenplay::VideoInputFFMPEG
  class VideoInFFMPEG : public VideoIn
  {
  public:
    VideoInFFMPEG();
    virtual ~VideoInFFMPEG();

    /// Gets the audio parameters of the movie
    virtual void getAudioParameters(int * channels,
                    int * sample_rate,
                    Radiant::AudioSampleFormat * format);
    /// Returns the nominal fps of the movie
    virtual float fps();

    /// Returns the total length of the movie, in seconds
    virtual double durationSeconds();

    /// Seek to some time in the movie
    // virtual bool seekTo(double seconds);

  private:

    virtual bool open(const char * filename, Radiant::TimeStamp pos);

    virtual void videoGetSnapshot(Radiant::TimeStamp pos);
    virtual void videoPlay(Radiant::TimeStamp pos);
    virtual void videoGetNextFrame();
    virtual void videoStop();

    void doSeek(const Radiant::VideoImage * im, const void * audio,
        int audioframes);
    // void needResync();
    // void doSync(int aframes, Radiant::TimeStamp vt);
    void endOfFile();

    int m_needSync;
    int m_gotSync;
    Radiant::TimeStamp m_syncAccu;
    Radiant::TimeStamp m_syncOffset;

    Radiant::TimeStamp m_duration;
    double m_frameDelta;

    Screenplay::VideoInputFFMPEG m_video;

    std::vector<float> m_audiobuf;
    int m_buffered;

    int m_channels;
    int m_sampleRate;
    int m_audioCount;
    Radiant::AudioSampleFormat m_auformat;
  };

}

#endif
