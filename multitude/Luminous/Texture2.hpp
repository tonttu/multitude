#if !defined (LUMINOUS_TEXTURE2_HPP)
#define LUMINOUS_TEXTURE_HPP

#include "Luminous/RenderResource.hpp"
#include "Luminous/PixelFormat.hpp"

namespace Luminous
{
  class Texture2 : public RenderResource
  {
  public:
    enum WrappingMode {
      Wrap_ClampToEdge,
      Wrap_ClampToBorder,
      Wrap_MirrorRepeat,
      Wrap_Repeat,
    };
    enum SwizzleMode {
      Swizzle_Red,
      Swizzle_Green,
      Swizzle_Blue,
      Swizzle_Zero,
      Swizzle_One,
    };
    enum MinimizeFilter {
      Minimize_Nearest,
      Minimize_Linear,
      Minimize_Nearest_MipMap_Nearest,
      Minimize_Linear_MipMap_Nearest,
      Minimize_Nearest_MipMap_Linear,
      Minimize_Linear_MipMap_Linear,
    };
    enum MagnifyFilter {
      Magnify_Nearest,
      Magnify_Linear,
    };
    
  public:
    Texture2(RenderResource::Id id, RenderDriver & driver);
    ~Texture2();

    /// Allocates an internal data buffer
    void allocate(unsigned int width, PixelFormat format);
    void allocate(unsigned int width, unsigned int height, PixelFormat format);
    void allocate(unsigned int width, unsigned int height, unsigned int depth, PixelFormat format);

    /// Uses the given pointer as a data buffer
    void setData(unsigned int width, PixelFormat format, char * data);
    void setData(unsigned int width, unsigned int height, PixelFormat format, char * data);
    void setData(unsigned int width, unsigned int height, unsigned int depth, PixelFormat format, char * data);

    /// Writes data to the data buffer
    void write(const char * data, size_t offset, size_t size);
    /// Reads data from the data buffer
    void read(char * data, size_t offset, size_t size);

    /// Returns a pointer to the texture data
    const char * data() const;
    /// Returns the used pixel format
    PixelFormat format() const;

    /// Enable/disable mipmapping
    void enableMipMapping(bool enable);
    /// Returns true if mipmapping is enabled
    bool mipMapping() const;

    /// Returns the number of texture dimensions (1, 2 or 3)
    unsigned int dimensions() const;

    /// Set the minimize filter
    void setMinimizeFilter(MinimizeFilter filter);
    /// Set the magnify filter
    void setMagnifyFilter(MagnifyFilter filter);
    /// Returns the minimize filter
    MinimizeFilter minimizeFilter() const;
    /// Returns the magnify filter
    MagnifyFilter magnifyFilter() const;

    /// Swizzle flags
    void setSwizzle(SwizzleMode red, SwizzleMode green, SwizzleMode blue, SwizzleMode alpha);
    void setSwizzleRed(SwizzleMode mode);
    void setSwizzleGreen(SwizzleMode mode);
    void setSwizzleBlue(SwizzleMode mode);
    void setSwizzleAlpha(SwizzleMode mode);
    SwizzleMode swizzleRed();
    SwizzleMode swizzleGreen();
    SwizzleMode swizzleBlue();
    SwizzleMode swizzleAlpha();

    /// Texture wrapping mode
    void setWrapping(WrappingMode s, WrappingMode t, WrappingMode u);
    void setWrappingS(WrappingMode s);
    void setWrappingT(WrappingMode s);
    void setWrappingU(WrappingMode s);
    WrappingMode wrappingS() const;
    WrappingMode wrappingT() const;
    WrappingMode wrappingU() const;

    /// Returns width of the texture
    unsigned int width() const;
    /// Returns height of the texture (or 0 if this is a 1D texture)
    unsigned int height() const;
    /// Returns depth of the texture (or 0 if this is a 1D or 2D texture)
    unsigned int depth() const;
  private:
    class D;
    D * m_d;
  };
}
#endif // LUMINOUS_TEXTURE_HPP
