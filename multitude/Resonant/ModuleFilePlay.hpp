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

#ifndef RESONANT_MODULE_FILE_PLAY_HPP
#define RESONANT_MODULE_FILE_PLAY_HPP

#include <Resonant/AudioFileHandler.hpp>

#include <Resonant/Module.hpp>

namespace Resonant {

  /** Audio file player module. */
  class ModuleFilePlay : public Module
  {
  public:
    RESONANT_API ModuleFilePlay(Application *);
    RESONANT_API virtual ~ModuleFilePlay();

    RESONANT_API virtual bool prepare(int & channelsIn, int & channelsOut);
    RESONANT_API virtual void process(Radiant::BinaryData *, float ** in, float ** out, int n);
    RESONANT_API virtual bool stop();
    
  private:

    Resonant::AudioFileHandler::Handle * m_file;
    
    std::string m_filename;
    std::vector<float> m_interleaved;
  };

}

#endif
