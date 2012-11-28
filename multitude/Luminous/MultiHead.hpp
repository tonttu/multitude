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

#include "Export.hpp"
#include "Collectable.hpp"
#include "GLKeyStone.hpp"
#include "HardwareColorCorrection.hpp"
#include "ColorCorrection.hpp"
#include "RGBCube.hpp"

#include <Nimble/Rect.hpp>
#include <Nimble/Vector4.hpp>

#include <memory>

#include <Valuable/AttributeString.hpp>
#include <Valuable/AttributeFloat.hpp>
#include <Valuable/AttributeBool.hpp>

#include <vector>


namespace Luminous {

  using Nimble::Rect;
  using Nimble::Vector2f;
  using Nimble::Vector2i;
  using Nimble::Vector4f;

  class RenderContext;
  class Shader;
  class Texture1D;

  /// Class for managing information on multiple OpenGL vindows/viewports.
  /** This class stores information about the layout of multiple
      OpenGL windows and viewports. This information is used in
      systems that render OpenGL into several windows at
      once. MultiHead also includes keystone information, so that
      keystoning can be done by using skewed OpenGL transformation
      matrices. */
  class LUMINOUS_API MultiHead : public Valuable::Node
  {
  public:

    class Window;

    /** One OpenGL area
    Areas are roughly equivalent to OpenGL viewports. Multiple
    areas can share the same OpenGL context, as one window can
    have may areas inside it.*/
    /// @todo rename to ViewPort?
    class LUMINOUS_API Area : public Valuable::Node, public Collectable
    {
      MEMCHECKED_USING(Collectable);
    public:

      /// Constructs a new area for the given window
      Area(Window * window = 0);
      virtual ~Area();
      /// Deserializes this area from an archive element
      bool deserialize(const Valuable::ArchiveElement & element);

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

        if(copyToGraphics) {
          setGraphicsGeometry(x, y, w, h);
          updateBBox();
        }
      }

      /// Sets the size of the area
      void setSize(Vector2i size)
      {
        m_size = size;
      }

      /// Sets the graphics geometry of the area
      void setGraphicsGeometry(int x, int y, int w, int h)
      {
        m_graphicsLocation = Nimble::Vector2f(x, y);
        m_graphicsSize = Nimble::Vector2f(w, h);
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
      //void applyViewportAndViewTransform(Luminous::RenderContext &) const;
      /// Blends the edges defined by seams
      void cleanEdges() const;

      /// Returns the type name for areas (="area").
      virtual const char * type() const { return "area"; }

      /// Returns the keystone correction
      /// @return keystone correction
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
      const Nimble::Vector2f graphicsLocation(bool withseams = true) const
      {
        return withseams ?
            m_graphicsLocation.asVector() - Nimble::Vector2f(m_seams[0], m_seams[3]) :
            m_graphicsLocation.asVector();
      }
      /// The size of the graphics inside this area (virtual pixels)
      const Nimble::Vector2f graphicsSize(bool withseams = true) const
      {
        return withseams ?
            m_graphicsSize.asVector() + Nimble::Vector2f(m_seams[0] + m_seams[1],
                                                         m_seams[2] + m_seams[3]) :
            m_graphicsSize.asVector();
      }

      /** The bounds of the graphics.

          In principle this method only combines the information you get with
          graphicsLocation() and graphicsSize(). In practice however that
          information is not exactly correct as the bounds also need to include
          the small extra areas, if edge-blending is used.
          @return graphics bounds rectangle*/
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
      Nimble::Vector2f windowToGraphics(Nimble::Vector2f loc, int windowheight, bool & insideArea) const;

      Nimble::Vector2f graphicsToWindow(Nimble::Vector2f loc, int windowheight, bool & insideArea) const;

      /// Returns true if the area is active (graphics will be drawn to it)
      bool active() const { return m_active; }

      /// Sets the width of a single pixel in centimeters
      void setPixelSizeCm(float sizeCm) { m_pixelSizeCm = sizeCm; }
      /// Returns the width of a single pixel in centimeters
      float pixelSizeCm() const { return m_pixelSizeCm; }
      /// Converts centimeters to pixels
      float cmToPixels(float cm) { return cm / m_pixelSizeCm; }

      /// Get the view transformation (projection) matrix defined by the area
      /// @return view transformation defined by the area
      Nimble::Matrix4 viewTransform() const;

      /// Swaps the width and height of the graphics size
      void swapGraphicsWidthHeight()
      {
        m_graphicsSize = m_graphicsSize.asVector().shuffle();
        updateBBox();
      }

      /// Returns a pointer to the window that holds this area
      const Window * window() const { return m_window; }

      RGBCube & rgbCube()
      {
        return m_rgbCube;
      }

      const RGBCube & rgbCube() const
      {
        return m_rgbCube;
      }

      ColorCorrection & colorCorrection()
      {
        return m_colorCorrection;
      }

      const ColorCorrection & colorCorrection() const
      {
        return m_colorCorrection;
      }

      /// Checks if software color correction is in use
      /// @return true if software correction is used, false if color correction
      /// is done by hardware or disabled
      bool isSoftwareColorCorrection() const;

      /// Get the viewport defined by the area in window coordinates.
      /// @return the viewport defined by the area
      Nimble::Recti viewport() const
      {
        return Nimble::Recti(m_location[0], m_location[1], m_location[0]+m_size[0], m_location[1]+m_size[1]);
      }

      enum {
        /* Render to the screen, using straight coordinates. Then
       read-back and re-render, with skewed coordinates. */
        METHOD_TEXTURE_READBACK,
        /* Render directly with skewed coordinates. Nice for
       performance, but a bit tricky for ripple effects etc. */
        METHOD_MATRIX_TRICK
      };
    private:

      void updateBBox();

      Window * m_window;
      GLKeyStone m_keyStone;
      Valuable::AttributeVector2i   m_location;
      Valuable::AttributeVector2i   m_size;
      Valuable::AttributeVector2f   m_graphicsLocation;
      Valuable::AttributeVector2f   m_graphicsSize;
      Valuable::AttributeVector4f   m_seams;
      Valuable::AttributeBool        m_active;
      Valuable::AttributeInt        m_method;
      Valuable::AttributeString m_comment;
      Rect m_graphicsBounds;
      float      m_pixelSizeCm;
      Shader * m_colorCorrectionShader;
      Collectable m_colorCorrectionTextureKey;
      ColorCorrection m_colorCorrection;
      RGBCube m_rgbCube;
    };

