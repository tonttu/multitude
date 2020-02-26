/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#if !defined (LUMINOUS_RENDERDRIVER_HPP)
#define LUMINOUS_RENDERDRIVER_HPP

/// @cond

#include "Luminous/Luminous.hpp"
#include "Luminous/RenderResource.hpp"
#include "Luminous/RenderCommand.hpp"
#include "Luminous/Style.hpp"
#include "Luminous/Buffer.hpp"
#include "Luminous/FrameBuffer.hpp"
#include "CullMode.hpp"

#include <Radiant/Color.hpp>
#include <memory>
#include <Radiant/Flags.hpp>

#include <QString>

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector3.hpp>
#include <Nimble/Vector4.hpp>
#include <Nimble/Matrix2.hpp>
#include <Nimble/Matrix3.hpp>
#include <Nimble/Matrix4.hpp>

namespace folly
{
  class ManualExecutor;
}

namespace Luminous
{
  class GfxDriver;

  /// The base class for different render drivers.
  class RenderDriver : public Patterns::NotCopyable
  {
  public:
    struct GpuInfo
    {
      /// Name of the GPU, for example "GeForce GTX 970"
      QByteArray name;
      /// GPU index
      int gpu = -1;
      /// PCI domain number
      int pciDomain = -1;
      /// PCI bus number
      int pciBus = -1;
      /// PCI device number
      int pciDevice = -1;
      /// PCI function number
      int pciFunction = -1;
      /// Link width, for example 16
      int link = -1;
      /// Link speed in MT/s, for example 8000
      int speed = -1;
      /// The NUMA node this GPU is attached to, or -1
      int numaNode = -1;
      /// If this GPU belongs to NUMA node, this is a list of logical CPU cores
      /// belonging to the same NUMA node.
      std::vector<int> cpuList;

#ifdef RADIANT_WINDOWS
      /// DXGI / D3D adapter LUID, valid if GPU affinity is used
      LUID dxgiAdapterLuid;
      /// CUDA device, valid if GPU affinity is used
      int cudaDev = -1;
#endif

      inline QByteArray busId() const
      {
        char buffer[128];
        int dev = std::max(0, pciDevice);
        int func = std::max(0, pciFunction);
        if (pciDomain >= 0) {
          // https://wiki.xen.org/wiki/Bus:Device.Function_(BDF)_Notation
          // This format also matches what NVML uses
          snprintf(buffer, sizeof(buffer), "%08x:%02x:%02x.%x", pciDomain, pciBus, dev, func);
        } else if (pciBus >= 0) {
          snprintf(buffer, sizeof(buffer), "%02x:%02x.%x", pciBus, dev, func);
        } else {
          return QByteArray();
        }
        return buffer;
      }
    };

  public:
    RenderDriver(GfxDriver & gfxDriver, int threadIndex);
    virtual ~RenderDriver();

    // Clear the current framebuffer
    LUMINOUS_API virtual void clear(ClearMask mask, const Radiant::ColorPMA & color, double depth, int stencil) = 0;

    // Draw primitives
    LUMINOUS_API virtual void draw(PrimitiveType type, unsigned int offset, unsigned int primitives) = 0;
    // Draw indexed primitives
    LUMINOUS_API virtual void drawIndexed(PrimitiveType type, unsigned int offset, unsigned int primitives) = 0;

    // Called at the beginning of every frame
    LUMINOUS_API virtual void preFrame() = 0;
    // Called at the end of every frame
    LUMINOUS_API virtual void postFrame() = 0;

    // Called when the rendering thread starts
    LUMINOUS_API virtual bool initialize() = 0;
    // Called when the rendering thread stops
    LUMINOUS_API virtual void deInitialize() = 0;

    // Reset the renderstate to its default
    LUMINOUS_API virtual void setDefaultState() = 0;

    // Enable/disable renderbuffers
    LUMINOUS_API virtual void setRenderBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer) = 0;

    LUMINOUS_API virtual void * mapBuffer(const Buffer & buffer, Buffer::Type type, int offset, std::size_t length,
                                          Radiant::FlagsT<Buffer::MapAccess> access) = 0;
    LUMINOUS_API virtual void unmapBuffer(const Buffer & buffer, Buffer::Type type, int offset = 0,
                                          std::size_t length = std::size_t(-1)) = 0;

