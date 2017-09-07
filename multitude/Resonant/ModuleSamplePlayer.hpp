/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RESONANT_MODULE_SAMPLE_PLAYER_HPP
#define RESONANT_MODULE_SAMPLE_PLAYER_HPP

#include <memory>

#include <Nimble/Ramp.hpp>

#include <Radiant/Thread.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/Condition.hpp>

#include <Resonant/Module.hpp>

#include <list>
#include <QString>
#include <vector>

namespace Resonant {

  class DSPNetwork;

  /** Audio sample player for Resonant.

      This class implements a basic sample player, which can be used
      as a minimal synthesizer.

      <B>Note management:</B> Playing samples become notes that can be controlled by the user.
      If you play the same sample many times, each note is treated individually.
      For this purpose the functions that start sample playback return NoteInfo objects that can
      be used to query the state of the note.

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

    /// Possible states of the individual notes
    /** Each note has its own id.*/
    enum NoteStatus {
      /// Note is playing
      NOTE_PLAYING,
      /// Note is finished
      NOTE_FINISHED
    };

  private:
    /* This private block is here since the class needs to be hidden, but it needs the above enum. */
    class RESONANT_API NoteInfoInternal
    {
    public:

      NoteInfoInternal();

      NoteStatus m_status;
      int m_noteId;
      float m_sampleLengthSeconds;
      float m_playHeadPosition;
    };

    typedef std::shared_ptr<NoteInfoInternal> NoteInfoInternalPtr;

  public:

    /// Note information container
    /** This class can be used to track the playback of individual notes. Instances of this class
        can be copied freely.
    */
    class RESONANT_API NoteInfo
    {
    public:
      NoteInfo();
      ~NoteInfo();

      /// Returns the current status of the note
      NoteStatus status() const;
      /// Returns the identified of the note
      int noteId() const;

      /// Returns true if the note is playing
      bool isPlaying() const { return status() == NOTE_PLAYING; }

      /// Returns the length of the sample in seconds
      float sampleLengthSeconds() const;
      /// Returns the current playhead location in seconds
      float playHeadSeconds() const;

    private:

      friend class ModuleSamplePlayer;
      void init(int id);

      NoteInfoInternalPtr m_info;
    };

    /// Parameters for note-on (aka play-sample) events
    /** This class is used to pass parameters to the sample player. */
    class RESONANT_API NoteParameters
    {
    public:
      NoteParameters(const QString & filename = QString::null)
        : m_fileName(filename)
        , m_gain(1.0f)
        , m_relativePitch(1.0f)
        , m_targetChannel(0)
        , m_sampleChannel(0)
        , m_loop(false)
        , m_playbackTime(0)
        , m_samplePlayhead(0)
      {}

      /// Returns the name of the file to be played
      QString fileName() const;
      /// Sets the name of the file to be played
      void setFileName(const QString &fileName);

      /// The gain coefficient for playback. Setting gain to one
      /// plays the file back at the original volume.
      float gain() const;
      /// Sets the gain of this note
      void setGain(float gain);

      /** The pitch for the playback. If the pitch is set to one,
      then the file will play back at the original speed. With pitch of 0.5
      the file will play back one octave below original pitch, and last
      twice as long as nominal file duration.
      */
      float relativePitch() const;
      /// Sets the relative pitch of this note
      void setRelativePitch(float relativePitch);

      /** Select the channel where the sound is going to.
          For example in 8-channel environment, this parameter can range from zero
          to seven.
      */
      int targetChannel() const;
      /// Sets the target playback channel
      void setTargetChannel(int targetChannel);

      /// Returns the channel of the source file that should be used as the source.
      /// Value -1 means downmixing all channels to one mono output.
      int sampleChannel() const;
      /// Sets the channel of the source file that should be used as the source.
      void setSampleChannel(int sampleChannel);

      /// Turns on looping if necessary. With looping the sample will play
      /// back for-ever.
      bool loop() const;
      /// Set the looping mode
      void setLoop(bool loop);

      /// Returns the timestamp when to play the sample.
      Radiant::TimeStamp playbackTime() const;
      /// Sets the timestamp when to play the sample.
      void setPlaybackTime(const Radiant::TimeStamp &playbackTime);

      /// Returns the time of sample when the playback should begin
      float samplePlayhead() const;
      /// Sets the time when the playback should begin.
      /// @param samplePlayhead The time in seconds
      void setSamplePlayhead(float samplePlayhead);

    private:

      QString m_fileName;
      float m_gain;
      float m_relativePitch;
      int   m_targetChannel;
      int   m_sampleChannel;
      bool  m_loop;
      Radiant::TimeStamp m_playbackTime;
      float m_samplePlayhead;
    };

    /// Audio sample player module
    ModuleSamplePlayer();
    /// Delete the sample player
    virtual ~ModuleSamplePlayer();

    virtual bool prepare(int & channelsIn, int & channelsOut) OVERRIDE;
    virtual void eventProcess(const QByteArray & address, Radiant::BinaryData &) OVERRIDE;
    virtual void process(float ** in, float ** out, int n, const Resonant::CallbackTime &) OVERRIDE;
    virtual bool stop() OVERRIDE;


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
        source. Value -1 means downmixing all channels to one mono output.

        @param loop Turns on looping if necessary. With looping the sample will play
        back for-ever.

        @param time optional timestamp when to play the sample.

        @return Returns a handle to the note playback information.
    */
    NoteInfo playSample(const char * filename,
                        float gain,
                        float relpitch,
                        int targetChannel,
                        int sampleChannel,
                        bool loop = false,
                        Radiant::TimeStamp time = Radiant::TimeStamp(0));

