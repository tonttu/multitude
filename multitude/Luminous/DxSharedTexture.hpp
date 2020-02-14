#pragma once

#include "Texture.hpp"

#include <Nimble/Size.hpp>

namespace Luminous
{
  class RenderContext;
  class LUMINOUS_API DxSharedTexture
  {
  public:
    DxSharedTexture();
    ~DxSharedTexture();

    bool init(void * sharedHandle);

    void acquire();
    // Returns false if force was false and the texture is still in use
    bool release(bool force);

    void ref(Radiant::TimeStamp time);
    void unref();

    void * sharedHandle() const;
    Radiant::TimeStamp lastUsed() const;

    Nimble::SizeI size() const;

    const Luminous::Texture * texture(RenderContext & r, bool copyIfNeeded, std::weak_ptr<DxSharedTexture> weak);

  private:
    class D;
    std::unique_ptr<D> m_d;
  };

  class LUMINOUS_API DxSharedTextureBag
  {
  public:
    DxSharedTextureBag();
    ~DxSharedTextureBag();

    void addSharedHandle(void * sharedHandle);

    const Luminous::Texture * texture(RenderContext & r);

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}
