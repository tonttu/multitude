/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */


#ifndef LUMINOUS_MULTIHEAD_HPP
#define LUMINOUS_MULTIHEAD_HPP

#include <Luminous/Collectable.hpp>
#include <Luminous/Export.hpp>
#include <Luminous/GLKeyStone.hpp>

#include <Nimble/Rect.hpp>
#include <Nimble/Vector4.hpp>

#include <Radiant/RefPtr.hpp>

#include <Valuable/ValueString.hpp>
#include <Valuable/ValueFloat.hpp>

#include <vector>

namespace Luminous {

  using Nimble::Rect;
  using Nimble::Vector2f;
  using Nimble::Vector2i;
  using Nimble::Vector4f;

  /// Class for managing information on multiple OpenGL vindows/viewports.
  /** This class stores information about the layout of multiple
      OpenGL windows and viewports. This information is used in
      systems that render OpenGL into several windows at
      once. MultiHead also includes keystone information, so that
      keystoning can be done by using skewed OpenGL transformation
      matrices. */
  class MultiHead : public Valuable::HasValues
  {
  public:

    class Window;

    /// One OpenGL area
    /** Areas are roughly equivalent to OpenGL viewports. Multiple
    areas can share the same OpenGL context, as one window can
    have may areas inside it.*/
    /// @todo rename to ViewPort?
    class Area : public Valuable::HasValues,
    public Collectable
    {
    public:
      LUMINOUS_API Area(Window * window = 0);
      LUMINOUS_API virtual ~Area();

      LUMINOUS_API bool deserializeXML(Valuable::DOMElement element);

      void setGeometry(int x, int y, int w, int h, bool copyToGraphics = true)
      {
        m_location = Nimble::Vector2i(x, y);
        m_size = Nimble::Vector2i(w, h);

        if(copyToGraphics)
          setGraphicsGeometry(x, y, w, h);
      }

      void setSize(Vector2i size)
      {
        m_size = size;
      }

      void setGraphicsGeometry(int x, int y, int w, int h)
      {
        m_graphicsLocation = Nimble::Vector2i(x, y);
        m_graphicsSize = Nimble::Vector2i(w, h);
        updateBBox();
      }

      void setSeams(float left, float right, float bottom, float top)
      {
        m_seams = Nimble::Vector4f(left, right, bottom, top);
        updateBBox();
      }

      float maxSeam() { return m_seams.asVector().maximum(); }

      LUMINOUS_API void applyGlState() const;
      LUMINOUS_API void cleanEdges() const;

      virtual const char * type() const { return "area"; }

      GLKeyStone & keyStone() { return m_keyStone; }
      const GLKeyStone & keyStone() const { return m_keyStone; }

      /// The pixel location of the area on the window
      const Vector2i & location() const { return m_location.asVector(); }
      int x() const { return m_location[0]; }
      int y() const { return m_location[1]; }
      /// The pixel size of the area on the window
      const Vector2i & size() const { return m_size.asVector(); }
      int width() const { return m_size[0]; }
      int height() const { return m_size[1]; }

      /// The offset of the graphics inside the area (virtual pixels)
      const Vector2f graphicsLocation(bool withseams = true) const
      {
        return withseams ?
            m_graphicsLocation.asVector() - Nimble::Vector2f(m_seams[0], m_seams[3]) :
            m_graphicsLocation.asVector();
      }
      /// The size of the graphics inside this area (virtual pixels)
      const Vector2f graphicsSize(bool withseams = true) const
      {
        return withseams ?
            m_graphicsSize.asVector() + Nimble::Vector2f(m_seams[0] + m_seams[1],
                                                         m_seams[2] + m_seams[3]) :
            m_graphicsSize.asVector();
      }

      /// The bounds of the graphics
      /** In principle this method only combines the information you
          get with graphicsLocation() and graphicsSize(). In practice
          how-ever that information is not exactly correct as the
          bounds also need to include the small extra areas, if one is
          using edge-blending. */
      LUMINOUS_API const Rect & graphicsBounds() const
      { return m_graphicsBounds; }

      /** Converts a screen-space coordinate to image-space coordinate.
          @param loc The location vector to convert.
          @param windowheight height of the window
          @param insideArea set to true if the location is inside this
      area. Otherwise convOk is set to
          false.
          @return The vector in graphics coordinates.
      */
      LUMINOUS_API Nimble::Vector2f windowToGraphics(Nimble::Vector2f loc, int windowheight, bool & insideArea) const;

      int active() const { return m_active.asInt(); }

      void setPixelSizeCm(float sizeCm) { m_pixelSizeCm = sizeCm; }
      float pixelSizeCm() const { return m_pixelSizeCm; }
      float cmToPixels(float cm) { return cm / m_pixelSizeCm; }

      Nimble::Matrix3 viewTransform();

      void swapGraphicsWidthHeight()
      {
        m_graphicsSize = m_graphicsSize.asVector().shuffle();
        updateBBox();
      }

    private:

      enum {
        /* Render to the screen, using straight coordinates. Then
       read-back and re-render, with skewed coordinates. */
        METHOD_TEXTURE_READBACK,
        /* Render directly with skewed coordinates. Nice for
       performance, but a bit tricky for ripple effects etc. */
        METHOD_MATRIX_TRICK
      };

      LUMINOUS_API void updateBBox();

      Window * m_window;
      GLKeyStone m_keyStone;
      Valuable::ValueVector2i   m_location;
      Valuable::ValueVector2i   m_size;
      Valuable::ValueVector2f   m_graphicsLocation;
      Valuable::ValueVector2f   m_graphicsSize;
      Valuable::ValueVector4f   m_seams;
      Valuable::ValueInt        m_active;
      Valuable::ValueInt        m_method;
      Valuable::ValueString m_comment;
      Rect m_graphicsBounds;
      float      m_pixelSizeCm;
    };

