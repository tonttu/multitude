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

#ifndef LUMINOUS_LUMINOUS_HPP
#define LUMINOUS_LUMINOUS_HPP

#include <Luminous/Export.hpp>
#include <Radiant/Platform.hpp>
#include <Radiant/Trace.hpp>

#ifdef RADIANT_OSX
// We do not use GLEW on OSX
#define MULTI_WITHOUT_GLEW 1
#include <cstring>
#endif

#ifdef MULTI_WITHOUT_GLEW
// This define is for glext.h
# define GL_GLEXT_PROTOTYPES 1

# ifdef RADIANT_OSX
#  ifdef RADIANT_IOS
//#   include <OpenGLES/ES1/gl.h>
#   include <OpenGLES/ES2/gl.h>
#   ifdef LUMINOUS_COMPILE
//#    include <OpenGLES/ES1/glext.h>
#    include <OpenGLES/ES2/glext.h>
#   endif
#   define LUMINOUS_OPENGLES 1
#  else
// #   include <OpenGL/gl3.h>
#   include <OpenGL/gl.h>
#   include <OpenGL/glext.h>
#   include <OpenGL/glu.h>
#  endif
# else
#  define GL_GLEXT_PROTOTYPES 1
#  include <GL/gl.h>
#  include <GL/glext.h>
#  include <GL/glu.h>
# endif
#else
# include <GL/glew.h>
# if defined (RADIANT_WINDOWS)
#   include <GL/wglew.h>
# endif
#endif

#ifdef LUMINOUS_OPENGLES
# define LUMINOUS_IN_FULL_OPENGL(x)
#else
# define LUMINOUS_OPENGL_FULL
# define LUMINOUS_IN_FULL_OPENGL(x) x
#endif


#ifdef LUMINOUS_OPENGLES
# include <Luminous/DummyOpenGL.hpp>

# define glGenRenderbuffersEXT glGenRenderbuffers
# define glDeleteRenderbuffersEXT glDeleteRenderbuffers
# define glBindRenderbufferEXT glBindRenderbuffer
# define glRenderbufferStorageEXT glRenderbufferStorage
# define GL_RENDERBUFFER_EXT GL_RENDERBUFFER

# define glGenFramebuffersEXT glGenFramebuffers
# define glDeleteFramebuffersEXT glDeleteFramebuffers
# define glBindFramebufferEXT glBindFramebuffer
# define glCheckFramebufferStatusEXT glCheckFramebufferStatus
# define glFramebufferRenderbufferEXT glFramebufferRenderbuffer
# define glFramebufferRenderbufferEXT glFramebufferRenderbuffer
# define glFramebufferTexture2DEXT glFramebufferTexture2D

# define GL_FRAMEBUFFER_EXT GL_FRAMEBUFFER
# define GL_FRAMEBUFFER_COMPLETE_EXT GL_FRAMEBUFFER_COMPLETE
# define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT
# define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT
# define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
# define GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT GL_FRAMEBUFFER_INCOMPLETE_FORMATS
# define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
# define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
# define GL_FRAMEBUFFER_UNSUPPORTED_EXT GL_FRAMEBUFFER_UNSUPPORTED

#endif

#define debugLuminous(...) (Radiant::trace("Luminous", Radiant::DEBUG, __VA_ARGS__))
/// Luminous is a library of C++ classes for computer graphics, using OpenGL.
/** Copyright: The Luminous library has been developed in Helsinki
    Institute for Information Technology (HIIT, 2006-2008) and
    MultiTouch Oy (2007-2008).

    Luminous is released under the GNU Lesser General Public License
    (LGPL), version 2.1.
*/
namespace Luminous
{

  /** Initializes the Luminous library.
      In practice this function only initializes the GLEW and checks
      the capabilities of the underlying OpenGL implementation. If the
      OpenGL version is below 2.0, then a warning message is
      issued.

      @param initOpenGL if set to false, glew will not be initialized

      @return true if all relevant resources were successfully
      initialized, false if something was left missing (for example
      too low OpenGL version).
  */
  LUMINOUS_API bool initLuminous(bool initOpenGL = true);

  /** Initializes the default image codecs.
  The image codecs are loaded as plugins that need to be loaded before they can
  be used. This functions does just that.
  */
  LUMINOUS_API void initDefaultImageCodecs();


  //////////////////////////////////////////////////////////////////////////
  enum PrimitiveType
  {
    PrimitiveType_Triangle,
    PrimitiveType_TriangleStrip,
    PrimitiveType_Line,
    PrimitiveType_LineStrip,
    PrimitiveType_Point,
  };

  enum BufferType {
    BufferType_VertexBuffer,
    BufferType_IndexBuffer,
    BufferType_ConstantBuffer,
  };

  enum DataType
  {
    DataType_Unknown,
    DataType_Byte,
    DataType_Short,
    DataType_Int,
    DataType_UnsignedByte,
    DataType_UnsignedShort,
    DataType_UnsignedInt,
    DataType_Float,
    DataType_Double
  };
  
  enum BufferLockOptions {
    BufferLockOptions_Discard     = (1 << 0),
    BufferLockOptions_Read        = (1 << 1),
    BufferLockOptions_Write       = (1 << 2),
    BufferLockOptions_NoOverwrite = (1 << 3),
    BufferLockOptions_ReadWrite   = BufferLockOptions_Read | BufferLockOptions_Write,
  };

  enum ClearMask
  {
    ClearMask_Color              = (1 << 0),
    ClearMask_Depth              = (1 << 1),
    ClearMask_Stencil            = (1 << 2),
    ClearMask_ColorDepth         = ClearMask_Color | ClearMask_Depth,
    ClearMask_ColorStencil       = ClearMask_Color | ClearMask_Stencil,
    ClearMask_DepthStencil       = ClearMask_Depth | ClearMask_Stencil,
    ClearMask_ColorDepthStencil  = ClearMask_Color | ClearMask_Depth | ClearMask_Stencil,
  };

  enum ShaderType
  {
    ShaderType_VertexShader,
    ShaderType_FragmentShader,
    ShaderType_GeometryShader,
  };

  /// Resource types
  enum ResourceType {
    ResourceType_Unknown,
    ResourceType_VertexArray,
    ResourceType_ShaderProgram,
    ResourceType_VertexShader,
    ResourceType_FragmentShader,
    ResourceType_GeometryShader,
    ResourceType_Texture,
    ResourceType_TextureArray,
    ResourceType_Buffer,
  };

  /// Usage flags for HardwareBuffer objects
  enum BufferUsage
  {
    BufferUsage_Stream,
    BufferUsage_Static,    // 
    BufferUsage_Dynamic,
    BufferUsage_Copy,
  };

  /// CPU access permissions for HardwareBuffer objects
  enum BufferAccess
  {
    BufferAccess_Read,
    BufferAccess_Write,
  };
  //////////////////////////////////////////////////////////////////////////
  // Utility functions
  /// @todo Luminous2 utilities, should rename once Luminous::Utils has been killed with fire
  namespace Utils2
  {
    LUMINOUS_API size_t getDataSize(DataType type);
  }

  //////////////////////////////////////////////////////////////////////////
  /// Forward declarations

  // 
  class RenderDriver;
  // Resources
  class HardwareBuffer;
  class ShaderProgram;
  class ShaderGLSL;
  class Texture2;

  // Vertex data
  struct VertexAttribute;
  class VertexDescription;
  class VertexAttributeBinding;
}

#endif
