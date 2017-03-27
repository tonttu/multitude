/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RESONANT_AUDIO_LOOP_HPP
#define RESONANT_AUDIO_LOOP_HPP

#include "Export.hpp"

#include <Radiant/TimeStamp.hpp>

namespace Resonant
{
  /// Timing and latency information about the processed samples.
  struct CallbackTime
  {
    CallbackTime(Radiant::TimeStamp outputTime, double latency, unsigned long flags)
      : outputTime(outputTime),
        latency(latency),
        flags(flags)
    {}
    /// When will be this sample be played on the sound card
    const Radiant::TimeStamp outputTime;
    /// Estimated latency
    const double latency;
    /// PaStreamCallbackFlags
    const unsigned long flags;
  };

  /// A simple audio IO class API.
  ///
  /// Implement this to provide different backends for DSPNetwork.
  class AudioLoop
  {
  public:
    virtual ~AudioLoop() {}

    /// Start the AudioLoop.
    /// In practice this spans a new thread that is managed by the backend.
    /// @param samplerate Desired samplerate, 44100 is safe choice
    /// @param channels Number of channels to open
    /// @return False on error
    virtual bool start(int samplerate, int channels) = 0;

    /// Stop the audio processing
    virtual bool stop() = 0;

    /// Check if the audio IO is operational
    virtual bool isRunning() const = 0;

    /// Returns the number of output channels in the current setup.
    virtual std::size_t outChannels() const = 0;
  };
}

#endif

