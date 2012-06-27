#if !defined (LUMINOUS_SHADERUNIFORM_HPP)
#define LUMINOUS_SHADERUNIFORM_HPP

#include "Luminous/Luminous.hpp"
#include <QString>
#include <vector>

namespace Luminous
{
  struct ShaderUniform
  {
    enum Type
    {
      Int,
      Int2,
      Int3,
      Int4,
      UnsignedInt,
      UnsignedInt2,
      UnsignedInt3,
      UnsignedInt4,
      Float,
      Float2,
      Float3,
      Float4,
      Float2x2,
      Float3x3,
      Float4x4,
    };

    QString name;
    Type type;
    std::vector<char> value;

    int index;        // Cached shader index location
  };
}
#endif // LUMINOUS_SHADERUNIFORM_HPP