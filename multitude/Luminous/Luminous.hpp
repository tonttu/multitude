/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_LUMINOUS_HPP
#define LUMINOUS_LUMINOUS_HPP

#include <Luminous/Export.hpp>

#include <Radiant/Platform.hpp>
#include <Radiant/Trace.hpp>

#include <QOpenGLFunctions_4_1_Core>
#include <QOpenGLFunctions_4_5_Core>

#define debugLuminous(...) (Radiant::trace("Luminous", Radiant::Trace::DEBUG, __VA_ARGS__))

/// Luminous is a library of C++ classes for computer graphics, using OpenGL.
/** Copyright: The Luminous library has been developed in Helsinki
    Institute for Information Technology (HIIT, 2006-2008) and
    MultiTouch Oy (2007-2008).

    Luminous is released under the GNU Lesser General Public License
    (LGPL), version 2.1.
*/
namespace Luminous
{
  /// Returns the OpenGL Core API version that is used by Cornerstone.
  /// This is the preferred version, while the version used by OpenGLAPI
  /// typedef is the minimum version.
  /// @return OpenGL version in major/minor format
  inline std::pair<int,int> preferredOpenGLVersion() { return {4, 5}; }

  /// This class will provide the minimum OpenGL API that is used by Cornerstone.
  typedef QOpenGLFunctions_4_1_Core OpenGLAPI;
  /// OpenGL 4.5 is used for additional optional functionality.
  typedef QOpenGLFunctions_4_5_Core OpenGLAPI45;

  /** Initializes the Luminous library.
      In practice this function initializes image codecs.
  */
  LUMINOUS_API void initLuminous();
  /** Initializes the Luminous library.

      This function should be called once for each rendering thread / context.


      @return true if all relevant resources were successfully
      initialized, false if something was left missing (for example
      too low OpenGL version).
  */
  LUMINOUS_API void initOpenGL(OpenGLAPI& opengl);
  /** Checks if the initLuminous function has been called successfully. */
  LUMINOUS_API bool isLuminousInitialized();

  /** Initializes the default image codecs.
  The image codecs are loaded as plugins that need to be loaded before they can
  be used. This functions does just that.
  */
  LUMINOUS_API void initDefaultImageCodecs();

  struct OpenGLVersion
  {
    QByteArray vendor;
    QByteArray version;
    QByteArray glsl;
    QByteArray renderer;

    bool operator==(const OpenGLVersion & b) const
    {
      return vendor == b.vendor && version == b.version &&
          glsl == b.glsl && renderer == b.renderer;
    }

    bool operator!=(const OpenGLVersion & b) const
    {
      return !operator==(b);
    }
  };

  /// Returns OpenGL version of the first OpenGL context that was initialized
  /// in initLuminous. This is thread-safe function.
  LUMINOUS_API OpenGLVersion glVersion();

  //////////////////////////////////////////////////////////////////////////

  /// Primitive type used for rendering
  enum PrimitiveType
  {
    /// Primitive corresponding to separate triangles
    PRIMITIVE_TRIANGLE       = GL_TRIANGLES,
    /// Primitive corresponding to triangle strips
    PRIMITIVE_TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
    /// Primitive corresponding to triangle fan
    PRIMITIVE_TRIANGLE_FAN   = GL_TRIANGLE_FAN,
    /// Primitive corresponding to line
    PRIMITIVE_LINE           = GL_LINES,
    /// Primitive corresponding to line strip
    PRIMITIVE_LINE_STRIP     = GL_LINE_STRIP,
    /// Primitive corresponding to points
    PRIMITIVE_POINT          = GL_POINTS
  };

  /// Mask to define which rendering buffers are cleared
  enum ClearMask
  {
    /// Indicates color buffer
    CLEARMASK_COLOR                = (1 << 0),
    /// Indicates depth buffer
    CLEARMASK_DEPTH                = (1 << 1),
    /// Indicates stencil buffer
    CLEARMASK_STENCIL              = (1 << 2),
    /// Shorthand for color and depth buffer
    CLEARMASK_COLOR_DEPTH          = CLEARMASK_COLOR | CLEARMASK_DEPTH,
    /// Shorthand for color and stencil buffer
    CLEARMASK_COLOR_STENCIL        = CLEARMASK_COLOR | CLEARMASK_STENCIL,
    /// Shorthand for depth and stencil buffer
    CLEARMASK_DEPTH_STENCIL        = CLEARMASK_DEPTH | CLEARMASK_STENCIL,
    /// Shorthand for color, depth, and stencil buffer
    CLEARMASK_COLOR_DEPTH_STENCIL  = CLEARMASK_COLOR | CLEARMASK_DEPTH | CLEARMASK_STENCIL
  };

  //////////////////////////////////////////////////////////////////////////
  /// Forward declarations

  class PixelFormat;
  // 
  class RenderDriver;
  class ContextArray;
  struct RenderCommand;
  class RenderContext;
  // Resources
  class Buffer;
  class BufferGL;
  class Program;
  class ProgramGL;
  class Shader;
  struct ShaderUniform;
  class Texture;
  class TextureGL;
  class TextLayout;
  class SimpleTextLayout;
  class RichTextLayout;

  // Vertex data
  struct VertexAttribute;
  class VertexDescription;
  class VertexArray;
  class VertexArrayGL;

  class Image;
  struct ImageInfo;
  class CompressedImage;

  class BlendMode;
  class DepthMode;
  class StencilMode;

  class RenderContext;
  class Style;
}

#endif
