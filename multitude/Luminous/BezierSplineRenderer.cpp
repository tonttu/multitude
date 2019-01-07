#include "BezierSplineRenderer.hpp"
#include "BezierSplineTesselator.hpp"
#include "Buffer.hpp"
#include "BufferGL.hpp"
#include "ContextArray.hpp"
#include "RenderContext.hpp"
#include "VertexArray.hpp"

#include <QMutex>

#include <boost/container/flat_map.hpp>

/// Number of consecutive frames with inefficient buffer usage before freeing
/// the existing buffer and reallocating a new smaller one.
static constexpr int s_bufferRecreateFrames = 30;
/// How many vertices to preallocate to a empty vertex buffer
static constexpr int s_bufferMinSize = 512;
/// How much (from 0 to 1) of the buffer needs to be used so that it's not
/// considered as inefficient.
static constexpr float s_bufferMinUsage = 0.15f;

namespace Luminous
{
  /// One LOD mipmap triangle strip of a stroke
  struct StrokeMipmap
  {
    StrokeMipmap() = default;
    StrokeMipmap(const StrokeMipmap & o)
      : ready((bool)o.ready)
      , triangleStrip(o.triangleStrip)
    {}

    std::atomic<bool> ready{false};
    QMutex generateMutex;

    std::vector<BezierSplineTesselator::Vertex> triangleStrip;
  };

  /// Stroke mipmap cache
  struct StrokeCache
  {
    BezierSplineRenderer::Stroke stroke;
    uint32_t cpuGeneration = 1;
    std::vector<StrokeMipmap> mipmaps;
  };

  /// One LOD mipmap triangle strip of a stroke, pointing on vertex buffer on GPU
  struct StrokeMipmapGpu
  {
    /// Copied from Stroke::depth here for quick sorting
    float depth = 0;

    /// Stroke::id, for matching the source stroke when invalidating changed or removed strokes
    Valuable::Node::Uuid strokeId = 0;

    /// Offset to GpuContext::buffer
    uint32_t bufferOffset = 0;
    uint32_t vertexCount = 0;

    /// Matches StrokeCache::cpuGeneration. If the generation doesn't match,
    /// the GPU cache needs to be recreated
    uint32_t cpuGeneration = 0;
  };

  /// Stroke mipmap cache on GPU
  struct StrokeCacheGpu
  {
    std::vector<StrokeMipmapGpu> levels;
  };

  /// Each separate ViewWidget path to the BezierSplineRenderer has its own View
  /// @todo add "lastUsed" and remove unused Views
  struct View
  {
    int activeLod = std::numeric_limits<int>::min();
    /// Viewport used when picking renderables. As long as the current
    /// viewport is inside this one, renderables can be reused.
    Nimble::Rectf viewRect;
    /// All mipmaps that should be rendered
    std::vector<StrokeMipmapGpu> renderables;

    /// If false, renderables are already in order
    bool depthChanged = false;

    /// Removed, changed or added strokes since last time we rendered something
    std::set<Valuable::Node::Uuid> removed;
    std::set<Valuable::Node::Uuid> changed;
    std::set<Valuable::Node::Uuid> added;
  };

  /// Each render thread has its own data
  struct GpuContext
  {
    GpuContext()
    {
      VertexDescription descr;
      descr.addAttribute<Nimble::Vector2f>("vertex_position");
      descr.addAttribute<Nimble::Vector4f>("vertex_color");
      vertexArray.addBinding(vertexBuffer, descr);

      buffer.reserve(s_bufferMinSize);
    }

    Buffer vertexBuffer;
    VertexArray vertexArray;

    /// Ideally we wouldn't have this. All data should be maintained on GPU
    /// memory, but the current rendering pipeline makes it difficult, so
    /// we always keep a copy of the data on host memory as well.
    std::vector<BezierSplineTesselator::Vertex> buffer;

    /// RenderContext::viewWidgetPathId() -> View
    Radiant::ArrayMap<QByteArray, View> views;

    /// Stroke id -> GPU mipmap cache
    std::map<Valuable::Node::Uuid, StrokeCacheGpu> mipmaps;

