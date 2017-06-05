#pragma once

#include <Valuable/Node.hpp>

#include <Radiant/Task.hpp>

#include <memory>

namespace VideoDisplay
{
  /// This class monitors capture devices connected to the machine, and sends
  /// events when devices are added, removed or changed.
  ///
  /// @event[out] source-added(QByteArray device, Nimble::Vector2i resolution)
  /// @event[out] source-removed(QByteArray device)
  /// @event[out] resolution-changed(QByteArray device, Nimble::Vector2i resolution)
  class VideoCaptureMonitor : public Valuable::Node, public Radiant::Task
  {
  public:
    VideoCaptureMonitor();
    ~VideoCaptureMonitor();

    /// Reset the monitored status of given device
    void resetSource(const QByteArray & device);

    /// Polling interval in seconds
    double pollInterval() const;
    void setPollInterval(double seconds);

  private:
    virtual void doTask() override;

    class D;
    std::unique_ptr<D> m_d;
  };
  typedef std::shared_ptr<VideoCaptureMonitor> VideoCaptureMonitorPtr;
}
