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

#ifndef RESONANT_MODULE_OUT_COLLECT_HPP
#define RESONANT_MODULE_OUT_COLLECT_HPP

#include <string>
#include <string.h>

#include <Resonant/Module.hpp>

#include <vector>

namespace Resonant {

  class DSPNetwork;

  /** Collect input from various sources and interleave it for audio
      playback. */

  class ModuleOutCollect : public Module
  {
  public:

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    class Move 
    {
    public:
      Move() : from(0), to(0) { sourceId[0] = 0; }
      char sourceId[Module::MAX_ID_LENGTH];
      int from, to;
    };
#endif

    ModuleOutCollect(Application *, DSPNetwork *);
    virtual ~ModuleOutCollect();

    virtual bool prepare(int & channelsIn, int & channelsOut);
    virtual void processMessage(const char *, Radiant::BinaryData *);
    virtual void process(float ** in, float ** out, int n);
    
    const float * interleaved() const { return & m_interleaved[0]; }

    int channels() const { return m_channels; }

  private:

    int  m_channels;
    DSPNetwork * m_host;
    std::vector<float> m_interleaved;

    typedef std::vector<Move> container;
    typedef container::iterator iterator;
    container m_map;
  };

  inline bool operator == (const ModuleOutCollect::Move & a,
			   const ModuleOutCollect::Move & b)
  {
    return (a.from == b.from)  && (a.to == b.to) && 
      (strcmp(a.sourceId, b.sourceId) == 0);
  }
}

#endif
