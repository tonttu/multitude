#pragma once

namespace Luminous
{
  class RenderDriver;
  class RenderContext;
  class GfxDriver
  {
  public:
    virtual RenderContext & renderContext(unsigned int threadIndex) = 0;
    virtual unsigned int renderThreadCount() const = 0;
    virtual RenderDriver & renderDriver(unsigned int threadIndex) = 0;
  };
}
