/* COPYRIGHT
 */

#include "VertexBufferImpl.hpp"

namespace Luminous
{

  template class BufferObject<GL_ARRAY_BUFFER>;
  template class BufferObject<GL_ELEMENT_ARRAY_BUFFER>;
  LUMINOUS_IN_FULL_OPENGL(template class BufferObject<GL_PIXEL_PACK_BUFFER>);

}
