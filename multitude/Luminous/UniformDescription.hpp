#ifndef LUMINOUS_UNIFORM_DESCRIPTION_HPP
#define LUMINOUS_UNIFORM_DESCRIPTION_HPP

#include "Luminous/Luminous.hpp"

#include <QByteArray>
#include <vector>

namespace Luminous
{
  /// @cond
  class UniformDescription
  {
  public:
    LUMINOUS_API UniformDescription();

    struct UniformVar
    {
      QByteArray name;
      int offsetBytes;
      int sizeBytes;
    };

    LUMINOUS_API void addAttribute(const QByteArray & name, int offsetBytes, int sizeBytes);

    const std::vector<UniformVar> & attributes() const { return m_attributes; }

  private:
    std::vector<UniformVar> m_attributes;
  };
  /// @endcond
}

#endif // LUMINOUS_UNIFORM_DESCRIPTION_HPP
