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

#include "Error.hpp"

#include <Radiant/Trace.hpp>
#include <Radiant/Mutex.hpp>

#include <QMap>

// OS X (Core 3.2) doesn't have these
#if defined(RADIANT_OSX)
#  define GL_STACK_OVERFLOW 0x0503
#  define GL_STACK_UNDERFLOW 0x504
#  define GL_TABLE_TOO_LARGE 0x8031
#endif

namespace Luminous
{

void glErrorToString(const QString & msg, int line)
{
    static QMap<GLuint, QString> errors;

    MULTI_ONCE {

        errors.insert(GL_NO_ERROR, "no error");
        errors.insert(GL_INVALID_ENUM, "invalid enumerant");
        errors.insert(GL_INVALID_VALUE, "invalid value");
        errors.insert(GL_INVALID_OPERATION, "invalid operation");
        errors.insert(GL_STACK_OVERFLOW, "stack overflow");
        errors.insert(GL_STACK_UNDERFLOW, "stack underflow");
        errors.insert(GL_OUT_OF_MEMORY, "out of memory");
        errors.insert(GL_TABLE_TOO_LARGE, "table too large");
        errors.insert(GL_INVALID_FRAMEBUFFER_OPERATION, "invalid framebuffer operation");

    }

    GLenum err, err2 = GL_NO_ERROR;
    while((err = glGetError()) != GL_NO_ERROR) {
      // If glGetError ever returns the same error twice, it's broken somehow.
      // This happens when called without GL context etc.
      if (err == err2) {
        Radiant::error("%s # glGetError called with broken OpenGL context", msg.toUtf8().data());
        return;
      }
      err2 = err;
      Radiant::error("%s:%d: %s", msg.toUtf8().data(), line, errors.value(err).toUtf8().data());
    };
}
/*
  const char * glInternalFormatToString(GLint format)
  {
    switch(format) {
      case GL_ALPHA:
        return "GL_ALPHA";
    case GL_LUMINANCE:
      return "GL_LUMINANCE";
    case GL_RGB:
      return "GL_RGB";
    case GL_RGBA:
      return "GL_RGBA";

#ifndef LUMINOUS_OPENGLES
      case GL_ALPHA4:
        return "GL_ALPHA4";
      case GL_ALPHA8:
      case GL_ALPHA12:
        return "GL_ALPHA12";
      case GL_ALPHA16:
        return "GL_ALPHA16";
      case GL_COMPRESSED_ALPHA:
        return "GL_COMPRESSED_ALPHA";
      case GL_COMPRESSED_LUMINANCE:
        return "GL_COMPRESSED_LUMINANCE";
      case GL_COMPRESSED_LUMINANCE_ALPHA:
        return "GL_COMPRESSED_LUMINANCE_ALPHA";
      case GL_COMPRESSED_INTENSITY:
        return "GL_COMPRESSED_INTENSITY";
      case GL_COMPRESSED_RGB:
        return "GL_COMPRESSED_RGB";
      case GL_COMPRESSED_RGBA:
        return "GL_COMPRESSED_RGBA";
      case GL_DEPTH_COMPONENT:
        return "GL_DEPTH_COMPONENT";
      case GL_DEPTH_COMPONENT16:
        return "GL_DEPTH_COMPONENT16";
      case GL_DEPTH_COMPONENT24:
        return "GL_DEPTH_COMPONENT24";
      case GL_DEPTH_COMPONENT32:
        return "GL_DEPTH_COMPONENT32";
    case GL_INTENSITY:
      return "GL_INTENSITY";

      case GL_LUMINANCE4:
        return "GL_LUMINANCE4";
      case GL_LUMINANCE8:
        return "GL_LUMINANCE8";
      case GL_LUMINANCE12:
        return "GL_LUMINANCE12";
      case GL_LUMINANCE16:
        return "GL_LUMINANCE16";
      case GL_LUMINANCE_ALPHA:
        return "GL_LUMINANCE_ALPHA";
      case GL_LUMINANCE4_ALPHA4:
        return "GL_LUMINANCE4_ALPHA4";
      case GL_LUMINANCE6_ALPHA2:
        return "GL_LUMINANCE6_ALPHA2";
      case GL_LUMINANCE8_ALPHA8:
        return "GL_LUMINANCE8_ALPHA8";
      case GL_LUMINANCE12_ALPHA4:
        return "GL_LUMINANCE12_ALPHA4";
      case GL_LUMINANCE12_ALPHA12:
        return "GL_LUMINANCE12_ALPHA12";
      case GL_LUMINANCE16_ALPHA16:
        return "GL_LUMINANCE16_ALPHA16";
      case GL_INTENSITY4:
        return "GL_INTENSITY4";
      case GL_INTENSITY8:
        return "GL_INTENSITY8";
      case GL_INTENSITY12:
        return "GL_INTENSITY12";
      case GL_INTENSITY16:
        return "GL_INTENSITY16";
      case GL_R3_G3_B2:
        return "GL_R3_G3_B2";
      case GL_RGB4:
        return "GL_RGB4";
      case GL_RGB5:
        return "GL_RGB5";
      case GL_RGB8:
        return "GL_RGB8";
      case GL_RGB10:
        return "GL_RGB10";
      case GL_RGB12:
        return "GL_RGB12";
      case GL_RGB16:
        return "GL_RGB16";
      case GL_RGBA2:
        return "GL_RGBA2";
      case GL_RGBA4:
        return "GL_RGBA4";
      case GL_RGB5_A1:
        return "GL_RGB5_A1";
      case GL_RGBA8:
        return "GL_RGBA8";
      case GL_RGB10_A2:
        return "GL_RGB10_A2";
      case GL_RGBA12:
        return "GL_RGBA12";
      case GL_RGBA16:
        return "GL_RGBA16";
      case GL_SLUMINANCE:
        return "GL_SLUMINANCE";
      case GL_SLUMINANCE8:
        return "GL_SLUMINANCE8";
      case GL_SLUMINANCE_ALPHA:
        return "GL_SLUMINANCE_ALPHA";
      case GL_SLUMINANCE8_ALPHA8:
        return "GL_SLUMINANCE8_ALPHA8";
      case GL_SRGB:
        return "GL_SRGB";
      case GL_SRGB8:
        return "GL_SRGB8";
      case GL_SRGB_ALPHA:
        return "GL_SRGB_ALPHA";
      case GL_SRGB8_ALPHA8:
        return "GL_SRGB8_ALPHA8";
#endif // LUMINOUS_OPENGLES
      default:
        return "Incorrect internal format";
    }
  }

  const char * glFormatToString(GLenum format)
  {
    switch(format)
    {
    case GL_RGB:
      return "GL_RGB";
    case GL_RGBA:
      return "GL_RGBA";
    case GL_LUMINANCE:
      return "GL_LUMINANCE";
    case GL_LUMINANCE_ALPHA:
      return "GL_LUMINANCE_ALPHA";
    case GL_ALPHA:
      return "GL_ALPHA";

#ifndef LUMINOUS_OPENGLES

      case GL_COLOR_INDEX:
        return "GL_CLOR_INDEX";
      case GL_RED:
        return "GL_RED";
      case GL_GREEN:
        return "GL_GREEN";
      case GL_BLUE:
        return "GL_BLUE";
      case GL_BGR:
        return "GL_BGR";
      case GL_BGRA:
        return "GL_BGRA";
#endif // LUMINOUS_OPENGLES

      default:
        return "Invalid format";
    }
  }
*/
}