#ifndef VIVID_LIGHT_HPP
#define VIVID_LIGHT_HPP

#include <Valuable/Node.hpp>
#include <Valuable/AttributeVector.hpp>
#include <Valuable/AttributeFloat.hpp>
#include <Valuable/AttributeColor.hpp>

namespace Vivid {

class Light : public Valuable::Node
{
public:
  enum Type
  {
    POINT_LIGHT = 0
  };
  Light();

private:
  Valuable::AttributeInt m_lightType;
  Valuable::AttributeVector3f m_position;
  Valuable::AttributeColor m_color;
  Valuable::AttributeFloat m_constantAttenuation;
  Valuable::AttributeFloat m_linearAttenuation;
  Valuable::AttributeFloat m_quadraticAttenuation;
};

} // namespace Vivid

#endif // LIGHT_HPP
