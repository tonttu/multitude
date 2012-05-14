#if !defined (LUMINOUS_SHADERCONSTANT_HPP)
#define LUMINOUS_SHADERCONSTANT_HPP

#include "Luminous.hpp"
#include <QString>

namespace Luminous
{
  struct ShaderConstant
  {
    QString name;
    // Can't use Vector/Matrix types because they aren't PODs
    union {
      char b[16];
      short s[16];
      int i[16];
      float f[16];
      double d[16];
    } value;

    Luminous::DataType type;
    uint8_t count;
  };
  inline bool operator==(const ShaderConstant & lhs, const QString & name) { return lhs.name == name; }
  inline bool operator!=(const ShaderConstant & lhs, const QString & name) { return !(lhs == name); }
}
#endif // LUMINOUS_SHADERCONSTANT_HPP
