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

#include "CPUMipmaps.hpp"

#include "GPUMipmaps.hpp"

#include <Luminous/GLResources.hpp>

#include <Radiant/Directory.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

#include <cassert>
#include <fstream>

#ifdef WIN32
#define snprintf _snprintf
#endif

namespace Luminous {

  using namespace Radiant;

  volatile static int __cpcount = 0;

  CPUMipmaps::CPUItem::CPUItem()
    : m_state(WAITING),
    m_image(0),
    m_unUsed(0)
  {
    // info("CPUMipmaps::CPUItem::CPUItem # %d", ++__cpcount);
  }

  CPUMipmaps::CPUItem::~CPUItem()
  {
    // info("CPUMipmaps::CPUItem::~CPUItem # %d", --__cpcount);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  CPUMipmaps::CPUMipmaps()
    : m_fileModified(0),
    m_nativeSize(100, 100),
    m_maxLevel(0),
    m_fileMask(0),
    m_hasAlpha(false),
    m_timeOut(3.0f),
    m_ok(true)
  {
    scheduleFromNowSecs(0.0f);
  }

  CPUMipmaps::~CPUMipmaps()
  {

  }

  void CPUMipmaps::update(float dt, float )
  {
    for(int i = DEFAULT_MAP1; i <= m_maxLevel; i++) {
      m_stack[i].m_unUsed += dt;
    }
  }

  int CPUMipmaps::getOptimal(Nimble::Vector2f size)
  {
    float bigdim = Nimble::Math::Min(size.maximum(),
                                     (float) m_nativeSize.maximum());

    int bestlevel = Nimble::Math::Round(log(bigdim) / log(2.0) + 0.0f);

    if(bestlevel > m_maxLevel)
      bestlevel = m_maxLevel;
    else if(bestlevel < lowestLevel())
      bestlevel = lowestLevel();

    //trace("CPUMipmaps::getOptimal # %dx%d -> %d",
    // (int) size.x, (int) size.y, bestlevel);

    return bestlevel;
  }

  int CPUMipmaps::getClosest(Nimble::Vector2f size)
  {
    int bestlevel = getOptimal(size);

    // trace("CPUMipmaps::getClosest # Best would be %d", bestlevel);

    CPUItem &  item = m_stack[bestlevel];

    item.m_unUsed = 0.0f;

    // Scan for the best available mip-map.

    for(int i = bestlevel; i <= m_maxLevel; i++) {

      if(m_stack[i].m_state == READY) {
        m_stack[i].m_unUsed = 0.0f;
        return i;
      }
    }

    for(int i = bestlevel-1; i > 0; i--) {

      if(m_stack[i].m_state == READY) {
        m_stack[i].m_unUsed = 0.0f;
        return i;
      }
    }

    return -1;
  }

  ImageTex * CPUMipmaps::getImage(int i)
  {
    CPUItem & item = m_stack[i];

    item.m_unUsed = 0.0f;

    if(item.m_state != READY)
      return 0;

    return item.m_image.ptr();
  }

  void CPUMipmaps::markImage(int i)
  {
    m_stack[i].m_unUsed = 0.0f;
  }

  bool CPUMipmaps::isReady()
  {
    // return true;

    float dt = Radiant::TimeStamp
               (Radiant::TimeStamp::getTime() - m_startedLoading).secondsD();

    if(dt > 3.0f)
      return true;

    for(int i = lowestLevel(); i < m_maxLevel; i++) {
      CPUItem & ci = m_stack[i];

      if(ci.m_state == WAITING && ci.m_unUsed < m_timeOut)
        return false;
    }

    return true;
  }

  bool CPUMipmaps::startLoading(const char * filename, bool immediate)
  {
    debug("CPUMipmaps::startLoading # %s, %d", filename, immediate);
    m_startedLoading = Radiant::TimeStamp::getTime();
    m_filename = filename;
    m_fileModified = FileUtils::lastModified(m_filename);

    for(int i = 0; i < MAX_MAPS; i++) {
      m_stack[i].clear();
    }

    m_info = Luminous::ImageInfo();

    if(!Luminous::Image::ping(filename, m_info)) {
      error("CPUMipmaps::startLoading # failed to query image size for %s",
            filename);
      return false;
    }

    m_nativeSize.make(m_info.width, m_info.height);

    if(!m_nativeSize.minimum())
      return false;

    int smax = m_nativeSize.maximum();

    for(m_maxLevel = 0; (1 << m_maxLevel) < smax; m_maxLevel++)
      ;

    if(m_maxLevel >= MAX_MAPS)
      m_maxLevel = MAX_MAPS - 1;

    return true;
  }

  GPUMipmaps * CPUMipmaps::getGPUMipmaps(GLResources * resources)
  {
    GLResource * r = resources->getResource(this);

    if(!r) {
      GPUMipmaps * gm = new GPUMipmaps(this, resources);
      resources->addResource(this, gm);
      return gm;
    }

    GPUMipmaps * gm = dynamic_cast<GPUMipmaps *> (r);

    assert(gm != 0);

    return gm;
  }

  GPUMipmaps * CPUMipmaps::getGPUMipmaps()
  {
    return getGPUMipmaps(GLResources::getThreadResources());
  }

  bool CPUMipmaps::bind(GLResources * r,
                        const Nimble::Matrix3 & transform,
                        Nimble::Vector2 pixelsize)
  {
    if(!this)
      return false;

    GPUMipmaps * gpumaps = getGPUMipmaps(r);

    return gpumaps->bind(transform, pixelsize);
    //return true;
  }

  bool CPUMipmaps::isActive()
  {
    for(int i = lowestLevel(); i <= m_maxLevel; i++) {

      if(m_stack[i].m_state == WAITING)
        return true;
    }

    return false;
  }

  int CPUMipmaps::pixelAlpha(Nimble::Vector2 relLoc)
  {
    // info("CPUMipmaps::pixelAlpha # %f %f", relLoc.x, relLoc.y);

    for(int i = MAX_MAPS - 1; i >= 1; i--) {
      Image * im = getImage(i);

      if(!im) continue;

      Nimble::Vector2f locf(im->size());
      locf.scale(relLoc);

      Vector2i loci(locf);

      loci.x = Nimble::Math::Clamp(loci.x, 0, im->width() - 1);
      loci.y = Nimble::Math::Clamp(loci.y, 0, im->height() - 1);

      if(im->pixelFormat() == PixelFormat::rgbaUByte()) {
        const uint8_t * pixels = im->data();
        return pixels[(loci.x + loci.y * im->width()) * 4 + 3];
      }
      else if(im->pixelFormat() == PixelFormat::alphaUByte()) {
        const uint8_t * pixels = im->data();
        return pixels[loci.x + loci.y * im->width() + 3];
      }
      else {
        error("CPUMipmaps::pixelAlpha # Unsupported pixel format");
        return 255;
      }
    }

    // info("No mipmaps for alpha calculus");

    return 255;
  }

  void CPUMipmaps::finish()
  {
    setState(Task::DONE);
  }

  void CPUMipmaps::doTask()
  {
    // Start loading whatever is needed, recursively

    // info("CPUMipmaps::doTask # %s %d", m_filename.c_str(), m_maxLevel);

    // Load everything that maybe should be loaded:
    for(int i = lowestLevel(); i < m_maxLevel; i++) {
      if((m_stack[i].m_unUsed < m_timeOut) && (m_stack[i].m_state == WAITING)) {
        recursiveLoad(i);
      }
    }

    // Purge unused images from memory

    for(int i = lowestLevel(); i < m_maxLevel; i++) {

      CPUItem & item = m_stack[i];

      if((item.m_unUsed > m_timeOut) && (item.m_state == READY)) {
        // info("CPUMipmaps::doTask # Dropping %s %d", m_filename.c_str(), i);
        item.m_state = WAITING;
        item.m_image = 0;
      }
    }

    // info("CPUMipmaps::doTask # %s %d EXIT", m_filename.c_str(), m_maxLevel);

    scheduleFromNowSecs(0.5f);
  }

  void CPUMipmaps::cacheFileName(std::string & name, int level)
  {
    char buf[32];

    name = Radiant::FileUtils::path(m_filename);

    if(!name.empty())
      name += "/";
    name += ".imagecache/";

    snprintf(buf, sizeof(buf), "%.2d_", level);

    name += buf;
    name += Radiant::FileUtils::filename(m_filename);

    // Put in the right suffix
    unsigned i = name.size() - 1;

    while(i && name[i] != '.' && name[i] != '/')
      i--;

    name.erase(i + 1);

    if(m_info.pf.layout() == PixelFormat::LAYOUT_RGB ||
       m_info.pf.layout() == PixelFormat::LAYOUT_LUMINANCE)
      name += "jpg";
    else // if(m_info.pf.layout() == PixelFormat::LAYOUT_RGBA)
      name += "png";
  }

  void CPUMipmaps::recursiveLoad(int level)
  {
    CPUItem & item = m_stack[level];

    // info("CPUMipmaps::recursiveLoad # %s %d %d", m_filename.c_str(), level, m_maxLevel);

    if(level == DEFAULT_MAP1 || level == DEFAULT_MAP2) {

      // info("Testing for previews");

      // Try loading a pre-generated smaller-scale mipmap

      std::string filename;

      cacheFileName(filename, level);

      if(Radiant::FileUtils::fileReadable(filename) &&
         FileUtils::lastModified(filename) > m_fileModified) {

        Luminous::ImageTex * im = new ImageTex();

        if(!im->read(filename.c_str())) {
          error("CPUMipmaps::recursiveLoad # Could not read %s", filename.c_str());
          delete im;
        }
        else {

          if(im->hasAlpha())
            m_hasAlpha = true;

          item.m_image = im;
          item.m_state = READY;
        }
      }
    }

    if(level == m_maxLevel) {

      // Load original, and scale to useful dimensions:

      Luminous::ImageTex * im = new ImageTex();
      Radiant::RefPtr<Luminous::ImageTex> rim(im);

      if(!im->read(m_filename.c_str())) {
        error("CPUMipmaps::recursiveLoad # Could not read %s", m_filename.c_str());
        delete im;
        item.m_state = FAILED;
        return;
      }
      else {

        if(im->hasAlpha())
          m_hasAlpha = true;

        Nimble::Vector2i s = im->size();
        int smax = s.maximum();

        if(smax > maxDimension()) {
          float scale = maxDimension() / smax;
          s = Nimble::Vector2(s) * scale;
        }

        while(s.x & 0xFF) {
          s.x++;
        }
        while(s.y & 0xFF) {
          s.y++;
        }

        Luminous::ImageTex * im2 = new ImageTex();
        Radiant::RefPtr<Luminous::ImageTex> rim2(im2);

        im2->copyResample(*im, s.x, s.y);

        item.m_image = rim2;
        item.m_state = READY;
        return;
      }
    }

    // Load the higher level, and scale down from that:

    recursiveLoad(level + 1);

    CPUItem & src = m_stack[level + 1];

    if(src.m_state != READY) {
      error("Failed to get mipmap %d", level + 1);
      item.m_state = FAILED;
      return;
    }

    // Scale down from higher-level mipmap

    Luminous::ImageTex * imsrc = src.m_image.ptr();
    Luminous::ImageTex * imdest = new Luminous::ImageTex();

    Nimble::Vector2i ss = imsrc->size();
    Nimble::Vector2i is(ss.x >> 1, ss.y >> 1);

    if(is.x & 0x1) is.x--;
    if(is.y & 0x1) is.y--;

    if(is * 2 == ss) {
      imdest->quarterSize(*imsrc);
    }
    else {
      imdest->copyResample(*imsrc, is.x, is.y);
    }

    item.m_image = imdest;
    item.m_state = READY;

    if(level == DEFAULT_MAP1 || level == DEFAULT_MAP2) {
      std::string filename;
      cacheFileName(filename, level);
      Directory::mkdir(FileUtils::path(filename));
      imdest->write(filename.c_str());
    }
  }

}
