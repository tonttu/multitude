/* COPYRIGHT
 */

#include "Texture.hpp"

#include "Image.hpp"
#include "PixelFormat.hpp"
#include "Utils.hpp"

#include <cassert>
#include <iostream>
#include <cstring>

#include "Error.hpp"

#include <Radiant/Trace.hpp>
#include <Radiant/Platform.hpp>
#include <Radiant/Thread.hpp>

#include <limits>
#include <algorithm>

namespace
{
  static RADIANT_TLS(int) t_frame(0);
  static RADIANT_TLS(long) t_available(0);
  /// @todo Fix UploadLimiter.. #1982
  static RADIANT_TLS(bool) t_enabled(true);
}

namespace Luminous
{
  using namespace std;
  using namespace Radiant;

  // tests show that 1.5GB/s is a pretty good guess for a lower limit of
  // upload rate with 500x500 RGBA images
  UploadLimiter::UploadLimiter()
    : m_frame(0)
    , m_frameLimit(1.5*(1<<30)/60.0)
    , m_inited(false)
  {
    eventAddIn("frame");
  }

  long & UploadLimiter::available()
  {
    UploadLimiter & i = instance();
    if(t_frame != i.m_frame) {
      t_frame = i.m_frame;
      t_available = i.m_frameLimit;
    }
    if(!i.m_inited || !t_enabled) t_available = std::numeric_limits<long>::max();
    return t_available;
  }

  long UploadLimiter::frame()
  {
    return instance().m_frame;
  }

  long UploadLimiter::limit()
  {
    return instance().m_frameLimit;
  }

  void UploadLimiter::setLimit(long limit)
  {
    instance().m_frameLimit = limit;
  }

  void UploadLimiter::setEnabledForCurrentThread(bool v)
  {
    t_enabled = v;
  }

  bool UploadLimiter::enabledForCurrentThread()
  {
    return t_enabled;
  }


  void UploadLimiter::processMessage(const char * type, Radiant::BinaryData &)
  {
    if(strcmp(type, "frame") == 0) {
      m_inited = true;
      ++m_frame;
    }
  }

  UploadLimiter & UploadLimiter::instance()
  {
    /// @todo this is not thread-safe! use the singleton pattern with RenderContext storing one shared_ptr each to UploadLimiter
    static Luminous::UploadLimiter s_limiter;
    return s_limiter;
  }

  //////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////

  template <GLenum TextureType>
      TextureT<TextureType>::~TextureT()
  {

    if(m_textureId)  {
      glDeleteTextures(1, &m_textureId);
      debugLuminous("Deallocated texture %.5d %p", (int) m_textureId, resources());
    }

    changeByteConsumption(consumesBytes(), 0);
  }

  template <GLenum TextureType>
      void TextureT<TextureType>::allocate()
  {
    if(!m_textureId) {
      glGenTextures(1, & m_textureId);
      debugLuminous("Allocated texture %.5d %p", (int) m_textureId, resources());
    }
  }

  template <GLenum TextureType>
  size_t TextureT<TextureType>::estimateMemoryUse() const
  {
    size_t used = m_width * m_height;

    // Estimate something, try to be conservative
    switch(m_internalFormat) {
    case GL_LUMINANCE:
    case GL_INTENSITY:
      used *= 1;
      break;
    case GL_RGB:
    case GL_BGR:
      used *= 3;
      break;
#ifdef GL_RGB32F
    case GL_RGB32F:
      used *= 3 * 4;
      break;
#endif
#ifdef GL_RGBA32F
    case GL_RGBA32F:
      used *= 4 * 4;
      break;
#endif
    default:
      used *= 4;
      break;
    }

    // Mipmaps used 33% more memory
    used *= (4.f / 3.f);

    return used;
  }

  template class TextureT<GL_TEXTURE_1D>;
  template class TextureT<GL_TEXTURE_2D>;
  template class TextureT<GL_TEXTURE_3D>;

  //////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////

  Texture1D * Texture1D::fromImage(Luminous::Image & image, bool buildMipmaps, GLResources * resources)
  {
    return fromBytes(GL_RGBA, image.height(), image.bytes(), image.pixelFormat(),
                     buildMipmaps, resources);
  }

