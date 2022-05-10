/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RESONANT_MODULE_FILE_PLAY_HPP
#define RESONANT_MODULE_FILE_PLAY_HPP

#include "Export.hpp"
#include "AudioFileHandler.hpp"
#include "Module.hpp"

namespace Resonant {

  /** Audio file player module. */
  class RESONANT_API ModuleFilePlay : public Resonant::Module
  {
  public:
    /// Constructs a new audio file player
    ModuleFilePlay();
    virtual ~ModuleFilePlay();

    virtual bool prepare(int & channelsIn, int & channelsOut);
    virtual void process(float ** in, float ** out, int n, const CallbackTime &);
    virtual bool stop();

  private:

    Resonant::AudioFileHandler::Handle * m_file;

    QString m_filename;
    std::vector<float> m_interleaved;
  };

}

#endif
