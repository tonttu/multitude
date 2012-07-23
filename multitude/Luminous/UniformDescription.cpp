#include "UniformDescription.hpp"

namespace Luminous
{
  UniformDescription::UniformDescription()
  {
  }

  void UniformDescription::addAttribute(const QByteArray & name, int offsetBytes, int sizeBytes)
  {
    UniformVar var;
    var.name = name;
    var.offsetBytes = offsetBytes;
    var.sizeBytes = sizeBytes;
    m_attributes.push_back(std::move(var));
  }
}