    /// One OpenGL window.
    /** A window is responsible for one OpenGL context. */
    class Window : public Valuable::HasValues
    {

    public:
      friend class Area;

      LUMINOUS_API Window(MultiHead * screen = 0);
      LUMINOUS_API ~Window();

      const char * type() const { return "window"; }

      /// Set the location and size of this window
      void setGeometry(int x, int y, int w, int h)
      {
        m_location = Nimble::Vector2i(x, y);
        m_size = Nimble::Vector2i(w, h);
      }

      /// Resize the window, and automatically one child area
      /** This method is used when the window contains only one child
          area, and automatially changes the size of the area to match
          the area of the window. */

      LUMINOUS_API void resizeEvent(Vector2i size);

      /// Number of areas that this window holds
      size_t areaCount() const { return m_areas.size(); }
      /// Get one of the areas
      Area & area(size_t i) { return * m_areas[i].ptr(); }
      /// Get one of the areas
      const Area & area(size_t i) const { return * m_areas[i].ptr(); }

      LUMINOUS_API Nimble::Rect graphicsBounds() const;

      LUMINOUS_API void setSeam(float seam);

      void addArea(Area * a) { m_areas.push_back(a); }

      /// Location of the window on the computer display
      const Vector2i & location() const { return m_location.asVector(); }
      /// Size of the window on the computer display
      const Vector2i & size() const { return m_size.asVector(); }

      /** Convert a coordinate from screen to graphics coordinates.
          This class traverses through all the areas to find an area
          that would include given location. It is entirely possible
          none of the areas contains the given coordinate, as there
          can be gaps between areas (due to keystoning, for example).

          @param loc The location in screen coordinates.

          @param convOk set to true or false depending on whether the
          conversion could be carried out.
      */
      LUMINOUS_API Nimble::Vector2f windowToGraphics(Nimble::Vector2f loc, bool & convOk) const;

      /// Should the window be frameless
      bool frameless() const { return ((m_frameless.asInt() == 0) ? false : true); }
      /// Should the window be full-screen
      bool fullscreen() const { return ((m_fullscreen.asInt() == 0) ? false : true); }
      /// Should the window be resizeable
      bool resizeable() const { return ((m_resizeable.asInt() == 0) ? false : true); }

      LUMINOUS_API void setPixelSizeCm(float sizeCm);

    protected:
      LUMINOUS_API virtual bool readElement(Valuable::DOMElement ce);

      MultiHead                *m_screen;
      Valuable::ValueVector2i   m_location;
      Valuable::ValueVector2i   m_size;
      Valuable::ValueInt        m_frameless;
      Valuable::ValueInt        m_fullscreen;
      Valuable::ValueInt        m_resizeable;
      /// Pixel size in centimeters
      float      m_pixelSizeCm;

      std::vector<Radiant::RefPtr<Area> > m_areas;
    };

    LUMINOUS_API MultiHead();
    LUMINOUS_API virtual ~MultiHead();

    /// The number of areas
    LUMINOUS_API size_t areaCount();
    /// Access the areas
    /** This method traverses all the windows to find the area with
    given index. */
    LUMINOUS_API Area & area(size_t i, Window ** winptr = 0);
    /// Create 1 window with 1 area.
    LUMINOUS_API void makeSingle(int x, int y, int w, int h);
    /// Create 1 window with 2 areas horizontally side by side.
    LUMINOUS_API void makeDouble(int x, int y, int w, int h, float seam);
    /// Create 1 window with 4 rotated areas
    /** This is useful for making a setup with four
        projectors/displays that are aligned in portrait mode. */
    LUMINOUS_API void makeQuadSideways(int x, int y, int w, int h, float seam);

    /// The number of windows
    size_t windowCount() const { return m_windows.size(); }
    /// Access one of the windows
    LUMINOUS_API Window & window(size_t i);
    LUMINOUS_API const Window & window(size_t i) const;

    /// Total size of all the windows
    LUMINOUS_API Nimble::Vector2i totalSize();

    LUMINOUS_API Rect graphicsBounds() const;


    Nimble::Vector2i size()
    { return Nimble::Vector2i(width(), height()); }
    LUMINOUS_API float seam();
    LUMINOUS_API void setSeam(float seam);

    /** Total width of the display area, in graphics pixels. */
    LUMINOUS_API int width();
    /** Total height of the display area, in graphics pixels. */
    LUMINOUS_API int height();

    LUMINOUS_API bool deserializeXML(Valuable::DOMElement element);

    void addWindow(Window * w) { m_windows.push_back(w); }

    void setEdited(bool edited) { m_edited = edited; }
    bool isEdited() const { return m_edited; }

    float gamma() const { return m_gamma; }

  protected:
    LUMINOUS_API virtual bool readElement(Valuable::DOMElement ce);

    std::vector<Radiant::RefPtr<Window> > m_windows;
    Valuable::ValueFloat m_widthcm;
    Valuable::ValueFloat m_gamma;
    bool m_edited;
  };

}

#endif