    /** One OpenGL window.
    A window is responsible for one OpenGL context.*/
    class Window : public Valuable::Node
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

      /// Sets the size of this Window
      void setSize(const Nimble::Vector2i &newSize) { m_size = newSize; }

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
      void addArea(Area * a) { m_areas.push_back(std::shared_ptr<Area>(a)); addValue(a); }

      /// Location of the window on the computer display
      const Vector2i & location() const { return m_location.asVector(); }
      /// Size of the window on the computer display
      const Vector2i & size() const { return m_size.asVector(); }

      /// Returns the width of this window in pixels
      /// @return width of the window
      int width() const { return m_size.x(); }
      /// Returns the height of this window in pixels
      /// @return height of the window
      int height() const { return m_size.y(); }

      /** Convert a coordinate from screen to graphics coordinates.
          This class traverses through all the areas to find an area
          that would include given location. It is entirely possible
          none of the areas contains the given coordinate, as there
          can be gaps between areas (due to keystoning, for example).

          @param loc The location in screen coordinates.

          @param convOk set to true or false depending on whether the
          conversion could be carried out.

          @return the point in graphics coordinates
      */
      LUMINOUS_API Nimble::Vector2f windowToGraphics(Nimble::Vector2f loc, bool & convOk) const;

      /// Should the window be frameless
      bool frameless() const { return m_frameless; }
      /// Should the window be full-screen
      bool fullscreen() const { return m_fullscreen; }
      /// Sets the fullscreen flag
      void setFullscreen(bool f) { m_fullscreen = f; }
      /// Should the window be resizeable
      bool resizeable() const { return m_resizeable; }

      /// X11 screen number for threaded rendering, -1 if not specified
      int screennumber() const { return m_screennumber.asInt(); }
      /// Sets X11 screen number for threaded rendering, -1 disables
      void setScreennumber(int s) { m_screennumber = s; }

      /// Number of samples per pixel for full-screen anti-aliasing
      int antiAliasingSamples() const
      { return m_fsaaSamplesPerPixel; }
      /// Sets the number of samples per pixel for full-screen anti-aliasing
      void setAntiAliasingSamples(int samplesPerPixel)
      { m_fsaaSamplesPerPixel = samplesPerPixel; }

      /// Direct rendering mode enabled
      /// @return true if direct rendering is enabled, false is rendering
      /// is done through an off-screen render target
      bool directRendering() const { return m_directRendering; }
      /// Set direct rendering mode
      void setDirectRendering(bool enable) { m_directRendering = enable; }

