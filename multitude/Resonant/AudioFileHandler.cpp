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

#include "AudioFileHandler.hpp"

#include <Radiant/Trace.hpp>
#include <Radiant/Sleep.hpp>

#include <sndfile.h>

#ifndef WIN32
#include <sched.h>
#endif

#include <string.h>
#include <strings.h>

#include <cassert>

namespace Resonant {
  
  using namespace Radiant;
  
  AudioFileHandler::Handle::Handle
  (AudioFileHandler * host, 
   const char * filename, 
   IoMode mode, 
   long startFrame, 
   Radiant::AudioSampleFormat userFormat)
    : m_host(host),
      m_fileName(filename),
      m_ioMode(mode),
      m_startFrame(startFrame),
      m_rewindTo(-1),
      m_status(OPEN_NOT),
      m_ready(false),
      m_file(0),
      m_info(0),
      m_userFormat(userFormat),
      m_userDone(false)
  {
    m_info = new SF_INFO();
    bzero( m_info, sizeof(SF_INFO));
  }

  AudioFileHandler::Handle::~Handle()
  {
    close();
    delete m_info;
  }

  bool AudioFileHandler::Handle::waitOpen()
  {
    if(m_status == OPEN_DONE)
      return true;
  
    lock();
    while(m_status == OPEN_NOT)
      waitCond();
    unlock();

    return m_status == OPEN_DONE;
  }

  int AudioFileHandler::Handle::writeFrames(float * data, int frames)
  {
    if(m_status != OPEN_DONE)
      return -1;

    int r = frames;

    while(frames) {

      /* trace2("AudioFileHandler::Handle::writeFrames # %ld %ld", 
	 m_userFrames, m_fileFrames); */

      long blockleft = m_blockSize - (m_userFrames % m_blockSize);

      int avail = frames > blockleft ? blockleft : frames;

      lock();
      while(m_userFrames + avail > m_fileFrames + m_blockSize * (m_blocks - 1))
	waitCond();
      unlock();

      // trace("AudioFileHandler::Handle::writeFrames >", frames);
    
      int samples = avail * channels();

      memcpy(ptr(m_userFrames), data, sizeof(float) * samples);
    
      data += samples;
      m_userFrames += avail;
      frames -= avail;
    }

    return r;
  }

  int AudioFileHandler::Handle::readFrames(float * data, int nframes)
  {
    if(m_status != OPEN_DONE && m_status != OPEN_EOF)
      return -1;

    int r = 0;

    long untilEnd = frames() - m_userFrames;

    if(nframes > untilEnd)
      nframes = untilEnd;

    while(nframes) {

      long blockleft = m_blockSize - (m_userFrames % m_blockSize);

      long avail = (nframes > blockleft) ? blockleft : nframes;

      /* trace2("AudioFileHandler::Handle::readFrames %ld %ld %ld %ld", 
	 frames, avail, m_userFrames, m_fileFrames); */

      lock();
      while(m_userFrames + avail > m_fileFrames)
	waitCond();
      unlock();

      int samples = avail * channels();

      memcpy(data, ptr(m_userFrames), sizeof(float) * samples);
    
      data += samples;
      m_userFrames += avail;
      nframes -= avail;
      r += avail;
    }
  
    return r;
  }

  void AudioFileHandler::Handle::rewind(long frame)
  {
    assert(frame >= 0);

    m_rewindTo = frame;
    m_ready = false;
  }

  bool AudioFileHandler::Handle::isReady()
  {
    return m_ready && m_rewindTo < 0;
  }

  /** Update the file structure. */

  bool AudioFileHandler::Handle::update()
  {
    bool something = false;

    if(m_userDone) {

      // puts("Closing the file");

      if(m_status == OPEN_DONE && m_ioMode == IO_OUTPUT)
	flushWrite();
      
      close();
    
      m_status = OPEN_CLOSED;

      something = true;
    }
    else if(m_status == OPEN_NOT) {

      if(open())
	m_status = OPEN_DONE;
      else
	m_status = OPEN_FAILED;

      something =  true;
    }
    else if(m_status == OPEN_DONE) {

      if(m_ioMode == IO_INPUT) {

	if(m_rewindTo >= 0) {
	  moveReadHead(m_rewindTo);
	  m_rewindTo = -1;
	  something = fileRead();
	}

	something = something || fileRead();

	m_ready = true;
      }
      else
	something = fileWrite();
    }
    else if(m_status == OPEN_EOF) {

      if(m_rewindTo >= 0) {
	moveReadHead(m_rewindTo);
	something =  true;
      }
      /*
	close();
	something =  true;
	m_status = OPEN_CLOSED;
      */
    }

    return something;
  }