  Texture1D* Texture1D::fromBytes(GLenum internalFormat,
                                  int h,
                                  const void* data,
                                  const PixelFormat& srcFormat, bool buildMipmaps,
                                  GLResources * resources)
  {
    Texture1D * tex = new Texture1D(resources);

    if(!tex->loadBytes(internalFormat, h, data, srcFormat, buildMipmaps)) {
      delete tex;
      return 0;
    }

    return tex;
  }

  bool Texture1D::loadBytes(GLenum internalFormat, int h,
                            const void* data,
                            const PixelFormat& srcFormat,
                            bool buildMipmaps)
  {
    // Check dimensions
    if(!GL_ARB_texture_non_power_of_two) {
      bool isPowerOfTwo = !((h - 1) & h);

      if(!isPowerOfTwo) {
        error("ERROR: non-power-of-two textures are not supported");
        return false;
      }
    }

    // Reset byte consumption
    m_consumed = 0;
    m_haveMipmaps = buildMipmaps;
    m_width = 1;
    m_height = h;

    bind();

    // Default to bilinear filtering
    GLint minFilter = GL_LINEAR;
    GLint magFilter = GL_LINEAR;

    // ...or trilinear if we have mipmaps
    if(buildMipmaps) minFilter = GL_LINEAR_MIPMAP_LINEAR;

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, magFilter);

    if(buildMipmaps) {
      gluBuild1DMipmaps(GL_TEXTURE_1D,
                        srcFormat.numChannels(), h,
                        srcFormat.layout(), srcFormat.type(), data);
    }
    else
      glTexImage1D(GL_TEXTURE_1D, 0, internalFormat, h, 0,
                   srcFormat.layout(), srcFormat.type(), data);

    // Notify UploadLimiter that we used some bandwidth
    long & available = UploadLimiter::available();
    available -= consumesBytes();

