/* COPYRIGHT
 */

#ifndef RESONANT_MODULE_SAMPLE_PLAYER_HPP
#define RESONANT_MODULE_SAMPLE_PLAYER_HPP

#include <Radiant/RefPtr.hpp>
#include <Radiant/Thread.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/Condition.hpp>

#include <Resonant/Module.hpp>

#include <list>
#include <QString>
#include <vector>

#include <strings.h>

namespace Resonant {

  class DSPNetwork;

  /** Audio sample player for Resonant.

      This class implements a basic sample player, which can be used
      as a minimal synthesizer.


      <B>Memory management:</B> The samples (aka audio files)
      are read from the disk as they are needed. The samples are loaded when they
      are first used. To force the loading of a particular sample, you can
      play the sample with zero volume. Samples are never dropped away from the
      RAM which may cause memory usage issues if the application is using a lot
      sound files.
  */
  class RESONANT_API ModuleSamplePlayer : public Module
  {
  public:

    /// Audio sample player module
    ModuleSamplePlayer(Application *);
    /// Delete the sample player
    virtual ~ModuleSamplePlayer();

    virtual bool prepare(int & channelsIn, int & channelsOut);
    virtual void processMessage(const QString & address, Radiant::BinaryData &) OVERRIDE;
    virtual void process(float ** in, float ** out, int n);

    /** Adds a few voices that will play an ambient sound background.
        All files in the given directory are loaded looped
        for-ever. In practice one wants to put 3-5 audio files with
        different lengths in the directory. The length of the files
        should be usually in the 20-30 second range. The end result will be a
        nice ambient background that does not sound like it is
        looping.

        The ambient sounds are replicated accross given number of channels, with channel rotation
        to ensure that each loudspeaker will generate slightly different
        sounds.

        @param directory Where the files are loaded from.

        @param gain The gain (volume) to give to the background material.

        @param fillchannels The number of output channels to fill. This number is limited
        by the active channel number.

        @param delay Delay time to wait before starting the playback. Short delay times
        may lead to poor synchronization between different channels.
    */

    void createAmbientBackground(const char * directory, float gain, int fillchannels = 1000,
                                 float delay = 7.0f);

    /// Plays an audio sample
    /** This function starts the playback of an audio sample.

        @param filename The name of the audio sample file.

        @param gain The gain coefficient for playback. Setting gain to one
        plays the file back at the original volume.

        @param relpitch The pitch for the playback. If the pitch is set to one,
        then the file will play back at the original speed. With pitch of 0.5
        the file will play back one octave below original pitch, and last
        twice as long as nominal file duration.

        @param targetChannel Select the channel where the sound is going to.
        For example in 8-channel environment, this parameter can range from zero
        to seven.

        @param sampleChannel Select the channel of the source file that should be used as the
        source.

        @param loop Turns on looping if necessary. With looping the sample will play
        back for-ever.

        @param time optional timestamp when to play the sample
    */
    void playSample(const char * filename,
                    float gain,
                    float relpitch,
                    int targetChannel,
                    int sampleChannel,
                    bool loop = false,
                    Radiant::TimeStamp time = 0);

    /** Sets the master gain */
    void setMasterGain(float gain) { m_masterGain = gain; }

    /// Number of output channels
    /// @return Number of output channels
    size_t channels() const { return m_channels; }

    /// Current playback time
    /// @return Current playback time
    const Radiant::TimeStamp & time() { return m_time; }

  private:

    bool addSample(const char * filename, const char * name);

    int findFreeVoice();
    int findSample(const char * );

    void loadSamples();

    class SampleInfo
    {
    public:
      QString m_name;
      QString m_filename;
    };

    /* This class holds audio sample data in RAM. */
    class Sample : public Patterns::NotCopyable
    {
    public:
      Sample();
      ~Sample();

      bool load(const char * filename, const char * name);

      inline const std::vector<float> & data() const { return m_data; }
      const float * buf(unsigned i) const;

      const QString & name() const { return m_name; }
      /** Number of samples available */
      unsigned available(unsigned pos) const;
      unsigned channels() const;
      unsigned frames() const;

    private:
      class Internal;

      Internal * m_d;

      std::vector<float> m_data;

      QString m_name;
    };

    /* This class controls the playback of a sample. */
    class SampleVoice
    {
    public:
      SampleVoice(Sample * s = 0)
        : m_state(INACTIVE), m_gain(1), m_relPitch(1.0f),
          m_sampleChannel(0), m_targetChannel(0),
          m_sample(s), m_position(0)
      {}

      bool synthesize(float ** out, int n, ModuleSamplePlayer *);

      void init(std::shared_ptr<Sample> sample, Radiant::BinaryData & data);

      bool isActive() { return m_state != INACTIVE; }

      void loadFailed() { m_state = INACTIVE; }

      void setSample(std::shared_ptr<Sample> s);

      void clear() { m_state = INACTIVE; m_sample.reset(); }


    private:
      enum State {
        INACTIVE,
        WAITING_FOR_SAMPLE,
        PLAYING
      };

      State m_state;

      float m_gain;
      float m_relPitch;
      double m_dpos;

      size_t m_sampleChannel;
      size_t m_targetChannel;
      bool     m_loop;
      std::shared_ptr<Sample> m_sample;
      size_t m_position;
      Radiant::TimeStamp m_startTime;
    };

    /* Loads samples from the disk, as necessary. */
    class LoadItem
    {
    public:
      enum { WAITING_COUNT = 64 };

      LoadItem();

      void init(const char * filename, SampleVoice * waiting)
      {
        bzero(m_waiting, sizeof(m_waiting));
        m_waiting[0] = waiting;
        m_name = filename;
        m_free = false;
      }

      inline bool addWaiting(SampleVoice * voice)
      {
        for(int i = 0; i < WAITING_COUNT; i++) {
          if(m_waiting[i] == 0) {
            m_waiting[i] = voice;
            return true;
          }
        }

        return false;
      }

      bool m_free;
      std::string m_name;

      SampleVoice * m_waiting[WAITING_COUNT];
    };

    /* Private sample loader thread. */
    class BGLoader : public Radiant::Thread
    {
    public:

      BGLoader(ModuleSamplePlayer * host);
      ~BGLoader();

      bool addLoadable(const char * filename, SampleVoice * waiting);

    private:

      virtual void childLoop();

      enum { BINS = 256};

      Radiant::Condition m_cond;
      Radiant::Mutex m_mutex;

      LoadItem m_loads[BINS];

      ModuleSamplePlayer * m_host;

      volatile bool m_continue;
    };

    bool addSample(std::shared_ptr<Sample> s);

    void dropVoice(size_t index);

    std::list<SampleInfo> m_sampleList;

    std::vector<std::shared_ptr<Sample> > m_samples;

    std::vector<SampleVoice> m_voices;
    std::vector<SampleVoice *> m_voiceptrs;

    size_t m_channels;
    size_t m_active;

    float    m_masterGain;
    Radiant::TimeStamp m_time;

    BGLoader * m_loader;
  };

}

#endif
