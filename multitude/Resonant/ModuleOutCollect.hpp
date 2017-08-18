/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RESONANT_MODULE_OUT_COLLECT_HPP
#define RESONANT_MODULE_OUT_COLLECT_HPP

#include "LimiterAlgorithm.hpp"
#include "Module.hpp"

#include <QString>

#include <vector>
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
    ModuleOutCollect(DSPNetwork *);
    virtual ~ModuleOutCollect();

    virtual bool prepare(int & channelsIn, int & channelsOut) OVERRIDE;
    virtual void eventProcess(const QByteArray &, Radiant::BinaryData &) OVERRIDE;
    virtual void process(float ** in, float ** out, int n, const Resonant::CallbackTime &) OVERRIDE;

    /// Access the collected frames, which have been interleaved
    const float * interleaved() const { return m_interleaved; }

    void setInterleavedBuffer(float * buffer) { m_interleaved = buffer ? buffer : m_internalInterleavedBuffer.data(); }

    /// Returns the number of channels that are collected by this module
    size_t channels() const { return m_channels; }

    /// Latest sample values for all channels
    const std::vector<float> lastSample() const { return m_lastSample; }

  private:

    size_t m_channels;
    int  m_subwooferChannel;
    bool m_compressOutput;
    DSPNetwork * m_host;

    /// Active interleaved buffer, either m_internalInterleavedBuffer or user-provieded buffer
    float * m_interleaved = nullptr;
    std::vector<float> m_internalInterleavedBuffer;
    std::vector<ChannelLimiter> m_limiters;

    typedef std::vector<Move> container;
    typedef container::iterator iterator;
    container m_map;

    std::vector<float> m_lastSample;
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