    /// Plays an audio sample
    /** This function starts the playback of an audio sample.

        @param parameters An object containing all the parameters for the playback of the sample
      */
    NoteInfo playSample(const NoteParameters & parameters);

    /// Plays an audio sample
    /** This function starts the playback of an audio sample.

        @param filename The name of the audio sample file.

        @param gain The gain coefficient for playback. Setting gain to one
        plays the file back at the original volume.

        @param relpitch The pitch for the playback. If the pitch is set to one,
        then the file will play back at the original speed. With pitch of 0.5
        the file will play back one octave below original pitch, and last
        twice as long as nominal file duration.

        @param location The screen location where this sample should be located. Currently
        the location is used so that the sample player tries to find an audio panning module and
        if the panner is present uses that to convert the location to one
        specific audio out channel. The sound is then played out on that channel.

        @param sampleChannel Select the channel of the source file that should be used as the
        source. Value -1 means downmixing all channels to one mono output.

        @param loop Turns on looping if necessary. With looping the sample will play
        back for-ever.

        @param time optional timestamp when to play the sample.

        @return Returns a handle to the note playback information.
    */
    NoteInfo playSampleAtLocation(const char * filename,
                                  float gain,
                                  float relpitch,
                                  Nimble::Vector2 location,
                                  int sampleChannel,
                                  bool loop = false,
                                  Radiant::TimeStamp time = Radiant::TimeStamp(0));

    /// Stops the playback of a given sample
    void stopSample(int noteId);

    void stopSample(const NoteInfo & info) { stopSample(info.noteId()); }
    void setSampleGain(const NoteInfo & info, float gain, float interpolationTimeSeconds = 0.02f);
    void setSampleRelativePitch(const NoteInfo & info, float relativePitch, float interpolationTimeSeconds = 0.02f);
    void setSamplePlayHead(const NoteInfo & info, float playHeadTimeSeconds, float interpolationTimeSeconds = 0.02f);
    void setSampleLooping(const NoteInfo & info, bool looping);

    /** Sets the master gain */
    void setMasterGain(float gain) { m_masterGain = gain; }

    /// Number of output channels
    /// @return Number of output channels
    size_t channels() const { return m_channels; }

    /// Current playback time
    /// @return Current playback time
    const Radiant::TimeStamp & time() { return m_time; }

    int locationToChannel(Nimble::Vector2 location);

  private:

    bool addSample(const char * filename, const char * name);

    int findFreeVoice();
    int findSample(const char * );

    void loadSamples();
    void stopSampleInternal(Radiant::BinaryData & data);
    void controlSample(int voiceId, const QByteArray & parameter, Radiant::BinaryData & data);

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
          m_dpos(0), m_noteId(0), m_finishCounter(-1),
          m_sampleChannel(0), m_targetChannel(0),
          m_sample(s), m_position(0), m_startPosition(0), m_startFadeInDurationSamples(0), m_startGain(1.0f),
          m_autoRestartAfterStop(false),
          m_stopped(false)
      {}

      bool synthesize(float ** out, int n, ModuleSamplePlayer *);

      void init(ModuleSamplePlayer *, std::shared_ptr<Sample> sample, Radiant::BinaryData & data);
      void processMessage(ModuleSamplePlayer * host, const QByteArray &parameter, Radiant::BinaryData &data);

      bool isActive() { return m_state != INACTIVE; }

      void loadFailed() { m_state = INACTIVE; }

      void setSample(std::shared_ptr<Sample> s);

      void clear() { m_state = INACTIVE; m_sample.reset(); }

      void stop(float fadeTime = 0.02f, float sampleRate = 44100.0f);

      int noteId() const { return m_noteId; }

      static void scanDataToEnd(Radiant::BinaryData &data);

      NoteInfoInternalPtr info() { return m_info; }
    private:
      enum State {
        INACTIVE,
        WAITING_FOR_SAMPLE,
        PLAYING
      };

      State m_state;

      Nimble::Rampd m_gain;
      Nimble::Rampd m_relPitch;
      double m_dpos;
      int m_noteId;
      int m_finishCounter;

      // -1 means downmix all channels to mono
      size_t m_sampleChannel;
      size_t m_targetChannel;
      bool     m_loop;
      std::shared_ptr<Sample> m_sample;
      unsigned m_position;
      // Start/restart position
      unsigned m_startPosition;
      unsigned m_startFadeInDurationSamples;
      // Gain to be used when restarting the sample
      float    m_startGain;
      bool m_autoRestartAfterStop;

      Radiant::TimeStamp m_startTime;
      NoteInfoInternalPtr m_info;

      bool m_stopped;
    };

    /* Loads samples from the disk, as necessary. */
    class LoadItem
    {
    public:
      enum { WAITING_COUNT = 64 };

      LoadItem();

      void init(const char * filename, SampleVoice * waiting)
      {
        memset(m_waiting, 0, sizeof(m_waiting));
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

    SampleVoice * findVoiceForNoteId(int noteId);

    std::list<SampleInfo> m_sampleList;

    std::vector<std::shared_ptr<Sample> > m_samples;

    std::vector<SampleVoice> m_voices;
    std::vector<SampleVoice *> m_voiceptrs;
    std::map<int, NoteInfoInternalPtr> m_infos;

    size_t m_channels;
    size_t m_active;

    float    m_masterGain;
    Radiant::TimeStamp m_time;

    BGLoader * m_loader;

    int m_userNoteIdCounter;

    Radiant::Mutex m_mutex;
  };

}

#endif
