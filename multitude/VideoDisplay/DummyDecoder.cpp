#include "DummyDecoder.hpp"

#include <random>

namespace VideoDisplay
{
  struct Frame
  {
    VideoFrame videoFrame;

    std::vector<uint8_t> data;
  };

  void init(Frame & f, Nimble::Size resolution)
  {
    Nimble::Vector2i ry = resolution.toVector();
    Nimble::Vector2i ruv = ry / 2;

    VideoFrame & v = f.videoFrame;
    v.setImageSize(ry);

    v.setFormat(VideoFrame::YUV);
    v.setPlanes(3);

    v.setPlaneSize(0, ry);
    v.setPlaneSize(1, ruv);
    v.setPlaneSize(2, ruv);

    v.setLineSize(0, ry.x);
    v.setLineSize(1, ruv.x);
    v.setLineSize(2, ruv.x);

    f.data.resize(ry.x * ry.y + 2 * (ruv.x * ruv.y));
    v.setData(0, f.data.data());
    v.setData(1, f.data.data() + ry.x * ry.y);
    v.setData(2, f.data.data() + ry.x * ry.y + ruv.x * ruv.y);

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

    std::array<Frame, 20> m_frames;
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

  VideoFrame * DummyDecoder::playFrame(Radiant::TimeStamp presentTimestamp, ErrorFlags &, PlayFlags)
  {
    if (state() != STATE_READY)
      return nullptr;

    Timestamp ts = m_d->m_sync.map(presentTimestamp);
    uint64_t frameNum = ts.pts() * m_d->m_fps;

    return &m_d->m_frames[frameNum % m_d->m_frames.size()].videoFrame;
  }

  int DummyDecoder::releaseOldVideoFrames(const Timestamp &, bool *)
  {
    return 0;
  }

  Nimble::Matrix4f DummyDecoder::yuvMatrix() const
  {
    return Nimble::Matrix4f(1.16438,         0,   1.59602, -0.871071,
                            1.16438, -0.391769, -0.812973,  0.529312,
                            1.16438,   2.01723,         0,  -1.08167,
                                  0,         0,         0,         1);
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

    for (Frame & f: m_d->m_frames)
      init(f, m_d->m_size);

    m_d->m_sync.sync(Radiant::TimeStamp::currentTime(), Timestamp());
    state() = STATE_READY;
  }
} // namespace VideoDisplay
