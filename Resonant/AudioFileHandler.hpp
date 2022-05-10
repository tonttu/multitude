/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RESONANT_AUDIO_FILE_HANDLER_HPP
#define RESONANT_AUDIO_FILE_HANDLER_HPP

#include "Export.hpp"

#include <Radiant/Condition.hpp>
#include <Radiant/IODefs.hpp>
#include <Radiant/Thread.hpp>

#include <list>
#include <QString>
#include <vector>

struct SNDFILE_tag;
struct SF_INFO;

namespace Resonant {

  /** Read/write multiple audio files at the same time in one
      background-thread. */

  class RESONANT_API AudioFileHandler : public Radiant::Thread
  {
    friend class Handle;

  public:

    /// @cond
    // The status of the file opening process.
    enum OpenStatus {
      OPEN_NOT,
      OPEN_DONE,
      OPEN_FAILED,
      OPEN_EOF,
      OPEN_CLOSED
    };
    /// @endcond

    /** A handle that offers access to the audio files. Each handle corresponds
        to a single audio file. */
    class Handle
    {
    public:
      friend class AudioFileHandler;

      /// Blocks until the file has been opened
      /** @return True if the file was opened successfully. */
      bool waitOpen();
      /// Checks if the file is now open.
      /// @return True is file is open
      bool isOpen() const { return m_file != 0; }

      /// Write frames into the file (asynchronously).
      /// @param data Buffer to write, (channels() * frames) floats
      /// @param frames number of frames in the buffer
      /// @return Number of frames written, or negative value on error
      int writeFrames(float * data, int frames);
      /// Read frames from the file.
      /// @param data Buffer that should hold at least (channels() * frames) floats
      /// @param frames Maximum number of frames to read
      /// @return Number of frames read to data, or negative value on error
      int readFrames(float * data, int frames);

      /// @copydoc writeFrames(float * data, int frames)
      int writeFrames(int * data, int frames)
      { return writeFrames((float *) data, frames); }
      /// readFrames(float * data, int frames)
      int readFrames(int * data, int frames)
      { return readFrames((float *) data, frames); }

      /// The number of channels in the sound file.
      int channels()   const;
      /// The sampling rate of the sound file
      int sampleRate() const;
      /// The total number of frames in the sound file
      long frames()    const;
      /// Current read/write frame counter
      long currentFrame() const { return m_userFrames; }

      /// Rewinds the file to a given frame
      void rewind(long frame);
      /// Check if the file is ready for using.
      bool isReady() const;

    private:

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

      QString m_fileName;
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

    /// Returns the first AudioFileHandler instance. This function is not
    /// thread-safe.
    static AudioFileHandler * instance() { return m_instance; }

    /// Starts the reading process for a given file
    /** @param filename The name of the file to read
        @param startFrame The initial frame for reading.
        This value is usually zero, for reading from the beginning of the file.
        @param userFormat The sample format the user of the handle is going to use.
        @return Read-only handle to the new audio file
        */
    Handle * readFile(const char * filename, long startFrame,
              Radiant::AudioSampleFormat userFormat = Radiant::ASF_FLOAT32);
    /// Starts the writing process for a file.
    /** @param filename The name of the audio file
        @param channels The number of audio channels in the file.
        @param samplerate The sampling rate of the audio file, typically 44100, 48000 etc.
        @param sfFormat The format of the sound file. The format is created by combining libsndfile
        sample type with lbsndfile file type. For example SF_FORMAT_WAV | SF_FORMAT_PCM_24
        will give file in wav format, with 24-bits per sample.
        @param userFormat The sample format the user of the handle is going to use.
        @return Write-only handle to the new audio file

        @see http://www.mega-nerd.com/libsndfile/api.html#open

    */
    Handle * writeFile(const char * filename,
               int channels,
               int samplerate,
               int sfFormat,
               // Either Radiant::ASF_FLOAT32 or Radiant::ASF_INT32
               Radiant::AudioSampleFormat userFormat = Radiant::ASF_FLOAT32);

    /// Tells the handler, that a given file handle can be deleted
    /** After calling this function, you should not use this handle any more. */
    void done(Handle *);

    /// Start the audio file IO thread
    void start();
    /// Stop the audio file IO thread
    void stop();

    /// Gets information about a given audio file
    /** @param filename The name of the audio file.
        @param info The information structure, that will be filled with relevant information
        @return True if the information was successfully obtained, and false otherwise.

    */
    static bool getInfo(const char * filename, SF_INFO * info);

    static SNDFILE_tag * open(const QString& filename, int openMode, SF_INFO *info);

  private:

    virtual void childLoop();

    bool update();

    void waitCond()   { m_cond.wait(m_mutex); }
    void signalCond() { m_cond.wakeOne(m_mutex); }

    typedef std::list<Handle *> container;
    typedef container::iterator iterator;

    container m_files;

    Radiant::Mutex m_mutex;
    /// For m_files
    Radiant::Mutex m_filesMutex;
    Radiant::Condition m_cond;

    volatile bool m_done;

    static AudioFileHandler * m_instance;
  };

}

#endif
