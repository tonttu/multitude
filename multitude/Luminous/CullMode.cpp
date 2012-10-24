#include "CullMode.hpp"

namespace Luminous
{

CullMode::CullMode()
  : m_enabled(true)
  , m_face(Back)
{
}

CullMode::CullMode(bool enabled, Face face)
  : m_enabled(enabled)
  , m_face(face)
{}

}