    return true;
  }

  bool Texture2D::loadImage(const char * filename, bool buildMipmaps) {
    Luminous::Image img;

    if(!img.read(filename)) return false;

    return loadImage(img, buildMipmaps);
  }

  bool Texture2D::loadImage(const Luminous::Image & image, bool buildMipmaps, int internalFormat)
  {
    // If internal format is not specified, use the number of channels to let
    // the driver decide what to do.
    // See http://www.opengl.org/sdk/docs/man/xhtml/glTexImage2D.xml
    int iformat = (internalFormat ? internalFormat : image.pixelFormat().numChannels());

    return loadBytes(iformat,
                     image.width(), image.height(),
                     image.bytes(),
                     image.pixelFormat(), buildMipmaps);
  }

  bool Texture2D::loadImage(const CompressedImage & image)
  {
    // Update estimate about consumed bytes
    m_consumed = image.datasize();
    m_internalFormat = image.compression();
    m_width = image.width();
    m_height = image.height();
    m_haveMipmaps = false;

    bind();

    if(resources() && !resources()->isBrokenProxyTexture2D()) {
      glCompressedTexImage2D(GL_PROXY_TEXTURE_2D, 0, m_internalFormat,
                             m_width, m_height, 0, image.datasize(), 0);
      GLint width = m_width;
      glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);

      if(width == 0) {
        Luminous::glErrorToString(__FILE__, __LINE__);
        Radiant::error("Texture2D::loadImage: Cannot load compressed texture, too big? "
                         "(%d x %d, id = %.5d, %d bytes)", m_width, m_height, (int) id(), image.datasize());
        m_consumed = 0;
        return false;
      }
    }

    glCompressedTexImage2D(GL_TEXTURE_2D, 0, m_internalFormat,
                           m_width, m_height, 0, image.datasize(), image.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    long & available = UploadLimiter::available();
    available -= consumesBytes();

    m_uploadedLines = m_height;

    return true;
  }

  bool Texture2D::loadBytes(GLenum internalFormat, int w, int h,
                            const void * data,
                            const PixelFormat& srcFormat,
                            bool buildMipmaps)
  {
    // Check dimensions
    if(!GL_ARB_texture_non_power_of_two) {
      bool isPowerOfTwo1 = !((w - 1) & w);
      bool isPowerOfTwo2 = !((h - 1) & h);

      if(!(isPowerOfTwo1 && isPowerOfTwo2)) {
        error("ERROR: non-power-of-two textures are not supported");
        return false;
      }
    }

    // If no data is specified, we just allocate the texture memory and reset
    // m_uploadedLines. Otherwise we mark the lines loaded.
    if(data)
      m_uploadedLines = h;
    else
      m_uploadedLines = 0;

    m_consumed = 0;
    m_internalFormat = internalFormat;
    m_width = w;
    m_height = h;
    m_haveMipmaps = buildMipmaps;

    bind();

    // Default to bilinear filtering
    GLint minFilter = GL_LINEAR;
    GLint magFilter = GL_LINEAR;

    // set byte alignment to maximum possible: 1,2,4 or 8
    int alignment = 1;
    while (alignment < 8) {
      if ((m_width * srcFormat.bytesPerPixel()) % (alignment*2)) {
        break;
      }
      alignment *= 2;
    }


    // ...or trilinear if we have mipmaps
    if(buildMipmaps) minFilter = GL_LINEAR_MIPMAP_LINEAR;

    if(buildMipmaps) {

      assert(Utils::glCheck("Texture2D::loadBytes # 1"));

      glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

      assert(Utils::glCheck("Texture2D::loadBytes # 2"));

      gluBuild2DMipmaps(GL_TEXTURE_2D, internalFormat,
                        w, h, srcFormat.layout(), srcFormat.type(), data);

      assert(Utils::glCheck("Texture2D::loadBytes # 3"));

    } else {
      /* debugLuminous("TEXTURE UPLOAD :: INTERNAL %s FORMAT %s [%d %d]",
             glInternalFormatToString(internalFormat),
             glFormatToString(srcFormat.layout()), w, h);
      */

      glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

      assert(Utils::glCheck("Texture2D::loadBytes # 4"));

      GLint width = w;

      if(resources() && !resources()->isBrokenProxyTexture2D()) {
        /* On ATI/Linux combination it seems that the GL_PROXY_TEXTURE_2D is
         broken, and cannot be trusted to give correct answers.
         It will at times fail with 1024x768 RGB textures. Sigh. */
        glTexImage2D(GL_PROXY_TEXTURE_2D, 0, internalFormat, w, h, 0,
                     srcFormat.layout(), srcFormat.type(), 0);

        assert(Utils::glCheck("Texture2D::loadBytes # 5"));

        glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);

        assert(Utils::glCheck("Texture2D::loadBytes # 6"));

        if(width == 0) {
          Luminous::glErrorToString(__FILE__, __LINE__);
          Radiant::error("Texture2D::loadBytes: Cannot load texture, too big? "
                         "(%d x %d, id = %.5d)", w, h, (int) id());

          glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

          assert(Utils::glCheck("Texture2D::loadBytes # 7"));

          return false;
        }
      }

      /* This seems to be faster on Linux and OS X at least. */
      glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, srcFormat.layout(), srcFormat.type(), 0);

      assert(Utils::glCheck("Texture2D::loadBytes # 8"));

      if(data)
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, srcFormat.layout(), srcFormat.type(), data);

      assert(Utils::glCheck("Texture2D::loadBytes # 9"));
    }

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