  bool AudioFileHandler::Handle::open()
  {
    close();

    int mode = (m_ioMode == IO_INPUT) ? SFM_READ : SFM_WRITE;

    m_file = sf_open(m_fileName.c_str(), mode, m_info);

    if(!m_file)
      return false;

    m_blocks = 6;
    m_blockSize = m_info->samplerate / m_blocks;

    int buffersamples = m_info->channels * m_blockSize * m_blocks;

    m_data.resize(buffersamples);

    if(!m_data.empty())
      bzero( & m_data[0], buffersamples * sizeof(float));

    m_fileFrames = 0;
    m_userFrames = 0;

    if(mode == SFM_WRITE)
      m_ready = true;
    else if(m_startFrame)
      moveReadHead(m_startFrame, false);
  
    return true;
  }

  /** Closes the audio file. */

  bool AudioFileHandler::Handle::close()
  {
    if(!m_file)
      return false;

    sf_close(m_file);

    m_file = 0;

    return true;
  }

  /** Reads data from the audio file. */
  bool AudioFileHandler::Handle::fileRead()
  {
    // trace("AudioFileHandler::Handle::fileRead", m_fileFrames);

    if(m_fileFrames >= (m_userFrames + (m_blocks - 1) * m_blockSize))
      return false;

    float * data = ptr(m_fileFrames);
  
    long blockleft = m_blockSize - (m_fileFrames % m_blockSize);
    long avail = long(m_info->frames - m_fileFrames);

    if(avail > blockleft)
      avail = blockleft;
  
    if(m_userFormat == Radiant::ASF_FLOAT32)
      sf_readf_float(m_file, data, avail);
    else
      sf_readf_int(m_file, (int *) data, avail);

    m_fileFrames += avail;

    if(m_fileFrames == m_info->frames)
      m_status = OPEN_EOF;

    return true;
  }

  bool AudioFileHandler::Handle::fileWrite()
  {
    /* trace2("AudioFileHandler::Handle::fileWrite # HELLO %ld %ld", 
       m_fileFrames, m_userFrames); */

    if(m_fileFrames + m_blockSize > m_userFrames)
      return false;

    /* trace2("AudioFileHandler::Handle::fileWrite # %ld %ld", 
       m_fileFrames, m_userFrames); */

    while(m_fileFrames + m_blockSize <= m_userFrames) {
      float * data = ptr(m_fileFrames);

      if(m_userFormat == Radiant::ASF_FLOAT32)
	sf_writef_float(m_file, data, m_blockSize);
      else
	sf_writef_int(m_file, (int *) data, m_blockSize);
    

      m_fileFrames += m_blockSize;
    }

    return true;
  }

  bool AudioFileHandler::Handle::flushWrite()
  {
    /* trace2("AudioFileHandler::Handle::flushWrite # %ld %ld", 
       m_fileFrames, m_userFrames); */

    while(m_fileFrames < m_userFrames) {

      long avail = m_userFrames - m_fileFrames;
      long n = avail > m_blockSize ? m_blockSize : avail;

      float * data = ptr(m_fileFrames);
      sf_writef_float(m_file, data, n);
      m_fileFrames += n;
    }

    return true;
  }

