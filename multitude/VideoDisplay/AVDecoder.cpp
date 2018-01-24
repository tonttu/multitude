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
#include "FfmpegDecoder.hpp"
#include "DummyDecoder.hpp"

#include <QFileInfo>

namespace VideoDisplay
{
  /// Mapping from backend name to decoder creator
  static std::map<QString, std::function<std::shared_ptr<AVDecoder>()>> s_decoderFactories;

  static Radiant::Mutex s_decodersMutex;
  static std::vector<std::weak_ptr<AVDecoder>> s_decoders;

  void init()
  {
    ffmpegInit();
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

  bool AVDecoder::realTimeSeeking() const
  {
    return false;
  }

  bool AVDecoder::setRealTimeSeeking(bool)
  {
    return false;
  }

  bool AVDecoder::isLooping() const
  {
    return false;
  }

  bool AVDecoder::setLooping(bool)
  {
    return false;
  }

  double AVDecoder::duration() const
  {
    return std::numeric_limits<double>::quiet_NaN();
  }

  QByteArray AVDecoder::audioPannerSourceId() const
  {
    return QByteArray();
  }

  bool AVDecoder::setAudioGain(float)
  {
    return false;
  }

  bool AVDecoder::setMinimizeAudioLatency(bool)
  {
    return false;
  }

  void AVDecoder::setPreviousDecoder(AVDecoderPtr decoder)
  {
    m_d->m_previousDecoder = decoder;
  }

  void AVDecoder::shutdown()
  {
    Radiant::Guard g(s_decodersMutex);
    for (auto weak: s_decoders) {
      if (auto decoder = weak.lock()) {
        decoder->close();
      }
    }
    for (auto weak: s_decoders) {
      if (auto decoder = weak.lock()) {
        decoder->waitEnd();
      }
    }
  }

  std::shared_ptr<AVDecoder> AVDecoder::create(const Options & options)
  {
    std::shared_ptr<AVDecoder> decoder;

    auto it = s_decoderFactories.find(options.decoderBackend());
    if(it != s_decoderFactories.end())
      decoder = it->second();
    else if (options.decoderBackend() == "dummy")
      decoder.reset(new DummyDecoder());
    else
      decoder.reset(new FfmpegDecoder());

    {
      Radiant::Guard g(s_decodersMutex);
      for (auto it = s_decoders.begin(); it != s_decoders.end();) {
        if (it->lock()) {
          ++it;
        } else {
          it = s_decoders.erase(it);
        }
      }
      s_decoders.push_back(decoder);
    }

    decoder->load(options);
    return decoder;
  }

  void AVDecoder::addDecoderBackend
  (const QString& backendName, std::function<std::shared_ptr<AVDecoder>()> factoryFunc)
  {
    s_decoderFactories[backendName] = factoryFunc;
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
