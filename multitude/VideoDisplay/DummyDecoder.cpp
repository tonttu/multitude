#include "DummyDecoder.hpp"

#include <Radiant/Sleep.hpp>

#include <random>

namespace VideoDisplay
{
  struct Frame
  {
    bool used = false;
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

    std::mt19937 gen((unsigned int)f.data.data());
    std::uniform_int_distribution<> dis(0, 255);
    for (auto & v: f.data)
      v = dis(gen);
  }

  class DummyDecoder::D
  {
  public:
    std::atomic<bool> m_running { true };
    AVDecoder::PlayMode m_mode = AVDecoder::PAUSE;
    Nimble::Size m_size {1920, 1080};
    double m_fps = 60;

    // At m_syncTime we should be playing frame m_syncFrame
    Radiant::TimeStamp m_syncTime;
    int m_syncFrame = 0;

    int m_seekGeneration = 0;
    int m_frameNum = -1;

    Radiant::Mutex m_framesMutex;
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
    if(isRunning())
      waitEnd();
  }

  void DummyDecoder::close()
  {
    m_d->m_running = false;
  }

  AVDecoder::PlayMode DummyDecoder::playMode() const
  {
    return m_d->m_mode;
  }

  void DummyDecoder::setPlayMode(AVDecoder::PlayMode mode)
  {
    if (m_d->m_mode == mode) return;
    ++m_d->m_seekGeneration;
    m_d->m_mode = mode;
    m_d->m_syncTime = Radiant::TimeStamp::currentTime();
    m_d->m_syncFrame = m_d->m_frameNum;
  }

  int DummyDecoder::seek(const AVDecoder::SeekRequest &)
  {
    return ++m_d->m_seekGeneration;
  }

  Nimble::Size DummyDecoder::videoSize() const
  {
    return m_d->m_size;
  }

  Timestamp DummyDecoder::getTimestampAt(const Radiant::TimeStamp & ts) const
  {
    if (m_d->m_mode == AVDecoder::PLAY) {
      double t = ts.secondsD() - m_d->m_syncTime.secondsD();
      return Timestamp(m_d->m_syncFrame / m_d->m_fps + t, m_d->m_seekGeneration);
    } else {
      return latestDecodedVideoTimestamp();
    }
  }

  Timestamp DummyDecoder::latestDecodedVideoTimestamp() const
  {
    Radiant::Guard g(m_d->m_framesMutex);

    if (m_d->m_frameNum < 0)
      return Timestamp();
    Frame & f = m_d->m_frames[m_d->m_frameNum % m_d->m_frames.size()];
    return f.videoFrame.timestamp();
  }

  VideoFrame * DummyDecoder::getFrame(const Timestamp & ts, AVDecoder::ErrorFlags & errors) const
  {
    Radiant::Guard g(m_d->m_framesMutex);

    Frame * ret = 0;
    for (int idx = std::max<int>(0, m_d->m_frameNum - m_d->m_frames.size() + 1);
         idx <= m_d->m_frameNum; ++idx) {
      Frame & f = m_d->m_frames[idx % m_d->m_frames.size()];
      if (!f.used && f.videoFrame.timestamp().seekGeneration() != ts.seekGeneration())
        continue;

      if (f.videoFrame.timestamp() > ts) {
        if (ret) return &ret->videoFrame;
        return &f.videoFrame;
      }

      if (f.videoFrame.timestamp() == ts)
        return &f.videoFrame;

      ret = &f;
    }

    errors |= ERROR_VIDEO_FRAME_BUFFER_UNDERRUN;
    return ret ? &ret->videoFrame : nullptr;
  }

  int DummyDecoder::releaseOldVideoFrames(const Timestamp & ts, bool *)
  {
    Radiant::Guard g(m_d->m_framesMutex);

    int count = 0;

    // keep the latest frame alive
    for (int idx = std::max<int>(0, m_d->m_frameNum - m_d->m_frames.size() + 1);
         idx < m_d->m_frameNum; ++idx) {
      Frame & f = m_d->m_frames[idx % m_d->m_frames.size()];
      if (f.used && f.videoFrame.timestamp() <= ts) {
        f.used = false;
        ++count;
      }
    }

    return count;
  }

  Nimble::Matrix4f DummyDecoder::yuvMatrix() const
  {
    return Nimble::Matrix4f(1.16438,         0,   1.59602, -0.871071,
                            1.16438, -0.391769, -0.812973,  0.529312,
                            1.16438,   2.01723,         0,  -1.08167,
                                  0,         0,         0,         1);
  }

  void DummyDecoder::load(const AVDecoder::Options & options)
  {
    setPlayMode(options.playMode());
  }

  void DummyDecoder::runDecoder()
  {
    for (Frame & f: m_d->m_frames)
      init(f, m_d->m_size);

    state() = STATE_HEADER_READY;

    while (m_d->m_running) {
      bool full = true;
      if (m_d->m_mode == AVDecoder::PLAY) {
        Radiant::Guard g(m_d->m_framesMutex);
        Frame & f = m_d->m_frames[(m_d->m_frameNum + 1) % m_d->m_frames.size()];
        if (!f.used) {
          f.videoFrame.setIndex(++m_d->m_frameNum);
          f.videoFrame.setTimestamp(Timestamp(m_d->m_frameNum / m_d->m_fps, m_d->m_seekGeneration));
          f.used = true;
          state() = STATE_READY;
          full = false;
        }
      }

      if (full)
        Radiant::Sleep::sleepSome(1.0 / m_d->m_fps);
    }
    state() = STATE_FINISHED;
  }
} // namespace VideoDisplay