  bool AudioFileHandler::Handle::moveReadHead(long frame, bool clear)
  {
    debug("AudioFileHandler::Handle::moveReadHead # %s %ld ", 
	  m_fileName.c_str(), frame);

    if(!m_data.empty() && clear)
      bzero( & m_data[0], m_data.size() * sizeof(float));

    if(sf_seek(m_file, frame, SEEK_SET) != frame) {
		Radiant::error("AudioFileHandler::Handle::moveReadHead");
      return false;
    }

    m_fileFrames = frame;
    m_startFrame = frame;
    m_userFrames = frame;

    if(m_status == OPEN_EOF)
      m_status = OPEN_DONE;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  AudioFileHandler * AudioFileHandler::m_instance = 0;

  AudioFileHandler::AudioFileHandler()
    : m_done(true)
  {
    if(!m_instance)
      m_instance = this;
  }

  AudioFileHandler::~AudioFileHandler()
  {
    if(!m_done)
      stop();

    if(m_instance == this)
      m_instance = 0;
  }

  AudioFileHandler::Handle * AudioFileHandler::readFile
  (const char * filename, long startFrame, Radiant::AudioSampleFormat userFormat)
  {
    assert(this);
    assert(userFormat == Radiant::ASF_FLOAT32 || userFormat == Radiant::ASF_INT32);

    Handle * h = new Handle(this, filename, IO_INPUT, startFrame, userFormat);

    m_mutex2.lock();
    m_files.push_back(h);
    m_mutex2.unlock();

    return h;
  }

  /** Prepare to write to a file.  */

  AudioFileHandler::Handle * AudioFileHandler::writeFile
  (
   /// The name of the file to write to
   const char * filename, 
   /// Number of channels
   int channels, 
   /// Sample rate (Hz)
   int samplerate, 
   /// file format in sndfile format
   int sfFormat, 
   /// The sample format that the application is using.
   Radiant::AudioSampleFormat userFormat)
  {
    assert(this);
    assert(userFormat == Radiant::ASF_FLOAT32 || userFormat == Radiant::ASF_INT32);


    Handle * h = new Handle(this, filename, IO_OUTPUT, 0, userFormat);

    h->m_info->channels   = channels;
    h->m_info->samplerate = samplerate;
    h->m_info->format     = sfFormat;

    m_mutex2.lock();
    m_files.push_back(h);
    m_mutex2.unlock();

    return h;
  }

  /** Return a file handle to the system, implying that its work is
      done. */

  void AudioFileHandler::done(Handle * h)
  {
    /* iterator it = std::find(m_files.begin(), m_files.end(), h);

    if(it == m_files.end())
    return; */

    h->m_userDone = true;
  }

  void AudioFileHandler::start()
  {
    assert(this);

    if(!m_done)
      return;

    m_done = false;

    run();
  }

  void AudioFileHandler::stop()
  {
    assert(this);

    if(m_done)
      return;

    m_done = true;
    waitEnd();
  }


  bool AudioFileHandler::getInfo(const char * filename, SF_INFO * info)
  {
    bzero(info, sizeof(SF_INFO));

    SNDFILE * file = sf_open(filename, SFM_READ, info);

    if(!file)
      return false;

    sf_close(file);

    return true;
  }

  void AudioFileHandler::childLoop()
  {  
    assert(this);

#ifdef __linux__
    {
      // Set real-time scheduler:
      sched_param param;
      param.sched_priority = 20;
      pid_t pid = getpid();
      sched_setscheduler(pid, SCHED_FIFO, &param);
    }
#endif

    while(!m_done) {
      if(!update()) {
	Sleep::sleepMs(20);
      }
    }

    for(iterator it = m_files.begin(); it != m_files.end(); it++) {
      Handle * h = (*it);
      h->m_userDone = true;
      h->update();
      delete h;
    }

    m_files.clear();
  }

  bool AudioFileHandler::update()
  {
    assert(this);

    bool something = false;

    m_mutex2.lock();

    for(iterator it = m_files.begin(); it != m_files.end(); ) {

      Handle * h = (*it);

      something = something || h->update();

      if(h->m_status == OPEN_CLOSED) {
	/* trace2("AudioFileHandler::update # Deleting file handle %s",
	   h->m_fileName.c_str()); */
	delete h;
	iterator tmp = it;
	tmp++;
	m_files.erase(it);
	it = tmp;
      }
      else
	it++;
    }

    m_mutex2.unlock();

    if(something)
      signalCond();

    // trace2("AudioFileHandler::update # %d", (int) something);

    return something;
  }

  int AudioFileHandler::Handle::channels() const 
  {
    return m_info->channels;
  }

  int AudioFileHandler::Handle::sampleRate() const 
  {
    return m_info->samplerate;
  }

  long AudioFileHandler::Handle::frames() const
  {
    return (long)m_info->frames;
  }
}
