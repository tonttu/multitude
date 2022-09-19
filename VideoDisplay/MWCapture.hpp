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

#include <Radiant/Singleton.hpp>

#include "WindowsVideoHelpers.hpp"

namespace VideoDisplay
{
  /// Magewell capture card support for VideoCaptureMonitor for Windows
  class MWCapture
  {
    DECLARE_SINGLETON(MWCapture);
    MWCapture();

  public:
    ~MWCapture();

    void initInput(VideoInput & vi);

    std::unique_ptr<Source> createSource(const VideoInput & videoInput,
                                         const AudioInput & audioInput);
  };

} // namespace VideoDisplay
