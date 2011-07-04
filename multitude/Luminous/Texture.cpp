/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
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

namespace Luminous
{

  using namespace std;
  using namespace Radiant;

  UploadLimiter::UploadLimiter() : m_frame(0), m_frameLimit(1.5e6*60*4) {}

  long & UploadLimiter::available()
  {
    static RADIANT_TLS(int) t_frame(0);
    static RADIANT_TLS(long) t_available(0);

    UploadLimiter & i = instance();
    if(t_frame != i.m_frame) {
      t_frame = i.m_frame;
      t_available = i.m_frameLimit;
    }
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

  void UploadLimiter::processMessage(const char * type, Radiant::BinaryData &)
  {
    if(strcmp(type, "frame") == 0) ++m_frame;
  }

  UploadLimiter & UploadLimiter::instance()
  {
    static UploadLimiter s_limiter;
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

    long used = consumesBytes();

    long & available = UploadLimiter::available();

    if(available < used) return false;

    m_haveMipmaps = buildMipmaps;
    m_width = 1;
    m_height = h;
    /// @todo this is actually wrong, should convert from internalFormat really
    m_srcFormat = srcFormat;

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

    long uses = consumesBytes();
    available -= uses;

    changeByteConsumption(used, uses);

    return true;

  }


  bool Texture2D::loadImage(const char * filename, bool buildMipmaps) {
    Luminous::Image img;

    if(!img.read(filename)) return false;

    return loadImage(img, buildMipmaps);
  }

  /*
  bool Texture2D::loadImage(const char * filename, bool buildMipmaps)
  {
      Radiant::trace("Texture2D::LoadImage");
    try {
      Magick::Image im;
      Radiant::trace("MUUUUUUUUUUU %s", filename);
      im.read(filename);
      Radiant::trace("MOOOOOOOOO");
      if(im.columns()) {
        loadImage(im, buildMipmaps);
    return true;
      }
    }
    catch(Magick::Exception & e) {
        Radiant::error("Texture2D::loadImage # %s", e.what());
    } catch(...) {}
    return false;
  }
*/

  bool Texture2D::loadImage(const Luminous::Image & image, bool buildMipmaps, int internalFormat)
  {
    return loadBytes(internalFormat ? internalFormat : image.pixelFormat().numChannels(),
                     image.width(), image.height(),
                     image.bytes(),
                     image.pixelFormat(), buildMipmaps);
  }

  bool Texture2D::loadImage(const CompressedImage & image)
  {
    long & available = UploadLimiter::available();
    // Radiant::info("available: %.1f, need: %.1f", available/1024.0f, image.datasize()/1024.0f);

    if(available < image.datasize()) return false;

    long used = consumesBytes();

    m_internalFormat = image.compression();
    m_consumed = image.datasize();
    m_width = image.width();
    m_height = image.height();
    m_srcFormat = PixelFormat();
    m_haveMipmaps = false;

    bind();

    if(resources() && !resources()->isBrokenProxyTexture2D()) {
      glCompressedTexImage2D(GL_PROXY_TEXTURE_2D, 0, m_internalFormat,
                             m_width, m_height, 0, image.datasize(), 0);
      GLint width = m_width;
      glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);

      if(width == 0) {
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

    long uses = consumesBytes();
    available -= uses;

    changeByteConsumption(used, uses);

    m_loadedLines = m_height;
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

    long used = consumesBytes();

    long & available = UploadLimiter::available();

    if(available < used) return false;

    m_internalFormat = internalFormat;
    m_width = w;
    m_height = h;
    m_srcFormat = srcFormat;
    m_haveMipmaps = buildMipmaps;

    bind();

    // Default to bilinear filtering
    GLint minFilter = GL_LINEAR;
    GLint magFilter = GL_LINEAR;

    // set byte alignment to maximum possible: 1,2,4 or 8
    int alignment = 1;
    while (alignment < 8) {
      if ((m_width * m_srcFormat.bytesPerPixel()) % (alignment*2)) {
        break;
      }
      alignment *= 2;
    }


    // ...or trilinear if we have mipmaps
    if(buildMipmaps) minFilter = GL_LINEAR_MIPMAP_LINEAR;

    if(buildMipmaps) {
      glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
      gluBuild2DMipmaps(GL_TEXTURE_2D, internalFormat,
                        w, h, srcFormat.layout(), srcFormat.type(), data);
    } else {
      /* debugLuminous("TEXTURE UPLOAD :: INTERNAL %s FORMAT %s [%d %d]",
             glInternalFormatToString(internalFormat),
             glFormatToString(srcFormat.layout()), w, h);
      */

      glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);

      GLint width = w;

      if(resources() && !resources()->isBrokenProxyTexture2D()) {
        /* On ATI/Linux combination it seems that the GL_PROXY_TEXTURE_2D is
         broken, and cannot be trusted to give correct answers.
         It will at times fail with 1024x768 RGB textures. Sigh. */
        glTexImage2D(GL_PROXY_TEXTURE_2D, 0, internalFormat, w, h, 0,
                     srcFormat.layout(), srcFormat.type(), 0);

        glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);


        if(width == 0) {
          Radiant::error("Texture2D::loadBytes: Cannot load texture, too big? "
                         "(%d x %d, id = %.5d)", w, h, (int) id());

          glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
          return false;
        }
      }


      /* should succeed */

#if 0

      glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0,
                   srcFormat.layout(), srcFormat.type(), data);
#else
      /* This seems to be faster on Linux and OS X at least. */
      glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0,
                   srcFormat.layout(), srcFormat.type(), 0);

