/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "MultiHead.hpp"

#include "RenderContext.hpp"
#include "PixelFormat.hpp"

#include <Nimble/Matrix4.hpp>

#include <Radiant/Trace.hpp>

#include <Valuable/DOMElement.hpp>
#include <Valuable/AttributeContainer.hpp>

#include <functional>

namespace Luminous
{

  MultiHead::Area::Area()
      : Node(0, "Area"),
      m_keyStone(this, "keystone"),
      m_location(this, "location", Nimble::Vector2i(0, 0)),
      m_size(this, "size", Nimble::Vector2i(100, 100)),
      m_graphicsLocation(this, "graphicslocation", Nimble::Vector2f(0, 0)),
      m_graphicsSize(this, "graphicssize", Nimble::Vector2f(100, 100)),
      m_seams(this, "seams", Nimble::Vector4f(0, 0, 0, 0)),
      m_method(this, "method", METHOD_MATRIX_TRICK),
      m_graphicsBounds(0, 0, 100, 100),
      m_colorCorrection(this, "colorcorrection"),
      m_rgbCube(this, "rgbcube")
  {
    auto update = [this] { updateBBox(); };
    m_graphicsLocation.addListener(update);
    m_graphicsSize.addListener(update);
    m_seams.addListener(update);
    updateBBox();
  }

  MultiHead::Area::~Area()
  {
  }

  void MultiHead::Area::setGeometry(int x, int y, int w, int h, bool copyToGraphics)
  {
    m_location = Nimble::Vector2i(x, y);
    m_size = Nimble::Vector2i(w, h);

    if(copyToGraphics) {
      setGraphicsGeometry(x, y, w, h);
    }
  }

  Nimble::Size MultiHead::Area::size() const
  {
    return Nimble::Size(*m_size);
  }

  void MultiHead::Area::setSize(Nimble::Size size)
  {
    m_size = size.toVector();
  }

  // @getter graphicslocation
  const Nimble::Vector2f MultiHead::Area::graphicsLocation(bool withseams) const
  {
    return withseams ?
        m_graphicsLocation.asVector() - Nimble::Vector2f(m_seams[0], m_seams[3]) :
        m_graphicsLocation.asVector();
  }

  // @setter graphicslocation
  void MultiHead::Area::setGraphicsLocation(Nimble::Vector2f l)
  {
    m_graphicsLocation = l;
  }

  // @getter graphicssize
  const Nimble::SizeF MultiHead::Area::graphicsSize(bool withseams) const
  {
    return Nimble::SizeF(withseams ?
        m_graphicsSize.asVector() + Nimble::Vector2f(m_seams[0] + m_seams[1],
                                                     m_seams[2] + m_seams[3]) :
        m_graphicsSize.asVector());
  }

  // @setter graphicssize
  void MultiHead::Area::setGraphicsSize(Nimble::SizeF size)
  {
    m_graphicsSize = size.toVector();
  }

  const Rect & MultiHead::Area::graphicsBounds() const
  {
    return m_graphicsBounds;
  }


  void MultiHead::Area::setGraphicsGeometry(int x, int y, int w, int h)
  {
    m_graphicsLocation = Nimble::Vector2f(x, y);
    m_graphicsSize = Nimble::Vector2f(w, h);
  }

  // @setter seams
  void MultiHead::Area::setSeams(Nimble::Vector4f seams)
  {
    m_seams = seams;
  }

  Nimble::Vector4f MultiHead::Area::seams() const
  {
    return m_seams;
  }

  float MultiHead::Area::maxSeam() const
  {
    return m_seams.asVector().maximum();
  }

  bool MultiHead::Area::deserialize(const Valuable::ArchiveElement & element)
  {
    bool ok = Node::deserialize(element);

    return ok;
  }

  GLKeyStone & MultiHead::Area::keyStone()
  {
    return m_keyStone;
  }

  const GLKeyStone & MultiHead::Area::keyStone() const
  {
    return m_keyStone;
  }

  const Vector2i & MultiHead::Area::location() const
  {
    return m_location;
  }

  void MultiHead::Area::setLocation(Nimble::Vector2i loc)
  {
    m_location = loc;
  }

