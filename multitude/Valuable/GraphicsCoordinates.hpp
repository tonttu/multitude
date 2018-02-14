#ifndef VALUABLE_GRAPHICS_COORDINATES_HPP
#define VALUABLE_GRAPHICS_COORDINATES_HPP

#include "Node.hpp"

#include <Nimble/Rect.hpp>

namespace Valuable
{
  /// API for converting between application and screen coordinate systems
  /// @event[out] graphics-bounds-changed
  class VALUABLE_API GraphicsCoordinates : public Node
  {
  public:
    /// Result of coordinate system transformation, see graphicsToDesktop
    struct DesktopPoint
    {
      /// X Screen number, or -1. See Luminous::MultiHead::Window::screennumber
      int screennumber = -1;
      /// Location in desktop coordinates
      Nimble::Vector2f location{0, 0};
      /// True if the query was inside of any of the windows
      bool isInside = false;
    };

    /// Result of coordinate system transformation, see desktopToGraphics
    struct GraphicsPoint
    {
      /// Location in graphics coordinates
      Nimble::Vector2f location{0, 0};
      /// True if the query was inside of any of the windows
      bool isInside = false;
    };

  public:
    GraphicsCoordinates(Node * host, const QByteArray & name);
    virtual ~GraphicsCoordinates();

    /// Graphics bounds for the whole application
    virtual Nimble::Rect graphicsBounds() const = 0;

    /// Converts graphics coordinate to operating system desktop coordinates.
    /// @sa Area::GraphicsToWindow
    virtual DesktopPoint graphicsToDesktop(Nimble::Vector2f loc) const = 0;

    /// Converts operating system desktop coordinates to graphics coordinates.
    virtual GraphicsPoint desktopToGraphics(Nimble::Vector2f loc, int screenNumber = -1) const = 0;
  };
}

#endif
