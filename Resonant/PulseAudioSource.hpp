/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RESONANT_PULSEAUDIOSOURCE_HPP
#define RESONANT_PULSEAUDIOSOURCE_HPP

#include "ModuleBufferPlayer.hpp"
#include "SourceInfo.hpp"

namespace Resonant
{
  /// Forwards PulseAudio source (microphone, line-input or other capture source)
  /// to the DSPNetwork. PortAudio will most likely spawn a new thread when using
  /// this class.
  /// First open the source, and then add module() to DSPNetwork manually.
  class RESONANT_API PulseAudioSource
  {
  public:
    /// @param name Resonant::Module name prefix
    PulseAudioSource(const QString & name);
    ~PulseAudioSource();

    PulseAudioSource(PulseAudioSource && src);
    PulseAudioSource & operator=(PulseAudioSource &&);

    /// Synchronously opens an input device
    bool open(const QString & sourceName, QString * errorMessage,
              double timeoutSecs, const QString & uiName = QString());

    QList<SourceInfo> sources(QString * errorMessage, double timeoutSecs);

    /// Synchronously closes the input source
    void close();

    ModuleBufferPlayerPtr module() const;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };

} // namespace Resonant

#endif // RESONANT_PULSEAUDIOSOURCE_HPP
