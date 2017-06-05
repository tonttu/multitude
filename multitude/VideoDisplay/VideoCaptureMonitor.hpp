#pragma once

#include <Valuable/Node.hpp>

#include <Radiant/Task.hpp>

namespace VideoDisplay
{
  /// @event[out] source-added(QByteArray device, Nimble::Vector2i resolution)
  /// @event[out] source-removed(QByteArray device)
  /// @event[out] resolution-changed(QByteArray device, Nimble::Vector2i resolution)
  class VideoCaptureMonitor : public Valuable::Node, public Radiant::Task
  {
  public:
    VideoCaptureMonitor();

    virtual void resetSource(const QByteArray & /*device*/) {}

  private:
    /// Called once in the beginning from a background thread.
    /// @return true if initialization succeeded and it's ok to start calling
    ///              poll() periodically
    virtual bool init();
    /// Called once per second from a background thread
    virtual void poll() = 0;

    /// Polling interval in seconds
    double pollInterval() const;
    void setPollInterval(double seconds);

  private:
    virtual void doTask() override;

  private:
    bool m_initialized = false;
    double m_pollInterval = 1.0;
  };
  typedef std::shared_ptr<VideoCaptureMonitor> VideoCaptureMonitorPtr;
}