    /// Each time we render something so that buffer reserved way more memory
    /// than needed, we increase this counter. When this reaches certain value,
    /// we free all memory in buffer and recreate it from scratch to optimize
    /// memory usage.
    int consecutiveFramesWithInefficientBufferUsage = 0;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class BezierSplineRenderer::D
  {
  public:
    int scaleToLod(float scale) const;
    float lodToScale(int lod) const;

    StrokeMipmapGpu & createMipmapLevelGpu(GpuContext & gpuContext, StrokeCache & mipmap, int mipmapLevel, float invScale);

  public:
    BezierSplineRenderer::RenderOptions m_opts;
    int m_minLod = 0;
    int m_lodLevels = 0;

    boost::container::flat_map<Valuable::Node::Uuid, StrokeCache> m_mipmaps;

    ContextArrayT<GpuContext> m_gpuContext;

    int m_translucentStrokes = 0;
  };

  int BezierSplineRenderer::D::scaleToLod(float scale) const
  {
    int lod = std::log2(scale) - m_minLod;
    return std::max(0, std::min(lod, m_lodLevels - 1));
  }

  float BezierSplineRenderer::D::lodToScale(int lod) const
  {
    return std::pow(2.f, lod + m_minLod);
  }

  StrokeMipmapGpu & BezierSplineRenderer::D::createMipmapLevelGpu(
      GpuContext & gpuContext, StrokeCache & mipmap, int mipmapLevel, float invScale)
  {
    if ((int)mipmap.mipmaps.size() != m_lodLevels)
      mipmap.mipmaps.resize(m_lodLevels);

    StrokeMipmap & level = mipmap.mipmaps[mipmapLevel];
    if (!level.ready) {
      QMutexLocker locker(&level.generateMutex);
      if (!level.ready) {
        BezierSplineTesselator tesselator(level.triangleStrip, m_opts.maxCurveError * invScale,
                                          m_opts.maxRoundCapError * invScale);
        tesselator.tesselate(*mipmap.stroke.path, mipmap.stroke.color);
        level.ready = true;
      }
    }

    StrokeCacheGpu & mipmapGpu = gpuContext.mipmaps[mipmap.stroke.id];
    if ((int)mipmapGpu.levels.size() != m_lodLevels)
      mipmapGpu.levels.resize(m_lodLevels);

    StrokeMipmapGpu & levelGpu = mipmapGpu.levels[mipmapLevel];
    if (levelGpu.cpuGeneration < mipmap.cpuGeneration) {
      levelGpu.strokeId = mipmap.stroke.id;
      levelGpu.depth = mipmap.stroke.depth;
      levelGpu.cpuGeneration = mipmap.cpuGeneration;

      const uint32_t bufferSize = gpuContext.buffer.size();
      const uint32_t vertexCount = level.triangleStrip.size();

      if (levelGpu.bufferOffset + levelGpu.vertexCount == bufferSize) {
        // This is the last item in the buffer, we can just replace it
        gpuContext.buffer.resize(bufferSize - levelGpu.vertexCount + vertexCount);
      } else if (vertexCount > levelGpu.vertexCount) {
        // Append to the end
        levelGpu.vertexCount = vertexCount;
        levelGpu.bufferOffset = bufferSize;
        gpuContext.buffer.insert(gpuContext.buffer.end(), level.triangleStrip.data(),
                                 level.triangleStrip.data() + levelGpu.vertexCount);
        return levelGpu;
      } // else the contents fit to the same region

      levelGpu.vertexCount = vertexCount;
      gpuContext.vertexBuffer.invalidateRegion(levelGpu.bufferOffset * sizeof(gpuContext.buffer[0]),
          vertexCount * sizeof(gpuContext.buffer[0]));
      std::copy_n(level.triangleStrip.data(), levelGpu.vertexCount,
                  gpuContext.buffer.data() + levelGpu.bufferOffset);
    }

    return levelGpu;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  BezierSplineRenderer::BezierSplineRenderer()
    : BezierSplineRenderer(RenderOptions())
  {
  }

  BezierSplineRenderer::BezierSplineRenderer(RenderOptions opts)
    : m_d(new D())
  {
    setRenderOptions(opts);
  }

  BezierSplineRenderer::~BezierSplineRenderer()
  {
  }

  void BezierSplineRenderer::clear()
  {
    for (GpuContext & context: m_d->m_gpuContext) {
      context.views.clear();
      context.mipmaps.clear();
      context.buffer.clear();
    }

    m_d->m_mipmaps.clear();
    m_d->m_translucentStrokes = 0;
  }

  BezierSplineRenderer::RenderOptions BezierSplineRenderer::renderOptions() const
  {
    return m_d->m_opts;
  }

  void BezierSplineRenderer::setRenderOptions(RenderOptions opts)
  {
    int minLod = std::log2(opts.minScale);
    int maxLod = std::ceil(std::log2(opts.maxScale));
    int lodLevels = maxLod - minLod + 1;

    if (m_d->m_minLod != minLod ||
        m_d->m_lodLevels != lodLevels ||
        m_d->m_opts.maxCurveError != opts.maxCurveError ||
        m_d->m_opts.maxRoundCapError != opts.maxRoundCapError) {
      for (GpuContext & context: m_d->m_gpuContext) {
        context.views.clear();
        context.mipmaps.clear();
        context.buffer.clear();
      }

      for (auto & p: m_d->m_mipmaps) {
        StrokeCache & strokeMipmap = p.second;
        strokeMipmap.cpuGeneration++;
        for (StrokeMipmap & level: strokeMipmap.mipmaps)
          level.ready = false;
      }
    }

    m_d->m_minLod = minLod;
    m_d->m_lodLevels = lodLevels;
    m_d->m_opts = std::move(opts);
  }

  Valuable::Node::Uuid BezierSplineRenderer::addStroke(BezierSplineRenderer::Stroke s)
  {
    if (!s.path)
      return 0;

    if (s.id == 0)
      s.id = Valuable::Node::generateId();

    /// @todo what to do when duplicate is found?
    if (m_d->m_mipmaps.find(s.id) != m_d->m_mipmaps.end())
      return 0;

    if (s.bbox.isEmpty()) {
      for (const BezierNode & p : *s.path) {
        const float radius = p.strokeWidth * 0.5f;
        s.bbox.expand(p.point, radius);
        s.bbox.expand(p.ctrlIn, radius);
        s.bbox.expand(p.ctrlOut, radius);
      }
    }

    if (s.color.alpha() < 1.f)
      ++m_d->m_translucentStrokes;

    StrokeCache & mipmap = m_d->m_mipmaps[s.id];
    mipmap.stroke = s;

    for (GpuContext & context: m_d->m_gpuContext)
      for (auto & p: context.views)
        p.second.added.insert(s.id);

    return s.id;
  }

  void BezierSplineRenderer::removeStroke(Valuable::Node::Uuid id)
  {
    auto it = m_d->m_mipmaps.find(id);
    if (it != m_d->m_mipmaps.end()) {
      if (it->second.stroke.color.alpha() < 1.f)
        --m_d->m_translucentStrokes;

      m_d->m_mipmaps.erase(it);

      for (GpuContext & context: m_d->m_gpuContext) {
        for (auto & p: context.views) {
          p.second.added.erase(id);
          p.second.removed.insert(id);
        }
      }
    }
  }

  void BezierSplineRenderer::setStrokePath(Valuable::Node::Uuid id,
                                           const std::vector<BezierNode> * path,
                                           Nimble::Rect bbox)
  {
    auto it = m_d->m_mipmaps.find(id);
    if (it == m_d->m_mipmaps.end())
      return;

    StrokeCache & c = it->second;

    if (bbox.isEmpty())
      for (auto p : *path)
        bbox.expand(p.point, p.strokeWidth * 0.5f);

    c.stroke.path = path;
    c.stroke.bbox = bbox;
    // Invalidate StrokeMipmapGpu
    c.cpuGeneration++;

    // Invalidate StrokeMipmap
    for (StrokeMipmap & level: c.mipmaps)
      level.ready = false;

    // Invalidate View
    for (GpuContext & context: m_d->m_gpuContext)
      for (auto & p: context.views)
        p.second.changed.insert(id);
  }

  void BezierSplineRenderer::setStrokeColor(Valuable::Node::Uuid id, Radiant::ColorPMA color)
  {
    auto it = m_d->m_mipmaps.find(id);
    if (it == m_d->m_mipmaps.end())
      return;

    StrokeCache & c = it->second;
    if (c.stroke.color.alpha() < 1.f)
      --m_d->m_translucentStrokes;
    if (color.alpha() < 1.f)
      ++m_d->m_translucentStrokes;

    c.stroke.color = color;

    // Invalidate StrokeMipmapGpu
    c.cpuGeneration++;

    // Invalidate StrokeMipmap - this could be optimized by just changing
    // the color in triangleStrip
    for (StrokeMipmap & level: c.mipmaps)
      level.ready = false;

    // Invalidate View
    for (GpuContext & context: m_d->m_gpuContext)
      for (auto & p: context.views)
        p.second.changed.insert(id);
  }

  void BezierSplineRenderer::setStrokeDepth(Valuable::Node::Uuid id, float depth)
  {
    auto it = m_d->m_mipmaps.find(id);
    if (it == m_d->m_mipmaps.end())
      return;

    StrokeCache & c = it->second;
    c.stroke.depth = depth;

    for (GpuContext & context: m_d->m_gpuContext) {
      for (auto & p: context.views) {
        View & v = p.second;
        for (StrokeMipmapGpu & levelGpu: v.renderables) {
          if (levelGpu.strokeId == id) {
            levelGpu.depth = depth;
            v.depthChanged = true;
          }
        }
      }

      auto it2 = context.mipmaps.find(id);
      if (it2 != context.mipmaps.end()) {
        StrokeCacheGpu & mipmapGpu = it2->second;
        for (StrokeMipmapGpu & gpu: mipmapGpu.levels)
          gpu.depth = depth;
      }
    }
  }

  void BezierSplineRenderer::render(RenderContext & r) const
  {
    GpuContext & gpuContext = *m_d->m_gpuContext;

    View & view = gpuContext.views[r.viewWidgetPathId()];

    float scale = r.approximateScaling();
    int mipmapLevel = m_d->scaleToLod(scale);
    if (m_d->scaleToLod(scale * 1.1) == view.activeLod) {
      mipmapLevel = view.activeLod;
    } else if (view.activeLod != mipmapLevel) {
      view.viewRect = Nimble::Rectf();
    }

    float invScale = 1.f / m_d->lodToScale(mipmapLevel);

    Nimble::Rect visibleArea = r.clipStack().boundingBox();
    visibleArea.transform(r.transform3().inverse());
    Nimble::Rect extendedArea = visibleArea;
    extendedArea.shrinkRelative(-0.2f, -0.2f);

    size_t oldBufferSize = gpuContext.buffer.size();
    size_t oldBufferCapacity = gpuContext.buffer.capacity();

    if (gpuContext.consecutiveFramesWithInefficientBufferUsage > s_bufferRecreateFrames) {
      gpuContext.consecutiveFramesWithInefficientBufferUsage = 0;
      gpuContext.buffer.clear();
      gpuContext.buffer.shrink_to_fit();
      gpuContext.buffer.reserve(s_bufferMinSize);
      gpuContext.mipmaps.clear();
      view.viewRect = Nimble::Rectf();
    }

    if (view.viewRect.contains(visibleArea)) {
      if (!view.removed.empty() || !view.changed.empty()) {
        size_t size = view.renderables.size();
        for (size_t idx = 0; idx < size;) {
          StrokeMipmapGpu & mipmapLevelGpu = view.renderables[idx];
          if (view.removed.count(mipmapLevelGpu.strokeId)) {
            mipmapLevelGpu = view.renderables[--size];
            view.depthChanged = true;
          } else if (view.changed.count(mipmapLevelGpu.strokeId)) {
            StrokeCache & mipmap = m_d->m_mipmaps[mipmapLevelGpu.strokeId];
            if (extendedArea.intersects(mipmap.stroke.bbox)) {
              mipmapLevelGpu = m_d->createMipmapLevelGpu(gpuContext, mipmap, mipmapLevel, invScale);
              ++idx;
            } else {
              view.depthChanged = true;
              mipmapLevelGpu = view.renderables[--size];
            }
          } else {
            ++idx;
          }
        }
        view.renderables.resize(size);
        view.removed.clear();
        view.changed.clear();
      }

      for (Valuable::Node::Uuid id: view.added) {
        StrokeCache & mipmap = m_d->m_mipmaps[id];
        if (extendedArea.intersects(mipmap.stroke.bbox)) {
          StrokeMipmapGpu & levelGpu = m_d->createMipmapLevelGpu(gpuContext, mipmap, mipmapLevel, invScale);
          view.renderables.push_back(levelGpu);
          view.depthChanged = true;
        }
      }
      view.added.clear();
    } else {
      view.viewRect = extendedArea;
      view.activeLod = mipmapLevel;
      view.renderables.clear();
      view.depthChanged = true;
      view.removed.clear();
      view.changed.clear();
      view.added.clear();

      for (auto it = m_d->m_mipmaps.begin(), end = m_d->m_mipmaps.end(); it != end; ++it) {
        StrokeCache & mipmap = it->second;
        if (extendedArea.intersects(mipmap.stroke.bbox)) {
          StrokeMipmapGpu & levelGpu = m_d->createMipmapLevelGpu(gpuContext, mipmap, mipmapLevel, invScale);
          view.renderables.push_back(levelGpu);
        }
      }
    }

    if (view.depthChanged) {
      std::sort(view.renderables.begin(), view.renderables.end(), [] (const StrokeMipmapGpu & l, const StrokeMipmapGpu & r) {
        return l.depth < r.depth;
      });
      view.depthChanged = false;
    }

    constexpr uint32_t vertexSize = sizeof(gpuContext.buffer[0]);
    if (oldBufferCapacity != gpuContext.buffer.capacity() || !gpuContext.vertexBuffer.data()) {
      // CPU buffer was (re)allocated, recreate the buffer
      gpuContext.vertexBuffer.setData(gpuContext.buffer.data(),
                                      gpuContext.buffer.size() * vertexSize,
                                      Buffer::DYNAMIC_DRAW,
                                      gpuContext.buffer.capacity() * vertexSize);
    } else if (oldBufferSize != gpuContext.buffer.size()) {
      // Data was appended to the CPU buffer, upload that part
      gpuContext.vertexBuffer.invalidateRegion(oldBufferSize * vertexSize,
                                               (gpuContext.buffer.size() - oldBufferSize) * vertexSize);
    }

    auto builder = r.multiDrawArrays<BasicUniformBlock>(
          m_d->m_translucentStrokes > 0 || r.opacity() < 0.9999f,
          m_d->m_opts.renderAsLineStrip ? PRIMITIVE_LINE_STRIP : PRIMITIVE_TRIANGLE_STRIP,
          view.renderables.size(),
          gpuContext.vertexArray,
          r.splineShader());

    uint32_t renderedVertices = 0;
    for (uint32_t idx = 0, count = view.renderables.size(); idx < count; ++idx) {
      StrokeMipmapGpu & mipmapLevelGpu = view.renderables[idx];
      builder.offsets[idx] = mipmapLevelGpu.bufferOffset;
      builder.counts[idx] = mipmapLevelGpu.vertexCount;
      renderedVertices += mipmapLevelGpu.vertexCount;
    }

    float bufferUsage = float(renderedVertices) / gpuContext.buffer.capacity();
    if (bufferUsage < s_bufferMinUsage && gpuContext.buffer.capacity() > s_bufferMinSize)
      ++gpuContext.consecutiveFramesWithInefficientBufferUsage;
    else
      gpuContext.consecutiveFramesWithInefficientBufferUsage = 0;

    float opacity = r.opacity();
    builder.uniform->color = Nimble::Vector4f(opacity, opacity, opacity, opacity);
    builder.uniform->depth = builder.depth;

    builder.uniform->projMatrix = r.viewTransform().transposed();
    builder.uniform->modelMatrix = r.transform().transposed();

    if (auto stats = m_d->m_opts.stats.get()) {
      stats->renderedStrokes += view.renderables.size();
      stats->renderedVertices += renderedVertices;
      stats->totalStrokes += m_d->m_mipmaps.size();
    }
  }
}
