/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_VM1_HPP
#define LUMINOUS_VM1_HPP

/// @cond

#include "Export.hpp"

#include <functional>
#include <Valuable/Node.hpp>
#include <Radiant/SerialPort.hpp>

namespace Luminous
{
  class ColorCorrection;

  /// Has exclusive access to VM1. Can block.
  typedef std::function<void(Radiant::SerialPort & vm1)> VM1Task;

  // This class is internal to MultiTouch Ltd. Do not use this class. It will
  // be removed in future revisions.
  // Use VM1 class only from the main thread.
  class LUMINOUS_API VM1 : public Valuable::Node
  {
    DECLARE_SINGLETON(VM1);

  public:
    /// Values used in VM1
    enum VideoSource {
      SOURCE_NONE         = 0,
      SOURCE_EXTERNAL_DVI = 1,
      SOURCE_INTERNAL_DVI = 2,
      SOURCE_TEST_IMAGE   = 3,
      SOURCE_LOGO         = 4,

      SOURCE_SCREENSAVER  = 1000
    };

    enum SourceStatus {
      STATUS_UNKNOWN,
      STATUS_NOT_CONNECTED,
      STATUS_DETECTED,
      STATUS_ACTIVE
    };

    enum Maybe {
      MAYBE_FALSE,
      MAYBE_TRUE,
      MAYBE_UNKNOWN
    };

    ~VM1();

    /// @returns True if this computer has VM1 and we have connected to it.
    ///          False doesn't necessary mean that there is no VM1, we might
    ///          be still trying to connect to it.
    bool isConnected() const;

    /// @param timeoutFromBeginningSecs how long to wait to connect from the
    ///                                 point when we started to open VM1.
    bool waitForConnection(double timeoutFromBeginningSecs) const;

    QString version() const;
    QString boardRevision() const;

    Maybe isAutoselectEnabled() const;
    VideoSource priorityVideoSource() const;

    SourceStatus videoSourceStatus(VideoSource) const;
    VideoSource activeVideoSource() const;

    Nimble::Vector2i totalSize() const;
    Nimble::Vector2i activeSize() const;

    Radiant::TimeStamp bootTime() const;

    /// @return logo timeout in minutes
    int logoTimeout() const;

    /// @return VM1 temperature in celsius degrees
    int temperature(Radiant::TimeStamp * timestamp = nullptr) const;

    Maybe isColorCorrectionEnabled() const;

    int sdramStatus() const;
    int sdramTotal() const;

    /// Lines that couldn't been parsed since last time VM1 info was read
    QStringList unknownLines();

    void setColorCorrection(const ColorCorrection & cc);
    void setLCDPower(bool enable);
    void setLogoTimeout(int timeoutMins);

    void setAutoselect(bool enabled);

    void setActiveVideoSource(VideoSource);
    void setPriorityVideoSource(VideoSource);
    void setColorCorrectionEnabled(bool enabled);

/*    QByteArray rawInfo(bool useCachedValue, Radiant::TimeStamp * ts = nullptr);
    Info info(bool useCachedValue, Radiant::TimeStamp * ts = nullptr);

    static Info parseInfo(const QByteArray & info);*/

    void write(const QByteArray & data);

    /// Run a VM1 task with exclusive access to VM1 for the duration. Can block.
    /// Is asynchronous. This call returns immediately but the actual lambda can be
    /// scheduled much later.
    void scheduleTask(const VM1Task & task);

    void reconnect();

    void run();

    static bool enabled();
    static void setEnabled(bool enabled);

  private:
    VM1();

    class D;
    std::unique_ptr<D> m_d;
  };
  typedef std::shared_ptr<VM1> VM1Ptr;
}

/// @endcond

#endif // VM1_HPP
