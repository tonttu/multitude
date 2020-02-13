#pragma once

namespace Luminous
{
  class RenderDriver;
  class GfxDriver
  {
  public:
    virtual unsigned int renderThreadCount() const = 0;
    virtual RenderDriver & renderDriver(unsigned int threadIndex) = 0;
  };
}