  Nimble::Vector2f MultiHead::Area::windowToGraphics
      (Nimble::Vector2f loc, int windowheight, bool & isInside) const
  {
    //      Radiant::trace("MultiHead::Area::windowToGraphics");

    assert((m_size[0] > 0.01f) && (m_size[1] > 0.01f));

    loc.x -= m_location[0];
    loc.y -= (windowheight - m_size[1] - m_location[1]);
    loc.descale(Nimble::Vector2f(m_size->x, m_size->y));
    loc.y = 1.0f - loc.y;

    bool dontCare = false;
    Nimble::Matrix4 m = m_keyStone.matrix().inverse( &dontCare);
    assert(dontCare);

    loc = GLKeyStone::projectCorrected(m, loc).vector2();

    Nimble::Rectf rectangle(0.f, 0.f, 1.f, 1.f);
    bool ok = rectangle.contains(loc);

    isInside = ok;

    loc.y = 1.0f - loc.y;
    loc.scale(graphicsBounds().size().toVector());
    loc += graphicsBounds().low();

    return loc;
  }

  Nimble::Vector2f MultiHead::Area::graphicsToWindow
      (Nimble::Vector2f loc, int windowheight, bool & isInside) const
  {
    loc -= graphicsBounds().low();
    loc.descale(graphicsBounds().size().toVector());
    loc.y = 1.0f - loc.y;

    Nimble::Matrix4 m = m_keyStone.matrix();
    loc = GLKeyStone::projectCorrected(m, loc).vector2();

    Nimble::Rectf rectangle(0.f, 0.f, 1.f, 1.f);
    bool ok = rectangle.contains(loc);

    isInside = ok;

    loc.y = 1.0f - loc.y;
    loc.scale(Nimble::Vector2f(m_size->x, m_size->y));
    loc.y += (windowheight - m_size[1] - m_location[1]);
    loc.x += m_location[0];

    return loc;
  }

  Nimble::Matrix4 MultiHead::Area::viewTransform() const
  {
    Nimble::Rect b = graphicsBounds();

    Nimble::Matrix4 m = Nimble::Matrix4::ortho3D(b.low().x, b.high().x,
                                                 b.high().y, b.low().y,
                                                 -1.0f, 1.0f);

    if(m_method == METHOD_MATRIX_TRICK) {
      Nimble::Matrix4 km = m_keyStone.matrix();

      Nimble::Matrix4 x1 = Nimble::Matrix4::makeScale(Nimble::Vector3(2, 2, 2));

      Nimble::Matrix4 x2 = Nimble::Matrix4::makeTranslation(Nimble::Vector3(-1, -1, 0));

      Nimble::Matrix4 x3 = Nimble::Matrix4::makeTranslation(Nimble::Vector3(1, 1, 0));
      Nimble::Matrix4 x4 = Nimble::Matrix4::makeScale(Nimble::Vector3(.5f, .5f, .5f));

      return x2 * x1 * km * x4 * x3 * m;
    } else {
      return m;
    }
  }

  void MultiHead::Area::swapGraphicsWidthHeight()
  {
    m_graphicsSize = m_graphicsSize.asVector().shuffle();
  }

  bool MultiHead::Area::readElement(const Valuable::ArchiveElement & element)
  {
    Radiant::warning("MultiHead::Window::readElement # Ignoring unknown element %s", element.name().toUtf8().data());
    return true;
  }

  void MultiHead::Area::updateBBox()
  {
    m_graphicsBounds.set
        (m_graphicsLocation.asVector(),
         m_graphicsLocation.asVector() + m_graphicsSize.asVector());
    m_graphicsBounds.low().x  -= m_seams[0];
    m_graphicsBounds.high().x += m_seams[1];
    m_graphicsBounds.low().y  -= m_seams[3];
    m_graphicsBounds.high().y += m_seams[2];
    if (auto window = dynamic_cast<Window*>(host())) {
      if (MultiHead * multihead = window->m_screen) {
        multihead->eventSend("graphics-bounds-changed");
      }
    }
  }

  RGBCube & MultiHead::Area::rgbCube()
  {
    return m_rgbCube;
  }

  const RGBCube & MultiHead::Area::rgbCube() const
  {
    return m_rgbCube;
  }

  ColorCorrection & MultiHead::Area::colorCorrection()
  {
    return m_colorCorrection;
  }

  const ColorCorrection & MultiHead::Area::colorCorrection() const
  {
    return m_colorCorrection;
  }

