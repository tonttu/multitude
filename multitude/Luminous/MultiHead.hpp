/* COPYRIGHT
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

    /** One OpenGL area
    Areas are roughly equivalent to OpenGL viewports. Multiple
    areas can share the same OpenGL context, as one window can
    have may areas inside it.*/
    /// @todo rename to ViewPort?
    class Area : public Valuable::HasValues,
    public Collectable
    {
    public:
      /// Constructs a new area for the given window
      LUMINOUS_API Area(Window * window = 0);
      LUMINOUS_API virtual ~Area();
      /// Deserializes this area from an archive element
      LUMINOUS_API bool deserialize(Valuable::ArchiveElement & element);

      /// Sets the geometry (size & offset) of the area
      /// @param x x offset
      /// @param y y offset
      /// @param w width
      /// @param h height
      /// @param copyToGraphics if true, the settings are copied to graphics coordinates
      void setGeometry(int x, int y, int w, int h, bool copyToGraphics = true)
      {
        m_location = Nimble::Vector2i(x, y);
        m_size = Nimble::Vector2i(w, h);

        if(copyToGraphics)
          setGraphicsGeometry(x, y, w, h);
      }

      /// Sets the size of the area
      void setSize(Vector2i size)
      {
        m_size = size;
      }

      /// Sets the graphics geometry of the area
      void setGraphicsGeometry(int x, int y, int w, int h)
      {
        m_graphicsLocation = Nimble::Vector2i(x, y);
        m_graphicsSize = Nimble::Vector2i(w, h);
        updateBBox();
      }

      /// Sets the seams for the area
      void setSeams(float left, float right, float bottom, float top)
      {
        m_seams = Nimble::Vector4f(left, right, bottom, top);
        updateBBox();
      }

      /// Returns the width of the largest seam
      float maxSeam() { return m_seams.asVector().maximum(); }

      /// Applies an orthogonal projection to OpenGL defined by the graphics geometry of the area
      LUMINOUS_API void applyGlState() const;
      /// Blends the edges defined by seams
      LUMINOUS_API void cleanEdges() const;

      /// Returns the type name for areas (="area").
      virtual const char * type() const { return "area"; }

      /// Returns the keystone correction
      GLKeyStone & keyStone() { return m_keyStone; }
      /// @copydoc keyStone
      const GLKeyStone & keyStone() const { return m_keyStone; }

      /// The pixel location of the area on the window
      const Vector2i & location() const { return m_location.asVector(); }
      /// The coordinate of the pixel location
      int x() const { return m_location[0]; }
      /// The coordinate of the pixel location
      int y() const { return m_location[1]; }
      /// The pixel size of the area on the window
      const Vector2i & size() const { return m_size.asVector(); }
      /// The width of the area in the window
      int width() const { return m_size[0]; }
      /// The width of the area in the window
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
      const Rect & graphicsBounds() const
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

      /// Returns true if the area is active (graphics will be drawn to it)
      int active() const { return m_active.asInt(); }

      /// Sets the width of a single pixel in centimeters
      void setPixelSizeCm(float sizeCm) { m_pixelSizeCm = sizeCm; }
      /// Returns the width of a single pixel in centimeters
      float pixelSizeCm() const { return m_pixelSizeCm; }
      /// Converts centimeters to pixels
      float cmToPixels(float cm) { return cm / m_pixelSizeCm; }

      /// Returns the view transform matrix
      Nimble::Matrix3 viewTransform();

      /// Swaps the width and height of the graphics size
      void swapGraphicsWidthHeight()
      {
        m_graphicsSize = m_graphicsSize.asVector().shuffle();
        updateBBox();
      }

      /// Returns a pointer to the window that holds this area
      const Window * window() const { return m_window; }

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

    /** One OpenGL window.
    A window is responsible for one OpenGL context.*/
    class Window : public Valuable::HasValues
    {
    public:
      friend class Area;

      /// Constructs a new window for the given screen
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
          the area of the window.
      @param size new window size */
      LUMINOUS_API void resizeEvent(Vector2i size);

      /// Number of areas that this window holds
      size_t areaCount() const { return m_areas.size(); }
      /// Get one of the areas
      Area & area(size_t i) { return * m_areas[i].get(); }
      /// Get one of the areas
      const Area & area(size_t i) const { return * m_areas[i].get(); }

      /// Returns the union of the areas' graphics bounds
      LUMINOUS_API Nimble::Rect graphicsBounds() const;

      /// Sets the seam for each area
      LUMINOUS_API void setSeam(float seam);

      /// Adds an area to the window
      void addArea(Area * a) { m_areas.push_back(std::shared_ptr<Area>(a)); }

      /// Location of the window on the computer display
      const Vector2i & location() const { return m_location.asVector(); }
      /// Size of the window on the computer display
      const Vector2i & size() const { return m_size.asVector(); }

      /// Returns the width of this window in pixels
      int width() const { return m_size.x(); }
      /// Returns the height of this window in pixels
      int height() const { return m_size.y(); }

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
      /// Sets the fullscreen flag
      void setFullscreen(bool f) { m_fullscreen = f; }
      /// Should the window be resizeable
      bool resizeable() const { return ((m_resizeable.asInt() == 0) ? false : true); }

      /// X11 display number for threaded rendering, -1 if not specified
      int displaynumber() const { return m_displaynumber.asInt(); }
      /// Sets X11 display number for threaded rendering, -1 disables
      void setSisplaynumber(int s) { m_displaynumber = s; }
      /// X11 screen number for threaded rendering, -1 if not specified
      int screennumber() const { return m_screennumber.asInt(); }
      /// Sets X11 screen number for threaded rendering, -1 disables
      void setScreennumber(int s) { m_screennumber = s; }

      /// Sets the width of a pixel in centimeters to each area inside this window
      LUMINOUS_API void setPixelSizeCm(float sizeCm);

      /// Return the screen configuration that this Window belongs to
      const MultiHead * screen() const { return m_screen; }

    private:
      LUMINOUS_API virtual bool readElement(Valuable::DOMElement ce);

      MultiHead                *m_screen;
      Valuable::ValueVector2i   m_location;
      Valuable::ValueVector2i   m_size;
      Valuable::ValueInt        m_frameless;
      Valuable::ValueInt        m_fullscreen;
      Valuable::ValueInt        m_resizeable;
      Valuable::ValueInt        m_displaynumber; // for X11
      Valuable::ValueInt        m_screennumber; // for X11
      /// Pixel size in centimeters
      float      m_pixelSizeCm;

      std::vector<std::shared_ptr<Area> > m_areas;
    };

    LUMINOUS_API MultiHead();
    LUMINOUS_API virtual ~MultiHead();

    /// The number of areas
    LUMINOUS_API size_t areaCount();
    /// Access the areas
    /** This method traverses all the windows to find the area with
    given index.
    @param index window index to look for
    @param winptr pointer to a window if the area is found
    @return area with the given index or the first area if no match is found */
    LUMINOUS_API Area & area(size_t index, Window ** winptr = 0);

    /// Returns the number of windows
    size_t windowCount() const { return m_windows.size(); }
    /// Access the ith window
    /// @param i window index
    LUMINOUS_API Window & window(size_t i);
    /// @copydoc window
    LUMINOUS_API const Window & window(size_t i) const;

    /// Total size of all the windows
    LUMINOUS_API Nimble::Vector2i totalSize();

    /// Returns the total graphics size
    LUMINOUS_API Rect graphicsBounds() const;

    /// Returns the size of the total display in graphics pixels
    Nimble::Vector2i size()
    { return Nimble::Vector2i(width(), height()); }

    /// Total width of the display area, in graphics pixels.
    LUMINOUS_API int width();
    /// Total height of the display area, in graphics pixels.
    LUMINOUS_API int height();

    LUMINOUS_API bool deserialize(Valuable::ArchiveElement & element);

    /// Adds a window to the collection
    void addWindow(Window * w) { m_windows.push_back(std::shared_ptr<Window>(w)); }

    /// Sets the edited flag
    void setEdited(bool edited) { m_edited = edited; }
    /// Returns true if the edited flag has been set
    bool isEdited() const { return m_edited; }

    /// Returns the gamma value used for edge blending with projector setups.
    float gamma() const { return m_gamma; }

  private:
    LUMINOUS_API virtual bool readElement(Valuable::DOMElement ce);

    std::vector<std::shared_ptr<Window> > m_windows;
    Valuable::ValueFloat m_widthcm;
    Valuable::ValueFloat m_gamma;
    bool m_edited;
  };

}

#endif
