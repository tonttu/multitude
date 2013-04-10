/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "AVDecoder.hpp"

/// @todo this include is just for create(), should be removed
#include "LibavDecoder.hpp"

namespace VideoDisplay
{
  class AVDecoder::D
  {
  public:
    D();

  public:
    AVDecoder::DecoderState m_state;
  };

  AVDecoder::D::D()
    : m_state(AVDecoder::STATE_LOADING)
  {}

  AVDecoder::AVDecoder()
    : m_d(new D())
  {
  }

  AVDecoder::~AVDecoder()
  {
  }

  AVDecoder::DecoderState & AVDecoder::state()
  {
    return m_d->m_state;
  }

  const AVDecoder::DecoderState & AVDecoder::state() const
  {
    return m_d->m_state;
  }

  bool AVDecoder::finished() const
  {
    return m_d->m_state == STATE_ERROR || m_d->m_state == STATE_FINISHED;
  }

  bool AVDecoder::isHeaderReady() const
  {
    return m_d->m_state == STATE_HEADER_READY || m_d->m_state == STATE_READY || m_d->m_state == STATE_FINISHED;
  }

  bool AVDecoder::hasError() const
  {
    return m_d->m_state == STATE_ERROR;
  }

  std::shared_ptr<AVDecoder> AVDecoder::create(const Options & options, const QString & /*backend*/)
  {
    /// @todo add some great factory registry thing here
    std::shared_ptr<AVDecoder> decoder(new LibavDecoder());
    decoder->load(options);
    return decoder;
  }
}
