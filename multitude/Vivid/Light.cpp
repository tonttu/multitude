#include "Light.hpp"

namespace Vivid {

Light::Light()
  : m_lightType(this, "light-type", POINT_LIGHT),
  m_position(this, "position", Nimble::Vector3(0, 0, 0)),
  m_color(this, "color", Radiant::Color(1.0f, 1.0f, 1.0f, 1.0f)),
  m_constantAttenuation(this, "constant-attenuation", 1.0f),
  m_linearAttenuation(this, "linear-attenuation", 0.01f),
  m_quadraticAttenuation(this, "quadratic-attenuation", 0.001f)

{

}

} // namespace Vivid
