/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "DummyDecoder.hpp"

#include <random>

namespace VideoDisplay
{
  struct Frame : public VideoFrame
  {
    std::vector<uint8_t> data;
  };

  void init(Frame & f, Nimble::Size resolution)
  {
    Nimble::Vector2i ry = resolution.toVector();
    Nimble::Vector2i ruv = ry / 2;

    f.setImageSize(ry);

    f.setFormat(VideoFrame::YUV);
    f.setPlanes(3);

    f.setPlaneSize(0, ry);
    f.setPlaneSize(1, ruv);
    f.setPlaneSize(2, ruv);

    f.setLineSize(0, ry.x);
    f.setLineSize(1, ruv.x);
    f.setLineSize(2, ruv.x);

    f.data.resize(ry.x * ry.y + 2 * (ruv.x * ruv.y));
    f.setData(0, f.data.data());
    f.setData(1, f.data.data() + ry.x * ry.y);
    f.setData(2, f.data.data() + ry.x * ry.y + ruv.x * ruv.y);

    std::mt19937 gen((unsigned int)(uintptr_t)f.data.data());
    std::uniform_int_distribution<> dis(0, 255);
    for (auto & v: f.data)
      v = dis(gen);
  }

  class DummyDecoder::D
  {
  public:
    std::atomic<bool> m_running { true };
    AVSync m_sync;

    Nimble::Size m_size {1920, 1080};
    double m_fps = 60;

    std::array<std::shared_ptr<Frame>, 20> m_frames;
  };

  DummyDecoder::DummyDecoder()
    : m_d(new D())
  {
    Thread::setName("DummyDecoder");
  }

  DummyDecoder::~DummyDecoder()
  {
    close();
  }

  void DummyDecoder::close()
  {
    m_d->m_running = false;
    state() = STATE_FINISHED;
  }

  AVSync::PlayMode DummyDecoder::playMode() const
  {
    return m_d->m_sync.playMode();
  }

  void DummyDecoder::setPlayMode(AVSync::PlayMode mode)
  {
    m_d->m_sync.setPlayMode(mode);
  }

  int DummyDecoder::seek(const AVDecoder::SeekRequest &)
  {
    m_d->m_sync.setSeekGeneration(m_d->m_sync.seekGeneration() + 1);
    return m_d->m_sync.seekGeneration();
  }

  Nimble::Size DummyDecoder::videoSize() const
  {
    return m_d->m_size;
  }

  std::shared_ptr<VideoFrame> DummyDecoder::playFrame(Radiant::TimeStamp presentTimestamp, ErrorFlags &, PlayFlags)
  {
    if (state() != STATE_READY)
      return nullptr;

    Timestamp ts = m_d->m_sync.map(presentTimestamp);
    uint64_t frameNum = static_cast<uint64_t>(ts.pts() * m_d->m_fps);

    auto & frame = m_d->m_frames[frameNum % m_d->m_frames.size()];
    frame->setIndex(static_cast<int>(frameNum));
    return frame;
  }

  std::shared_ptr<VideoFrame> DummyDecoder::peekFrame(std::shared_ptr<VideoFrame> ref, int offset)
  {
    Frame & f = static_cast<Frame&>(*ref);
    auto & ret = m_d->m_frames[(f.index() + offset) % m_d->m_frames.size()];
    ret->setIndex(f.index() + offset);
    return ret;
  }

  bool DummyDecoder::isEof() const
  {
    return false;
  }

  Nimble::Matrix4f DummyDecoder::yuvMatrix() const
  {
    return Nimble::Matrix4f(1.16438f,        0.f,   1.59602f, -0.871071f,
                            1.16438f, -0.391769f, -0.812973f,  0.529312f,
                            1.16438f,   2.01723f,        0.f,  -1.08167f,
                                 0.f,        0.f,        0.f,        1.f);
  }

  QString DummyDecoder::source() const
  {
    return "DummyDecoder";
  }

  void DummyDecoder::load(const AVDecoder::Options & options)
  {
    setPlayMode(options.playMode());
  }

  void DummyDecoder::runDecoder()
  {
    state() = STATE_HEADER_READY;

    for (std::shared_ptr<Frame> & f: m_d->m_frames) {
      f.reset(new Frame());
      init(*f, m_d->m_size);
    }

    m_d->m_sync.sync(Radiant::TimeStamp::currentTime(), Timestamp());
    state() = STATE_READY;
  }
} // namespace VideoDisplay
