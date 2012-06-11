#ifndef VIDEODISPLAY_AUDIO_TRANSFER2_HPP
#define VIDEODISPLAY_AUDIO_TRANSFER2_HPP

#include "AVDecoder.hpp"

#include <Radiant/Allocators.hpp>
#include <Radiant/IODefs.hpp>

#include <Resonant/Module.hpp>

namespace VideoPlayer2
{
  class DecodedAudioBuffer
  {
  public:
    DecodedAudioBuffer() : m_timestamp(), m_offset(0) {}

    void fill(Timestamp timestamp, int channels, int samples, const int16_t * src);
    void fillPlanar(Timestamp timestamp, int channels, int samples, const float ** src);

    Timestamp timestamp() const { return m_timestamp; }

    int offset() const { return m_offset; }
    void setOffset(int offset) { m_offset = offset; }

    const float * data(unsigned channel) const
    {
      if(channel >= m_data.size()) return 0;
      return m_data[channel].data();
    }

    int samples() const { return m_data.empty() ? 0 : m_data[0].size(); }

  private:
    //DecodedAudioBuffer(const DecodedAudioBuffer &);
    //DecodedAudioBuffer & operator=(DecodedAudioBuffer &);
  private:
    Timestamp m_timestamp;
    int m_offset;

    // vector of channels
    typedef std::vector<float, Radiant::aligned_allocator<float, 32>> AlignedFloatVector;
    std::vector<AlignedFloatVector> m_data;
  };

  class AudioTransfer : public Resonant::Module
  {
  public:
    AudioTransfer(int channels, int & seekGeneration, AVDecoder::PlayMode & playMode);
    virtual ~AudioTransfer();

    virtual bool prepare(int & channelsIn, int & channelsOut);
    virtual void process(float ** in, float ** out, int n, const Resonant::CallbackTime & time);

    Timestamp toPts(const Radiant::TimeStamp & ts) const;

    Timestamp lastPts() const;

    bool isBufferEmpty() const;

    DecodedAudioBuffer * takeFreeBuffer(int samples);
    void putReadyBuffer(int samples);

  private:
    class D;
    D * m_d;
  };
}

#endif // VIDEODISPLAY_AUDIO_TRANSFER2_HPP
