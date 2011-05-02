#ifndef VIVID_LIGHT_HPP
#define VIVID_LIGHT_HPP

#include <Valuable/HasValues.hpp>
#include <Valuable/ValueVector.hpp>
#include <Valuable/ValueFloat.hpp>
#include <Valuable/ValueColor.hpp>

namespace Vivid {

class Light : public Valuable::HasValues
{
public:
  enum Type
  {
    POINT_LIGHT = 0
  };
  Light();

private:
  Valuable::ValueInt m_lightType;
  Valuable::ValueVector3f m_position;
  Valuable::ValueColor m_color;
  Valuable::ValueFloat m_constantAttenuation;
  Valuable::ValueFloat m_linearAttenuation;
  Valuable::ValueFloat m_quadraticAttenuation;
};

} // namespace Vivid

#endif // LIGHT_HPP
