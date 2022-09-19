/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef WINDOWSSTRUCTURES_HPP
#define WINDOWSSTRUCTURES_HPP

#include <Nimble/Size.hpp>

#include <QString>

namespace VideoDisplay
{

  struct VideoInput
  {
    QString friendlyName;
    QString devicePath;

    QString rgbDeviceName;
    int rgbIndex = -1;

    QString magewellDevicePath;

    QString instanceId;

    QString asString() const;
    bool operator==(const VideoInput& other) const;
    bool operator<(const VideoInput& other) const;
  };

  struct AudioInput
  {
    QString friendlyName;
    QString devicePath;

    int waveInId = -1; // invalid==-1

    bool isValid() const;
    QString asString() const;
    bool operator==(const AudioInput& other) const;
    bool operator<(const AudioInput& other) const;
  };

  // ---------------------------------------------------------------------

  /// Snapshot of the state of single AV input source
  struct SourceState
  {
    Nimble::Size resolution{0, 0};
    bool enabled = false;
  };

  /// Single AV source, essentially a (videoinput, audioinput)-pair
  struct Source
  {
    Source() {}
    Source(const VideoInput& vi, const AudioInput& ai)
      : video(vi), audio(ai) {}

    virtual SourceState update();
    QString ffmpegName() const;
    QString friendlyName() const;
    bool isValid() const;
    bool operator<(const Source& other) const;
    bool operator==(const Source& other) const;

    SourceState previousState;
    VideoInput video;
    AudioInput audio;
  };
  typedef std::unique_ptr<Source> SourcePtr;
}

#endif // WINDOWSSTRUCTURES_HPP