      /// Sets the width of a pixel in centimeters to each area inside this window
      LUMINOUS_API void setPixelSizeCm(float sizeCm);

      /// Return the screen configuration that this Window belongs to
      const MultiHead * screen() const { return m_screen; }

      void deleteAreas()
      {
        for(std::vector<std::shared_ptr<Area> >::iterator it = m_areas.begin(); it != m_areas.end(); ++it)
        {
          removeValue(it->get());
        }
        m_areas.clear();
      }

      Nimble::Recti getRect() const {
        return Nimble::Recti(location().x,
                             location().y,
                             location().x + width(),
                             location().y + height());
      }

    private:
      LUMINOUS_API virtual bool readElement(const Valuable::ArchiveElement & ce);

      MultiHead                *m_screen;
      Valuable::AttributeVector2i   m_location;
      Valuable::AttributeVector2i   m_size;
      Valuable::AttributeBool       m_frameless;
      Valuable::AttributeBool       m_fullscreen;
      Valuable::AttributeBool       m_resizeable;
      Valuable::AttributeInt        m_fsaaSamplesPerPixel;
      Valuable::AttributeBool       m_directRendering;
      Valuable::AttributeInt        m_screennumber; // for X11
      /// Pixel size in centimeters
      float      m_pixelSizeCm;

      std::vector<std::shared_ptr<Area> > m_areas;
    };

    MultiHead();
    virtual ~MultiHead();

    /// The number of areas
    size_t areaCount();
    /// Access the areas
    /** This method traverses all the windows to find the area with
    given index.
    @param index window index to look for
    @param winptr pointer to a window if the area is found
    @return area with the given index or the first area if no match is found */
    Area & area(size_t index, Window ** winptr = 0);

    /// Returns the number of windows
    size_t windowCount() const { return m_windows.size(); }
    /// Access the ith window
    /// @param i window index
    /// @return the window
    Window & window(size_t i);
    /// @copydoc window
    const Window & window(size_t i) const;

    /// Total size of all the windows
    Nimble::Vector2i totalSize();

    /// Returns the total graphics size
    Rect graphicsBounds() const;

    /// Returns the size of the total display in graphics pixels
    Nimble::Vector2i size()
    { return Nimble::Vector2i(width(), height()); }

    /// Total width of the display area, in graphics pixels.
    int width();
    /// Total height of the display area, in graphics pixels.
    int height();

    Nimble::Vector3i dpms() const { return m_dpms; }
    void setDpms(const Nimble::Vector3i & dpms);

    bool deserialize(const Valuable::ArchiveElement & element);

    /// Adds a window to the collection
    void addWindow(Window * w);

    /// Sets the edited flag
    void setEdited(bool edited) { m_edited = edited; }
    /// Returns true if the edited flag has been set
    bool isEdited() const { return m_edited; }

    /// Sets the iconify flag
    void setIconify(bool iconify) { m_iconify = iconify; }
    /// Returns true if the iconify flag has been set
    bool iconify() const { return m_iconify; }

    /// Returns the gamma value used for edge blending with projector setups.
    float gamma() const { return m_gamma; }

    const HardwareColorCorrection & hwColorCorrection() const
    {
      return m_hwColorCorrection;
    }

    HardwareColorCorrection & hwColorCorrection()
    {
      return m_hwColorCorrection;
    }

    void deleteWindows()
    {
      //this should remove listeners that refer to Areas within the windows
      m_hwColorCorrection.syncWith(0);
      for(std::vector<std::shared_ptr<Window> >::iterator it = m_windows.begin(); it != m_windows.end(); ++it)
      {
        //delete window's areas
        it->get()->deleteAreas();
        removeValue(it->get());
      }
      m_windows.clear();
    }

    float dpi() const;
    void setDpi(float dpi);

  private:
    virtual bool readElement(const Valuable::ArchiveElement & ce);
    virtual void dpmsChanged();

    std::vector<std::shared_ptr<Window> > m_windows;
    Valuable::AttributeFloat m_widthcm;
    Valuable::AttributeFloat m_gamma;
    Valuable::AttributeBool m_iconify;
    Valuable::AttributeVector3i m_dpms;
    Valuable::AttributeFloat m_dpi;
    Valuable::AttributeBool m_hwColorCorrectionEnabled;
    HardwareColorCorrection m_hwColorCorrection;

    bool m_edited;
  };

}

#endif
