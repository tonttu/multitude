#include "GraphicsCoordinates.hpp"

namespace Valuable
{
  GraphicsCoordinates::GraphicsCoordinates(Valuable::Node * host, const QByteArray & name)
    : Node(host, name)
  {
    eventAddOut("graphics-bounds-changed");
  }

  GraphicsCoordinates::~GraphicsCoordinates()
  {
  }
}
