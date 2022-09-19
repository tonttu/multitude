/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef GPUASSOCIATION_HPP
#define GPUASSOCIATION_HPP

#include "Export.hpp"

#include <QOpenGLContext>

namespace Luminous
{  

  /// This class provides a mechanism for applications to explicitly query GPU
  /// information using the AMD_gpu_association OpenGL extension.
  class GPUAssociation
  {
  public:
    /// Is AMD_gpu_association OpenGL extension supported on current hardware?
    /// @return true if the extension is supported; otherwise false
    static LUMINOUS_API bool isSupported();

    /// Get total number of GPUs
    /// @return number of GPUs detected
    static LUMINOUS_API unsigned int numGPUs();

    /// Get GPU id for the given OpenGL context
    /// @param context OpenGL context to query
    /// @return id for the GPU attached to the context
    static LUMINOUS_API unsigned int gpuId(QOpenGLContext& context);

    /// Get amount of RAM available on the given GPU in MB
    /// @param gpuId id of the GPU to query
    /// @return total amount of memory in MB
    static LUMINOUS_API unsigned int gpuRam(unsigned int gpuId);
  };
}

#endif // GPUASSOCIATION_HPP
