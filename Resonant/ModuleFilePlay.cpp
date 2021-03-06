/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ModuleFilePlay.hpp"

namespace Resonant {

  ModuleFilePlay::ModuleFilePlay()
  {}

  ModuleFilePlay::~ModuleFilePlay()
  {}

  bool ModuleFilePlay::prepare(int & channelsIn, int & channelsOut)
  {
    AudioFileHandler * afh = AudioFileHandler::instance();

    m_file = afh->readFile(m_filename.toUtf8().data(), 0, Radiant::ASF_FLOAT32);

    bool ok = m_file->waitOpen();

    if(!ok) {
      return false;
    }

    channelsIn = 0;
    channelsOut = m_file->channels();

    m_interleaved.resize(channelsOut * MAX_CYCLE);

    return true;
  }

  void ModuleFilePlay::process(float **, float ** out, int n, const CallbackTime &)
  {
    // assert(m_file);

    int left = m_file->frames() - m_file->currentFrame();

    int m = n < left ? n : left;

    m_file->readFrames( & m_interleaved[0], m);

    int chans = m_file->channels();


    for(int i = 0; i < chans; i++) {
      const float * src = & m_interleaved[i];
      float * dest = out[i];
      float * sentinel = dest + m;

      while(dest < sentinel) {
    *dest = * src;
    dest++;
    src += chans;
      }

      for(int k = m; k < n; k++)
    *dest++ = 0.0f;
    }
  }

  bool ModuleFilePlay::stop()
  {
    if(!m_file)
      return true;

    AudioFileHandler * afh = AudioFileHandler::instance();

    afh->done(m_file);

    m_file = 0;

    m_interleaved.clear();

    return true;
  }

}