      if(data)
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
                        srcFormat.layout(), srcFormat.type(), data);
#endif

    }

    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

    float whitef[4] = { 1, 1, 1, 1 };

    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, whitef);

    long uses = consumesBytes();
    available -= uses;


    if(data)
      changeByteConsumption(used, uses);

    /* try not to interfere with other users of glTexImage2d etc */
    if (alignment > 4) glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    if(data)
      m_loadedLines = h;
    else
      m_loadedLines = 0;

    return true;
  }

  void Texture2D::loadSubBytes(int x, int y, int w, int h, const void * data)
  {
    bind();
    if(m_haveMipmaps)
    {
      error("Texture2D::loadSubBytes # Cannot be used with mipmaps");
      // @todo
    }
    else
    {
      int alignment = 1;
      while (alignment < 8) {
        if ((w * m_srcFormat.bytesPerPixel()) % (alignment*2)) {
          break;
        }
        alignment *= 2;
      }
      glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
      glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, m_srcFormat.layout(), m_srcFormat.type(), data);
      if (alignment > 4) glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
  }

  void Texture2D::loadLines(int y, int h, const void * data, const PixelFormat& pf)
  {
    Utils::glCheck("Texture2D::loadLines # in");
    bind();


    // Flush the data by drawing a zero-size triangle
    glColor4f(0,0,0,0);
    glBegin(GL_TRIANGLES);
    // info("BEGIN %.2lf", now.sinceSecondsD() * 1000);
    glVertex2f(0,0);
    glVertex2f(0,0);
    glVertex2f(0,0);
    //info("VERTI %.2lf", now.sinceSecondsD() * 1000);
    glEnd();

    Utils::glCheck("Texture2D::loadLines # Dummy TRI");

    if(m_haveMipmaps)
    {
      error("Texture2D::loadLines # Cannot be used with mipmaps");

      // @todo
    }
    else
    {
      int alignment = 1;
      while (alignment < 8) {
        if ((width() * pf.bytesPerPixel()) % (alignment*2)) {
          break;
        }
        alignment *= 2;
      }
      glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, width(), h, pf.layout(), pf.type(), data);
      if (alignment > 4) glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

      if(!Utils::glCheck("Texture2D::loadLines # TEX SUB"))
        error("PARAMS = %.5d %d %d %d", (int) id(), y, width(), h);
    }

    // Utils::glCheck("Texture2D::loadLines");

    if((unsigned) y == m_loadedLines)
      m_loadedLines += h;

    unsigned bytes = h * width() * pf.bytesPerPixel();

    changeByteConsumption(bytes, bytes);
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

}

