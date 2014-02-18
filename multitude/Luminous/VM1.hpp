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

#include <Radiant/Mutex.hpp>
#include <Radiant/SerialPort.hpp>
#include <Radiant/TimeStamp.hpp>

#include <Valuable/Node.hpp>

#include <QMap>

#include <memory>

namespace Luminous
{
  class ColorCorrection;

  // This class is internal to MultiTouch Ltd. Do not use this class. It will
  // be removed in future revisions.
  class LUMINOUS_API VM1 : public Valuable::Node
  {
    DECLARE_SINGLETON(VM1);

  public:
    struct Info
    {
      enum SourceStatus {
        STATUS_UNKNOWN,
        STATUS_NOT_CONNECTED,
        STATUS_DETECTED,
        STATUS_ACTIVE
      };

      enum Maybe {
        INFO_FALSE,
        INFO_TRUE,
        INFO_UNKNOWN
      };

      inline Info();
      inline bool operator==(const Info & o) const;
      inline bool isEmpty() const { return *this == Info(); }

      bool header;
      QString version;
      QString boardRevision;
      Maybe autoselect;
      QMap<QString, SourceStatus> sources;
      QString prioritySource;
      int totalPixels;
      int activePixels;
      int totalLines;
      int activeLines;
      int uptimeMins;
      int screensaverMins;
      int temperature;
      QString videoSource;
      Maybe colorCorrectionEnabled;
      int sdramStatus;
      int sdramTotal;

      QList<QByteArray> extraLines;
    };

    ~VM1();

    bool detected(bool useCachedValue, Radiant::TimeStamp * ts = nullptr);
    void setColorCorrection(const ColorCorrection & cc);
    void setLCDPower(bool enable);
    void setLogoTimeout(int timeout);
    void setVideoAutoselect();
    void setVideoInput(int input);
    void setVideoInputPriority(int input);
    void enableGamma(bool state);

    QByteArray rawInfo(bool useCachedValue, Radiant::TimeStamp * ts = nullptr);
    Info info(bool useCachedValue, Radiant::TimeStamp * ts = nullptr);

    static Info parseInfo(const QByteArray & info);

  private:
    VM1();

    class D;
    std::shared_ptr<D> m_d;
  };
  typedef std::shared_ptr<VM1> VM1Ptr;

  VM1::Info::Info()
    : header(false),
      autoselect(INFO_UNKNOWN),
      totalPixels(std::numeric_limits<int>::min()),
      activePixels(std::numeric_limits<int>::min()),
      totalLines(std::numeric_limits<int>::min()),
      activeLines(std::numeric_limits<int>::min()),
      uptimeMins(std::numeric_limits<int>::min()),
      screensaverMins(std::numeric_limits<int>::min()),
      temperature(std::numeric_limits<int>::min()),
      colorCorrectionEnabled(INFO_UNKNOWN),
      sdramStatus(std::numeric_limits<int>::min()),
      sdramTotal(std::numeric_limits<int>::min())
  {}

  bool VM1::Info::operator==(const VM1::Info & o) const
  {
    return header == o.header &&
        version == o.version &&
        boardRevision == o.boardRevision &&
        autoselect == o.autoselect &&
        sources == o.sources &&
        prioritySource == o.prioritySource &&
        totalPixels == o.totalPixels &&
        activePixels == o.activePixels &&
        totalLines == o.totalLines &&
        activeLines == o.activeLines &&
        uptimeMins == o.uptimeMins &&
        screensaverMins == o.screensaverMins &&
        temperature == o.temperature &&
        videoSource == o.videoSource &&
        colorCorrectionEnabled == o.colorCorrectionEnabled &&
        sdramStatus == o.sdramStatus &&
        sdramTotal == o.sdramTotal &&
        extraLines == o.extraLines;
  }
}

/// @endcond

#endif // VM1_HPP