  Nimble::Recti MultiHead::Area::viewport() const
  {
    return Nimble::Recti(m_location[0], m_location[1], m_location[0]+m_size[0], m_location[1]+m_size[1]);
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  namespace
  {
    const int s_default_fsaa_samples = 4;
  }
  MultiHead::Window::Window(MultiHead * screen)
    : Node(0, "Window"),
      m_screen(screen),
      m_location(this, "location", Nimble::Vector2i(0, 0)),
      m_size(this, "size", Nimble::Vector2i(100, 100)),
      m_frameless(this, "frameless", true),
      m_stayOnTop(this, "stay-on-top", true),
      m_bypassWindowManager(this, "bypass-window-manager", true),
      m_fullscreen(this, "fullscreen", false),
      m_resizable(this, "resizable", false),
      m_resizeable(this, "resizeable", &m_resizable),
      m_fsaaSamplesPerPixel(this, "fsaa-samples", s_default_fsaa_samples),
      m_uploadLimit(this, "gpu-upload-limit", ((int64_t)4) << 36),
      m_uploadMargin(this, "gpu-upload-margin", ((int64_t)128<<12)),
      m_directRendering(this, "direct-rendering", true),
      m_screennumber(this, "screennumber", -1)
  {
    // stay-on-top default value depends on frameless value for backwards compatibility
    m_frameless.addListener([this] {
      m_stayOnTop.setValue(m_frameless.value(), DEFAULT);
      m_bypassWindowManager.setValue(m_frameless.value(), DEFAULT);
    });
  }

  MultiHead::Window::~Window()
  {}

  void MultiHead::Window::resizeEvent(Nimble::Size size)
  {
    // Area resizing is currently only supported if there is only one area
    // which has the same size as the window. We could be more intelligent here
    // and support various other cases as well, and even add some layout
    // parameters to areas (similar to flexbox), but currently this should cover
    // the typical use case
    if (m_areas.size() == 1 && m_areas[0]->size() == this->size() &&
       m_areas[0]->graphicsSize(false).cast<int>() == m_areas[0]->size()) {
      m_areas[0]->setGraphicsSize(size.cast<float>());
    }
    m_size = size.toVector();
  }

  Nimble::Rect MultiHead::Window::graphicsBounds() const
  {
    if(m_areas.empty())
      return Nimble::Rect(0,0, 99, 99);

    Rect r = m_areas[0]->graphicsBounds();

    for(size_t i = 1; i < m_areas.size(); i++) {
      r.expand(m_areas[i]->graphicsBounds());
    }

    return r;
  }

  void MultiHead::Window::setSeam(float seam)
  {
    for(size_t i = 0; i < m_areas.size(); i++) {
      m_areas[i]->setSeams(Nimble::Vector4f(i == 0 ? 0 : seam,
                                 i + 1 >= m_areas.size() ? 0 : seam,
                                 0, 0));
    }
  }

  void MultiHead::Window::addArea(std::unique_ptr<Area> a)
  {
    if(!a)
      return;

    addAttribute(a.get());

    m_areas.push_back(std::move(a));
  }

  void MultiHead::Window::removeArea(size_t i)
  {
    if(m_areas.size() <= i) return;

    removeAttribute(m_areas[i].get());
    m_areas.erase(m_areas.begin() + i);

  }

  Nimble::Vector2f MultiHead::Window::windowToGraphics(Nimble::Vector2f loc, bool & convOk) const
  {
    //      Radiant::trace("MultiHead::Window::windowToGraphics # loc(%f,%f), m_size[1] = %d", loc.x, loc.y, m_size[1]);

    Nimble::Vector2f res(0, 0);
    for(size_t i = 0; i < m_areas.size(); i++) {
      bool ok = false;
      res = m_areas[i]->windowToGraphics(loc, m_size[1], ok);

      if(ok) {
        convOk = true;
        return res;
      }
    }

    convOk = false;

    return res;
  }

  QPointF MultiHead::Window::windowToGraphics(QPointF loc, bool &convOk) const
  {
    Nimble::Vector2 nloc(loc.x(), loc.y());
    nloc = windowToGraphics(nloc, convOk);
    return QPointF(nloc.x, nloc.y);
  }

  void MultiHead::Window::deleteAreas()
  {
    m_areas.clear();
    if (MultiHead * multihead = m_screen) {
      multihead->eventSend("graphics-bounds-changed");
    }
  }

  bool MultiHead::Window::isAreaSoftwareColorCorrected(int areaIndex) const
  {
    const bool isSW = m_areas[areaIndex]->rgbCube().isDefined() || !m_areas[areaIndex]->colorCorrection().isIdentity();

    return isSW;
  }

  Nimble::Recti MultiHead::Window::getRect() const {
    return Nimble::Recti(location().x,
                         location().y,
                         location().x + width(),
                         location().y + height());
  }

  bool MultiHead::Window::readElement(const Valuable::ArchiveElement & ce)
  {
    /// @todo Remove this function and use the correct serialization API
    bool ok = true;
    const QByteArray name = ce.name().toUtf8();

    // This is for backwards compatibility. The attribute was removed in 2.0
    // but we still want to be able to parse old configuration files.
    if(name == "displaynumber")
      return true;

    // Get the 'type' attribute
    const QString & type = ce.get("type");

    if(type == QString("area")) {
      auto area = std::unique_ptr<Area>(new Area());
      // Add as child & recurse
      addAttribute(name, area.get());
      ok &= area->deserialize(ce);
      m_areas.push_back(std::move(area));
      if (MultiHead * multihead = m_screen) {
        multihead->eventSend("graphics-bounds-changed");
      }
    } else {
      Radiant::warning("MultiHead::Window::readElement # Ignoring unknown element %s", name.data());
    }

    return ok;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  MultiHead::MultiHead()
      : Node(0, "MultiHead"),
      m_iconify(this, "iconify", false),
      m_dpi(this, "dpi", 40.053f), /* DPI for 55" */
      m_vsync(this, "vsync",
#ifdef RADIANT_WINDOWS
              /// Disable vsync by default on Windows because of DWM issues, see #12221
              false
#else
              true
#endif
              ),
      m_glFinish(this, "gl-finish", false),
      m_layerSize(this, "layer-size", Nimble::Vector2i(0, 0))
  {
    eventAddOut("graphics-bounds-changed");
  }

  MultiHead::~MultiHead()
  {}

  MultiHead::Window & MultiHead::window(size_t i)
  {
    if(i >= m_windows.size()) {
      Radiant::fatal("MultiHead::window # Array index %lu exceeds array size %lu",
                     i, m_windows.size());
    }

    return * m_windows[i];
  }

  const MultiHead::Window & MultiHead::window(size_t i) const
  {
    if(i >= m_windows.size()) {
      Radiant::fatal("MultiHead::window # Array index %lu exceeds array size %lu",
                     i, m_windows.size());
    }

    return * m_windows[i];
  }

  size_t MultiHead::areaCount()
  {
    size_t n = 0;

    for(size_t i = 0; i < m_windows.size(); i++)
      n += m_windows[i]->areaCount();

    return n;
  }

  MultiHead::Area & MultiHead::area(size_t index, MultiHead::Window ** winptr)
  {
    size_t used = 0;

    for(size_t i = 0; i < m_windows.size(); i++) {
      size_t n = m_windows[i]->areaCount();
      if(used + n > index) {
        if(winptr)
          *winptr = m_windows[i].get();
        return m_windows[i]->area(index - used);
      }
      used += n;
    }

    assert(false); // Out of range

    return m_windows[0]->area(0); // Unreachable
  }

  Rect MultiHead::graphicsBounds() const
  {
    if(!windowCount())
      return Nimble::Rect(0, 0, 100, 100);

    Rect r = window(0).graphicsBounds();

    for(size_t i = 1; i < windowCount(); i++) {
      r.expand(window(i).graphicsBounds());
    }

    return r;
  }

  Rect MultiHead::layerSize() const
  {
    if (m_layerSize->length()) {
      return Nimble::Rect(0, 0, m_layerSize.x(), m_layerSize.y());
    }
    else return graphicsBounds();
  }

  int MultiHead::width()
  {
    float left =  1000000;
    float right =-1000000;

    size_t n = areaCount();

//    debugLuminous("MultiHead::width # %lu", n);

    for(size_t i = 0; i < n; i++) {
      Area & a = area(i);

      float wleft  = a.graphicsLocation().x;
      float wright = wleft + a.graphicsSize().width();

      left  = std::min(left,  wleft);
      right = std::max(right, wright);

//      debugLuminous("lr = %f %f", left, right);
    }

    return (int) (right - left);
  }

  int MultiHead::height()
  {
    float top = 1000000;
    float bottom = -1000000;

    size_t n = areaCount();

    for(size_t i = 0; i < n; i++) {
      Area & a = area(i);

      float wtop = a.graphicsLocation().y;
      float wbot = wtop + a.graphicsSize().height();

      top = std::min(top, wtop);
      bottom = std::max(bottom, wbot);
    }

    return (int) (bottom - top);
  }

  float MultiHead::dpi() const
  {
    return m_dpi;
  }

  void MultiHead::setDpi(float dpi)
  {
    m_dpi = dpi;
  }

  void MultiHead::setGlFinish(bool v)
  {
    m_glFinish = v;
  }

  bool MultiHead::useGlFinish() const
  {
    return m_glFinish;
  }

  bool MultiHead::deserialize(const Valuable::ArchiveElement & element)
  {
    m_windows.clear();

    bool ok = Node::deserialize(element);
    if(ok) {
      m_edited = false;
    }

    return ok;
  }

  void MultiHead::addWindow(std::unique_ptr<Window> w)
  {
    addAttribute(w.get());

    m_windows.push_back(std::move(w));
    eventSend("graphics-bounds-changed");
  }

  void MultiHead::deleteWindows()
  {
    /// @todo this should remove listeners that refer to Areas within the windows
    m_windows.clear();
  }

  void MultiHead::createFullHDConfig()
  {
    // Add a default layout of 1920x1080
    auto win = std::unique_ptr<Window>(new Window());
    win->setGeometry(0,0,1920,1080);
    auto area = std::unique_ptr<Area>(new Area());
    area->setGeometry(0,0,1920,1080);
    win->addArea(std::move(area));

    addWindow(std::move(win));
  }

  void MultiHead::mergeConfiguration(const MultiHead &source)
  {
    QSet<QByteArray> oldWindows, newWindows;

    // Collect old windows
    for(size_t i = 0; i < windowCount(); ++i)
      oldWindows.insert(window(i).name());

    // Collect new windows
    for(size_t i = 0; i < source.windowCount(); ++i)
      newWindows.insert(source.window(i).name());

    // Find windows in both configurations to copy
    auto windowsToCopy = oldWindows;
    windowsToCopy.intersect(newWindows);

    // Find windows to remove
    auto windowsToRemove = oldWindows;
    windowsToRemove.subtract(windowsToCopy);

    // Find windows to add
    auto windowsToAdd = newWindows;
    windowsToAdd.subtract(windowsToCopy);

    // Remove windows not present in the source configuration
    auto pend = std::remove_if(m_windows.begin(), m_windows.end(), [windowsToRemove](const std::unique_ptr<Window> & p)
    {
      return windowsToRemove.contains(p->name());
    });

    m_windows.erase(pend, m_windows.end());

    // Create new windows to add
    for(auto & name : windowsToAdd) {

      auto w = std::unique_ptr<Window>(new Window(this));
      w->setName(name);

      addWindow(std::move(w));

      // Add new windows to be copied later
      windowsToCopy.insert(name);
    }

    // Copy values
    for(auto & name : windowsToCopy) {

      auto src = static_cast<Node*>(source.attribute(name));
      auto dst = static_cast<Node*>(attribute(name));

      Node::copyValues(*src, *dst);
    }

    // To copy the values in MultiHead object itself, we can't use copyValues()
    // because it would re-create the windows and areas as well. We need to
    // copy these values manually.
    for(auto it : source.attributes()) {

      Valuable::XMLArchive archive(Valuable::SerializationOptions::LAYER_USER);

      auto attributeName = it.first;
      auto element = it.second->serialize(archive);

      // If the attribute was serialized (e.g. was set on LAYER_USER)
      if(!element.isNull()) {

        // Check if the same attribute is available in dest (might not be when
        // window or areas are different)
        auto dstAttribute = attribute(attributeName);

        if(dstAttribute)
          dstAttribute->deserialize(element);
      }
    }
    removeDuplicateAreas();
  }

  void MultiHead::adjustGraphicsToOrigin()
  {
    Nimble::Rect bb = graphicsBounds();
    Nimble::Vector2f diff = bb.low();
    for(size_t j = 0; j < windowCount(); ++j) {
      for(size_t i = 0; i < window(j).areaCount(); ++i) {
        Area& a = window(j).area(i);
        a.setGraphicsLocation(a.graphicsLocation(false) - diff);
      }
    }
  }

  void MultiHead::removeDuplicateAreas()
  {
    for(size_t j = 0; j < windowCount(); ++j) {
      Window& win = window(j);

      size_t areas = win.areaCount();
      for(size_t i = 0; i < areas; ++i) {
        Area & a = win.area(i);
        Nimble::Recti areaA(a.location(), a.size());

        for(size_t k = 0; k < areas;) {
          Area & b = win.area(k);
          Nimble::Recti areaB(b.location(), b.size());

          if(k != i && areaA.contains(areaB) && areaA.area() > areaB.area()) {
            win.removeArea(k);
            if(i > k) --i;
            --areas;
          }
          else
            ++k;
        }
      }

    }

  }

  bool MultiHead::readElement(const Valuable::ArchiveElement & ce)
  {
    const QString & type = ce.get("type");

    if(type == "window") {
      auto win = std::unique_ptr<Window>(new Window(this));

      bool ok = win->deserialize(ce);
      if(!ok) {
        Radiant::error("MultiHead::readElement # failed to parse window configuration");
        return false;
      }

      addWindow(std::move(win));
    } else {
      Radiant::warning("MultiHead::readElement # Ignoring unknown element %s", ce.name().toUtf8().data());
    }

    return true;
  }

}
