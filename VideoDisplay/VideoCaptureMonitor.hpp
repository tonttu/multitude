/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#pragma once

#include "Export.hpp"

#include <Valuable/Node.hpp>

#include <Radiant/Task.hpp>

#include <QList>

#include <memory>

#define debugVideoCapture(...) (Radiant::trace("VideoCapture", Radiant::Trace::DEBUG, __VA_ARGS__))

namespace VideoDisplay
{
  /// This class monitors capture devices connected to the machine, and sends
  /// events when devices are added, removed or changed.
  ///
  /// @event[out] source-added(QByteArray device, Nimble::Vector2i resolution, QString humanReadableName)
  /// @event[out] source-removed(QByteArray device)
  /// @event[out] resolution-changed(QByteArray device, Nimble::Vector2i resolution)
  class VIDEODISPLAY_API VideoCaptureMonitor : public Valuable::Node, public Radiant::Task
  {
    DECLARE_SINGLETON(VideoCaptureMonitor);
    MEMCHECKED

  public:
    struct VideoSource
    {
      QByteArray device;
      QString friendlyName;
      Nimble::Vector2i resolution;
    };

    ~VideoCaptureMonitor();

    /// Polling interval in seconds
    double pollInterval() const;
    void setPollInterval(double seconds);

    /// If the application has some additional information before the
    /// scanning task is started it can add hints. Only values that matter
    /// are values sent in source-* -events.
    /// @note this is only used on Windows at the moment
    void addHint(const QString& device);

    /// Sometimes client may have failed to open source. In this case
    /// it would like to have event if the source is still relevant. For example
    /// in Windows it seems that there is some latency for device to be able to
    /// used after it has been released by some other entity
    void removeSource(const QString& source);

    /// Returns currently active video sources
    QList<VideoSource> sources() const;

  private:
    VideoCaptureMonitor();
    virtual void doTask() override;

    class D;
    std::unique_ptr<D> m_d;
  };
  typedef std::shared_ptr<VideoCaptureMonitor> VideoCaptureMonitorPtr;
}
