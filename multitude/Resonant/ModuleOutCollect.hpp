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

#include <QString>
#include <string.h>

#include "Module.hpp"

#include <vector>
#include <string>
#include <string.h>

namespace Resonant {

  class DSPNetwork;

  /** Collect input from various sources and interleave it for audio
      playback. */

  class RESONANT_API ModuleOutCollect : public Module
  {
  public:

/// @cond
    class Move
    {
    public:
      Move() : from(0), to(0) {}
      QString sourceId;
      int from, to;
    };
/// @endcond

    /// Creates a new ModuleOutCollect
    ModuleOutCollect(Application *, DSPNetwork *);
    virtual ~ModuleOutCollect();

    virtual bool prepare(int & channelsIn, int & channelsOut);
    virtual void processMessage(const QString &, Radiant::BinaryData *);
    virtual void process(float ** in, float ** out, int n);

    /// Access the collected frames, which have been interleaved
    const float * interleaved() const { return & m_interleaved[0]; }

    /// Returns the number of channels that are collected by this module
    size_t channels() const { return m_channels; }

  private:

    size_t m_channels;
    int m_subwooferChannel;
    DSPNetwork * m_host;
    std::vector<float> m_interleaved;

    typedef std::vector<Move> container;
    typedef container::iterator iterator;
    container m_map;
  };

  /// @cond
  inline bool operator == (const ModuleOutCollect::Move & a,
                           const ModuleOutCollect::Move & b)
  {
    return (a.from == b.from)  && (a.to == b.to) &&
      (a.sourceId == b.sourceId);
  }

  /// @endcond
}

#endif
