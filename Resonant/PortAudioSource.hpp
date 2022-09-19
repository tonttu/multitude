/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RESONANT_PORTAUDIOSOURCE_HPP
#define RESONANT_PORTAUDIOSOURCE_HPP

#include "ModuleBufferPlayer.hpp"
#include "SourceInfo.hpp"

namespace Resonant
{
  /// Forwards PortAudio source (microphone, line-input or other capture source)
  /// to the DSPNetwork. PortAudio will most likely spawn a new thread when using
  /// this class.
  /// First open the source, and then add module() to DSPNetwork manually.
  class RESONANT_API PortAudioSource
  {
  public:
    enum class OpenResult
    {
      SUCCESS,               ///< Device was opened successfully
      PA_INIT_ERROR,         ///< Pa_Initialize failed
      PA_DEVICE_NOT_FOUND,   ///< Failed to find PortAudio device with the given name
      NO_INPUT_CHANNELS,     ///< There are no input channels on the device
      PA_OPEN_ERROR,         ///< Failed to open PA stream
      PA_START_ERROR         ///< Failed to start PA stream
    };

  public:
    /// @param name Resonant::Module name prefix
    PortAudioSource(const QString & name);
    /// Calls close if the player wasn't closed already
    ~PortAudioSource();

    /// Synchronously opens an input source
    /// @param deviceName full name matching the PortAudio device name (use
    ///                   ListPortAudioDevices to list them all), or just ALSA
    ///                   name like "hw:2,0" in the same format how PortAudio
    ///                   prints it.
    /// @param errorMessage[out] Error message
    OpenResult open(const QString & deviceName, QString * errorMessage);

    QList<SourceInfo> sources(QString * errorMessage);

    /// Synchronously closes the input source
    void close();

    ModuleBufferPlayerPtr module() const;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
} // namespace Resonant

#endif // RESONANT_PORTAUDIOSOURCE_HPP
