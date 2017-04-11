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
#ifdef USE_LIBAV
#include "LibavDecoder.hpp"
#else
#include "FfmpegDecoder.hpp"
#endif

#include <QFileInfo>

namespace VideoDisplay
{

  void init()
  {
#ifdef USE_LIBAV
    libavInit();
#else
    ffmpegInit();
#endif
  }

  class AVDecoder::D
  {
  public:
    D();

  public:
    AVDecoder::DecoderState m_state;
    AVDecoderPtr m_previousDecoder;
  };

  AVDecoder::D::D()
    : m_state(AVDecoder::STATE_LOADING)
  {}

  AVDecoder::AVDecoder()
    : m_d(new D())
  {
  }

  void AVDecoder::childLoop()
  {
    m_d->m_previousDecoder.reset();
    runDecoder();
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

  void AVDecoder::setPreviousDecoder(AVDecoderPtr decoder)
  {
    m_d->m_previousDecoder = decoder;
  }

  std::shared_ptr<AVDecoder> AVDecoder::create(const Options & options, const QString & /*backend*/)
  {
    /// @todo add some great factory registry thing here
#ifdef USE_LIBAV
    std::shared_ptr<AVDecoder> decoder(new LibavDecoder());
#else
    std::shared_ptr<AVDecoder> decoder(new FfmpegDecoder());
#endif
    decoder->load(options);
    return decoder;
  }

  bool AVDecoder::looksLikeV4L2Device(const QString & path)
  {
    QRegExp v4l2m("/dev/(vtx|video|radio|vbi)\\d+");
    if (v4l2m.exactMatch(path))
      return true;

    const QFileInfo pathInfo(path);
    return pathInfo.isSymLink() && v4l2m.exactMatch(pathInfo.symLinkTarget());
  }
}
