/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef LUMINOUS_GPU_AFFINITY_HPP
#define LUMINOUS_GPU_AFFINITY_HPP

/// @cond

#include "Export.hpp"

#include <memory>
#include <vector>

#include <QString>
#include <QStringList>
#include <QRect>
#include <QRegion>

namespace Luminous
{
  /// Wrapper for WGL_NV_gpu_affinity extension
  class LUMINOUS_API GPUAffinity
  {
  public:
    /// There must be an active OpenGL context in the calling thread
    GPUAffinity();
    ~GPUAffinity();

    bool isSupported() const;
    std::vector<uint32_t> gpusForDesktopArea(QRect desktop) const;
    std::vector<QRegion> gpuDesktopAreas() const;
    QString gpuName(uint32_t index) const;
    QStringList displayGdiDeviceNames(uint32_t gpuIndex) const;
    QStringList adapterInstanceIds(uint32_t gpuIndex) const;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };

} // namespace Luminous

/// @endcond

#endif // LUMINOUS_GPU_AFFINITY_HPP
