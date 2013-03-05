/* COPYRIGHT
 *
 * This file is part of VideoDisplay.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "AVDecoder.hpp"

/// @todo this include is just for create(), should be removed
#include "AVDecoderFFMPEG.hpp"

namespace VideoDisplay
{
  class AVDecoder::D
  {
  public:
    D();

  public:
    AVDecoder::VideoState m_state;
  };

  AVDecoder::D::D()
    : m_state(AVDecoder::LOADING)
  {}

  AVDecoder::AVDecoder()
    : m_d(new D())
  {
  }

  AVDecoder::~AVDecoder()
  {
  }

  AVDecoder::VideoState & AVDecoder::state()
  {
    return m_d->m_state;
  }

  const AVDecoder::VideoState & AVDecoder::state() const
  {
    return m_d->m_state;
  }

  bool AVDecoder::finished() const
  {
    return m_d->m_state == ERROR || m_d->m_state == FINISHED;
  }

  bool AVDecoder::isHeaderReady() const
  {
    return m_d->m_state == HEADER_READY || m_d->m_state == READY || m_d->m_state == FINISHED;
  }

  bool AVDecoder::hasError() const
  {
    return m_d->m_state == Valuable::ERROR;
  }

  std::shared_ptr<AVDecoder> AVDecoder::create(const Options & options, const QString & /*backend*/)
  {
    /// @todo add some great factory registry thing here
    std::shared_ptr<AVDecoder> decoder(new AVDecoderFFMPEG());
    decoder->load(options);
    return decoder;
  }
}
