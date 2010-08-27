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
#include "Error.hpp"

#include <Radiant/Trace.hpp>


namespace Luminous
{

  using namespace std;
  using namespace Radiant;

  template <GLenum TextureType>
  TextureT<TextureType>::~TextureT()
  {
    if(m_textureId) glDeleteTextures(1, &m_textureId);
    changeByteConsumption(consumesBytes(), 0);
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

  Texture1D* Texture1D::fromBytes(GLenum internalFormat, int h,
                  const void* data,
                  const PixelFormat& srcFormat,
                  bool buildMipmaps, GLResources * resources)
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
        cerr << "ERROR: non-power-of-two textures are not supported" << endl;
        return false;
      }
    }


    m_haveMipmaps = buildMipmaps;
    m_width = 1;
    m_height = h;
    /// @todo this is actually wrong, should convert from internalFormat really
    m_pf = srcFormat;

    bind();

    // Default to bilinear filtering
    GLint minFilter = GL_LINEAR;
    GLint magFilter = GL_LINEAR;

    // ...or trilinear if we have mipmaps
    if(buildMipmaps) minFilter = GL_LINEAR_MIPMAP_LINEAR;

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, magFilter);

    glTexImage1D(GL_TEXTURE_1D, 0, internalFormat, h, 0,
                 srcFormat.layout(), srcFormat.type(), data);

    if(buildMipmaps) {
      gluBuild1DMipmaps(GL_TEXTURE_1D,
            srcFormat.numChannels(), h,
            srcFormat.layout(), srcFormat.type(), data);
    }

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

  bool Texture2D::loadImage(const Luminous::Image & image, bool buildMipmaps)
  {
    return loadBytes(image.pixelFormat().layout(),
             image.width(), image.height(),
             image.bytes(),
             image.pixelFormat(), buildMipmaps);
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
        cerr << "ERROR: non-power-of-two textures are not supported" << endl;
        return false;
      }
    }

    long used = consumesBytes();

    m_width = w;
    m_height = h;
    m_pf = srcFormat;
    m_haveMipmaps = buildMipmaps;

    bind();

    // Default to bilinear filtering
    GLint minFilter = GL_LINEAR;
    GLint magFilter = GL_LINEAR;

    // set byte alignment to maximum possible: 1,2,4 or 8
    int alignment = 1;
    while (alignment < 8) {
      if ((m_width * m_pf.bytesPerPixel()) % (alignment*2)) {
        break;
      }
      alignment *= 2;
    }


    // ...or trilinear if we have mipmaps
    if(buildMipmaps) minFilter = GL_LINEAR_MIPMAP_LINEAR;

    if(buildMipmaps) {
      glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
      gluBuild2DMipmaps(GL_TEXTURE_2D, srcFormat.numChannels(),
            w, h, srcFormat.layout(), srcFormat.type(), data);
    } else {
      /* Radiant::debug("TEXTURE UPLOAD :: INTERNAL %s FORMAT %s [%d %d]",
             glInternalFormatToString(internalFormat),
             glFormatToString(srcFormat.layout()), w, h);
      */

      GLint width;
      glTexImage2D(GL_PROXY_TEXTURE_2D, 0, internalFormat, w, h, 0,
           srcFormat.layout(), srcFormat.type(), 0);

      glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);

      if(width == 0) {
        Radiant::error("Texture2D::loadBytes: Cannot load texture, too big? (%d x %d)", w, h);
        return false;
      } else {
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        /* should succeed */

#if 0

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0,
                     srcFormat.layout(), srcFormat.type(), data);
#else
        /* This seems to be faster on Linux and OSX at least. */
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0,
                     srcFormat.layout(), srcFormat.type(), 0);

        if(data)
          glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
                          srcFormat.layout(), srcFormat.type(), data);
#endif
      }
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
        if ((w * m_pf.bytesPerPixel()) % (alignment*2)) {
          break;
        }
        alignment *= 2;
      }
      glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
      glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, m_pf.layout(), m_pf.type(), data);
      if (alignment > 4) glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    }
  }

  void Texture2D::loadLines(int y, int h, const void * data, const PixelFormat& pf)
  {
    // Utils::glCheck("Texture2D::loadLines # in");
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
        cerr << "ERROR: non-power-of-two textures are not supported" << endl;
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