//    float whitef[4] = { 1, 1, 1, 1 };
//    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, whitef);

    /* try not to interfere with other users of glTexImage2d etc */
    if (alignment > 4) glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    long & available = UploadLimiter::available();
    available -= consumesBytes();

    assert(Utils::glCheck("Texture2D::loadBytes # end"));

    return true;
  }

  void Texture2D::loadSubBytes(int x, int y, int w, int h, const void * data, const Luminous::PixelFormat & srcFormat)
  {
    assert(Utils::glCheck("Texture2D::loadSubBytes # start") == true);

    bind();

    if(m_haveMipmaps)
    {
      /// @todo mipmap support should be implemented
      error("Texture2D::loadSubBytes # Cannot be used with mipmaps");
    } else {

      int alignment = 1;

      while (alignment < 8) {
        if ((w * srcFormat.bytesPerPixel()) % (alignment*2)) {
          break;
        }

        alignment *= 2;
      }

      glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

      assert(Utils::glCheck("Texture2D::loadSubBytes # 1") == true);

      glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, srcFormat.layout(), srcFormat.type(), data);

      assert(Utils::glCheck("Texture2D::loadSubBytes # 2") == true);

      if (alignment > 4)
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

      assert(Utils::glCheck("Texture2D::loadSubBytes # 3") == true);

      // Update upload limits
      long & available = UploadLimiter::available();
      available -= w * h * srcFormat.bytesPerPixel();
    }

    assert(Utils::glCheck("Texture2D::loadSubBytes # end") == true);
  }

  Texture2D* Texture2D::fromImage
      (Luminous::Image & image, bool buildMipmaps, GLResources * resources)
  {
    return fromBytes(GL_RGBA, image.width(), image.height(), image.bytes(), image.pixelFormat(),
                     buildMipmaps, resources);
  }

  Texture2D * Texture2D::fromFile(const char * filename, bool buildMipmaps, GLResources * rs)
  {
    Luminous::Image img;
    if(!img.read(filename)) return 0;

    return fromImage(img, buildMipmaps, rs);
  }

  Texture2D* Texture2D::fromBytes(GLenum internalFormat, int w, int h,
                                  const void* data,
                                  const PixelFormat& srcFormat,
                                  bool buildMipmaps, GLResources * resources)
  {
    // Check dimensions
    if(!GL_ARB_texture_non_power_of_two) {
      bool isPowerOfTwo1 = !((w - 1) & w);
      bool isPowerOfTwo2 = !((h - 1) & h);

      if(!(isPowerOfTwo1 && isPowerOfTwo2)) {
        error("ERROR: non-power-of-two textures are not supported");
        return 0;
      }
    }

    Texture2D* tex = new Texture2D(resources);
    if(!tex->loadBytes(internalFormat, w, h, data, srcFormat, buildMipmaps)) {
      delete tex;
      return 0;
    }
    return tex;
  }

  bool Texture2D::progressiveUpload(Luminous::GLResources * resources, GLenum textureUnit, const Image & srcImage)
  {
    Utils::glCheck("Texture2D::progressiveUpload # begin");

    // Do nothing if already uploaded
    if(m_uploadedLines == size_t(height()))
      return true;

    // Get available bandwidth
    long & available = UploadLimiter::available();

    // Bandwidth may be negative if user has manually uploaded stuff
    if(available <= 0)
      return false;

    // Check how many lines we could upload
    /// @todo should estimate from texture pixel format, not image?
    const size_t lineBytes = width() * srcImage.pixelFormat().bytesPerPixel();
    const size_t lineBudget = available / lineBytes;

    // If we have no bandwidth for anything, abort
    if(lineBudget == 0)
      return false;

    // How many lines we need to upload to finish
    const size_t linesToFinish = height() - m_uploadedLines;

    assert(linesToFinish > 0);

    if(!resources)
      resources = GLResources::getThreadResources();

    // Bind the texture and draw zero-area triangle to flush the texture data
    bind(textureUnit);

    // Flush the data by drawing a zero-size triangle
    glColor4f(0,0,0,0);
    glBegin(GL_TRIANGLES);
    glVertex2f(0,0);
    glVertex2f(0,0);
    glVertex2f(0,0);
    glEnd();

    //Radiant::warning("ImageTex::progressiveUpload # lines to finish: %ld, line bytes: %ld, available bandwidth %ld, line budget: %ld", linesToFinish, lineBytes, available, lineBudget);

    assert(Utils::glCheck("Texture2D::progressiveUpload # upload beg") == true);

    size_t linesToUpload = std::min(linesToFinish, lineBudget);

    // Do progressive upload continuing from what has been previously uploaded

    // Compute source offset
    size_t offset = srcImage.width() * m_uploadedLines;
    const unsigned char * data = srcImage.data();
    data += offset * srcImage.pixelFormat().bytesPerPixel();

    loadSubBytes(0, m_uploadedLines, width(), linesToUpload, data, srcImage.pixelFormat());

    m_uploadedLines += linesToUpload;

    //Radiant::warning("Texture2D::progressiveUpload # uploaded %ld lines", linesToUpload);

    assert(Utils::glCheck("Texture2D::progressiveUpload # end") == true);

    return (m_uploadedLines == size_t(height()));
  }

}

