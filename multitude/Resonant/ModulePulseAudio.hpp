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

#ifndef RESONANT_MODULEPULSEAUDIO_HPP
#define RESONANT_MODULEPULSEAUDIO_HPP

#include "Module.hpp"
#include "PulseAudioCore.hpp"

/// @cond

namespace Resonant {


  class RESONANT_API ModulePulseAudio : public Module, public PulseAudioCore
  {
  public:
    ModulePulseAudio(const QString & monitorName, uint32_t sinkInput);
    virtual ~ModulePulseAudio();

    void contextChange(pa_context_state_t state);
    void dataAvailable(pa_stream * p, size_t nbytes);
    void streamState(pa_stream_state_t state);

    bool prepare(int & channelsIn, int & channelsOut);
    void process(float ** in, float ** out, int n);

    bool stop();

  protected:
    bool m_ready;
    pa_stream * m_stream;
    QString m_monitorName;
    uint32_t m_sinkInput;
    float * m_buffer; // pointer to m_buffer
    std::vector<float> m_bufferData; // actual storage
    size_t m_bufferSize; // in samples

    int m_syncCount;
    bool m_canSync;

    void openStream();
    void beforeShutdown();
  };
}

/// @endcond

#endif // RESONANT_MODULEPULSEAUDIO_HPP