    LUMINOUS_API virtual RenderCommand & createRenderCommand(bool translucent,
                                                             const VertexArray & vertexArray,
                                                             const Buffer & uniformBuffer,
                                                             const Luminous::Program & shader,
                                                             const std::map<QByteArray, const Texture *> * textures,
                                                             const std::map<QByteArray, ShaderUniform> * uniforms) = 0;

    LUMINOUS_API virtual MultiDrawCommand & createMultiDrawCommand(
        bool translucent,
        int drawCount,
        const VertexArray & vertexArray,
        const Buffer & uniformBuffer,
        const Luminous::Program & shader,
        const std::map<QByteArray, const Texture *> * textures,
        const std::map<QByteArray, ShaderUniform> * uniforms) = 0;

    LUMINOUS_API virtual void flush() = 0;

    LUMINOUS_API virtual void setBlendMode(const BlendMode & mode) = 0;
    LUMINOUS_API virtual void setDepthMode(const DepthMode & mode) = 0;
    LUMINOUS_API virtual void setStencilMode(const StencilMode & mode) = 0;
    LUMINOUS_API virtual void setCullMode(const CullMode & mode) = 0;
    LUMINOUS_API virtual void setFrontFace(FaceWinding winding) = 0;

    LUMINOUS_API virtual void enableClipDistance(const QList<int> & planes) = 0;
    LUMINOUS_API virtual void disableClipDistance(const QList<int> & planes) = 0;

    LUMINOUS_API virtual void setDrawBuffers(const std::vector<GLenum> & buffers) = 0;

    LUMINOUS_API virtual void setViewport(const Nimble::Recti & rect) = 0;
    LUMINOUS_API virtual void setScissor(const Nimble::Recti & rect) = 0;

    LUMINOUS_API virtual void blit(const Nimble::Recti & src, const Nimble::Recti & dst,
                                   Luminous::ClearMask mask = Luminous::CLEARMASK_COLOR_DEPTH,
                                   Luminous::Texture::Filter filter = Luminous::Texture::FILTER_NEAREST) = 0;

    LUMINOUS_API virtual int64_t uploadLimit() const = 0;
    LUMINOUS_API virtual int64_t uploadMargin() const = 0;
    LUMINOUS_API virtual void setUploadLimits(int64_t limit, int64_t margin) = 0;

    LUMINOUS_API virtual int uniformBufferOffsetAlignment() const = 0;

    LUMINOUS_API virtual bool setupSwapGroup(int group, int screen) = 0;

    LUMINOUS_API virtual void setUpdateFrequency(float fps) = 0;

    // Driver factory
    LUMINOUS_API static std::shared_ptr<RenderDriver> createInstance(
        GfxDriver & gfxDriver, unsigned int threadIndex,
        QScreen * screen, const QSurfaceFormat & format);

    /// Set the GPU id for the driver
    LUMINOUS_API virtual void setGPUId(unsigned int gpuId) = 0;
    /// Get the GPU id for the driver, returns -1 if GPU id is not available
    LUMINOUS_API virtual unsigned int gpuId() const = 0;

    folly::ManualExecutor & afterFlush() { return *m_afterFlush; }

    const GpuInfo & gpuInfo() const { return m_gpuInfo; }
    void setGpuInfo(const GpuInfo & gpuInfo) { m_gpuInfo = gpuInfo; }

    unsigned int threadIndex() const { return m_threadIndex; }

    const GfxDriver & gfxDriver() const { return m_gfxDriver; }
    GfxDriver & gfxDriver() { return m_gfxDriver; }

  private:
    // Not exported, should only be used by the render manager
    friend class RenderManager;
    // Marks a resource as deleted, queuing it for removal on GPU
    virtual void releaseResource(RenderResource::Id id) = 0;

  private:
    GpuInfo m_gpuInfo;
    GfxDriver & m_gfxDriver;
    unsigned int m_threadIndex;
    std::unique_ptr<folly::ManualExecutor> m_afterFlush;
  };
}

/// @endcond

#endif // LUMINOUS_RENDERDRIVER_HPP
