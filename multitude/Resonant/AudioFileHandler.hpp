/* COPYRIGHT
 *
 * This file is part of Resonant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Resonant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RESONANT_AUDIO_FILE_HANDLER_HPP
#define RESONANT_AUDIO_FILE_HANDLER_HPP

#include <Radiant/Condition.hpp>
#include <Radiant/Config.hpp>
#include <Radiant/IODefs.hpp>
#include <Radiant/Thread.hpp>

#include <list>
#include <string>
#include <vector>

struct SNDFILE_tag;
struct SF_INFO;

namespace Resonant {

  /** Read/write multiple audio files at the same time in one
      background-thread. */

  class AudioFileHandler : public Radiant::Thread
  {
    friend class Handle;

  public:

    enum OpenStatus {
      OPEN_NOT,
      OPEN_DONE,
      OPEN_FAILED,
      OPEN_EOF,
      OPEN_CLOSED
    };

    /** A handle that offers access to the audio files. */
    class Handle
    {
    public:
      friend class AudioFileHandler;

      bool waitOpen();
      bool isOpen() const { return m_file != 0; }

      int writeFrames(float * data, int frames);
      int readFrames(float * data, int frames);

      int writeFrames(int * data, int frames) 
      { return writeFrames((float *) data, frames); }
      int readFrames(int * data, int frames)
      { return readFrames((float *) data, frames); }

      int channels()   const;
      int sampleRate() const;
      long frames()    const;
      long currentFrame() const { return m_userFrames; }

      void rewind(long frame);
      bool isReady();

    private:

      void lock()   { m_host->lock(); }
      void unlock() { m_host->unlock(); }
      void waitCond()   { m_host->waitCond(); }
      void signalCond() { m_host->signalCond(); }

      Handle(AudioFileHandler * host,
	     const char * filename, Radiant::IoMode mode, long startFrame, 
	     Radiant::AudioSampleFormat userFormat);
      ~Handle();

      bool update();

      bool open();
      bool close();

      bool fileRead();
      bool fileWrite();
      bool flushWrite();
      bool moveReadHead(long frame, bool clear = true);

      float * ptr(long frame)
      { return & m_data[(frame * channels()) % m_data.size()]; }

      AudioFileHandler * m_host;

      std::string m_fileName;
      Radiant::IoMode   m_ioMode;

      long        m_blocks;
      long        m_blockSize;
      long        m_startFrame;
      long        m_rewindTo;

      volatile OpenStatus m_status;
      volatile long       m_fileFrames;
      volatile long       m_userFrames;
      volatile bool       m_ready;

      SNDFILE_tag * m_file;
      SF_INFO * m_info;
      Radiant::AudioSampleFormat m_userFormat;
      std::vector<float>   m_data;
      bool m_userDone;
    };


    AudioFileHandler();
    ~AudioFileHandler();

    static AudioFileHandler * instance() { return m_instance; }

    Handle * readFile(const char * filename, long startFrame, 
		      Radiant::AudioSampleFormat userFormat = Radiant::ASF_FLOAT32);
    Handle * writeFile(const char * filename, 
		       int channels, 
		       int samplerate, 
		       int sfFormat,
		       // Either ASF_FLOAT32 or ASF_INT32
		       Radiant::AudioSampleFormat userFormat = Radiant::ASF_FLOAT32);

    void done(Handle *);

    void start();
    void stop();

    static bool getInfo(const char * filename, SF_INFO * info);

  private:

    virtual void childLoop();
  
    bool update();

    void lock()   { m_mutex.lock(); }
    void unlock() { m_mutex.unlock(); }
    void waitCond()   { m_cond.wait(m_mutex); }
    void signalCond() { m_cond.wakeOne(m_mutex); }

    typedef std::list<Handle *> container;
    typedef container::iterator iterator;

    container m_files;
  
    Radiant::MutexAuto m_mutex, m_mutex2;
    Radiant::Condition m_cond;

    volatile bool m_done;

    static AudioFileHandler * m_instance;
  };

}

#endif
