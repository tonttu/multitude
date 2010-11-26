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

namespace {
  // after first resize modify the dimensions so that we can resize
  // 5 times with quarterSize
  const int resizes = 5;
}

namespace Luminous {

  using namespace Radiant;

  CPUMipmaps::CPUMipmaps()
    : m_fileModified(0),
    m_stack(1),
    m_nativeSize(0, 0),
    m_firstLevelSize(0, 0),
    m_maxLevel(0),
    m_hasAlpha(false),
    m_timeOut(3.0f),
    m_keepMaxLevel(true)
  {
  }

  CPUMipmaps::~CPUMipmaps()
  {
  }

  void CPUMipmaps::update(float dt, float )
  {
    Radiant::Guard g(&m_stackMutex);
    for(int i = 0; i < m_maxLevel; i++) {
      m_stack[i].m_unUsed += dt;
    }
    if(!m_keepMaxLevel)
      m_stack[m_maxLevel].m_unUsed += dt;

    Radiant::Guard g2(&m_stackChangeMutex);
    for(StackMap::iterator it = m_stackChange.begin(); it != m_stackChange.end(); ++it)
      m_stack[it->first] = it->second;
    m_stackChange.clear();
  }

  int CPUMipmaps::getOptimal(Nimble::Vector2f size)
  {
    float ask = size.maximum(), first = m_firstLevelSize.maximum();

    if(ask >= first)
      return 0;

    int bestlevel = Nimble::Math::Floor(log(ask/first) / log(0.5)) + 1;

    if(bestlevel > m_maxLevel)
      bestlevel = m_maxLevel;

    return bestlevel;
  }

  int CPUMipmaps::getClosest(Nimble::Vector2f size)
  {
    Radiant::Guard g(&m_stackMutex);

    int bestlevel = m_maxLevel;

    if (Nimble::Math::isFinite(size.x) && Nimble::Math::isFinite(size.y))
        bestlevel = getOptimal(size);
//    else
//        Radiant::error("CPUMipmaps::getClosest(): requesting image for invalid dimensions (%f,%f)", size.x, size.y);
    const CPUItem & item = m_stack[bestlevel];
    markImage(bestlevel);

    if(item.m_state == READY)
      return bestlevel;

    reschedule();
    BGThread::instance()->reschedule(this);

    // Scan for the best available mip-map.

    for(int i = bestlevel-1; i >= 0; --i) {
      if(m_stack[i].m_state == READY) {
        markImage(i);
        return i;
      }
    }

    for(int i = bestlevel+1; i <= m_maxLevel; ++i) {
      if(m_stack[i].m_state == READY) {
        markImage(i);
        return i;
      }
    }

    return -1;
  }

  std::shared_ptr<ImageTex> CPUMipmaps::getImage(int i)
  {
    const CPUItem item = getStack(i);

    std::shared_ptr<ImageTex> image = item.m_image;
    if(item.m_state != READY)
      return std::shared_ptr<ImageTex>();

    return image;
  }

  void CPUMipmaps::markImage(size_t i)
  {
    assert(i < m_stack.size());
    /// assert(is locked)
    m_stack[i].m_unUsed = 0.0f;
  }

  bool CPUMipmaps::isReady()
  {
    Radiant::Guard g(&m_stackMutex);
    for(int i = 0; i <= m_maxLevel; i++) {
      CPUItem & ci = m_stack[i];

      if(ci.m_state == WAITING && ci.m_unUsed < m_timeOut)
        return false;
    }

    return true;
  }

  bool CPUMipmaps::startLoading(const char * filename, bool)
  {
    m_filename = filename;
    m_fileModified = FileUtils::lastModified(m_filename);
    m_info = Luminous::ImageInfo();

    if(!Luminous::Image::ping(filename, m_info)) {
      error("CPUMipmaps::startLoading # failed to query image size for %s",
            filename);
      return false;
    }

    m_shouldSave.clear();
    m_stack.clear();
    m_nativeSize.make(m_info.width, m_info.height);

    if(!m_nativeSize.minimum())
      return false;

    m_firstLevelSize = m_nativeSize / 2;

    // Make sure that we can make "resizes" amount of resizes with quarterSize
    // after first resize
    const int mask = (1 << resizes) - 1;
    m_firstLevelSize.x += ((~(m_firstLevelSize.x & mask) & mask) + 1) & mask;
    m_firstLevelSize.y += ((~(m_firstLevelSize.y & mask) & mask) + 1) & mask;

    // m_maxLevel, m_firstLevelSize and m_nativeSize have to be set before running getOptimal
    m_maxLevel = std::numeric_limits<int>::max();
    m_maxLevel = getOptimal(Nimble::Vector2f(SMALLEST_IMAGE, SMALLEST_IMAGE));

    m_shouldSave.insert(getOptimal(Nimble::Vector2f(SMALLEST_IMAGE, SMALLEST_IMAGE)));
    m_shouldSave.insert(getOptimal(Nimble::Vector2f(DEFAULT_SAVE_SIZE1, DEFAULT_SAVE_SIZE1)));
    m_shouldSave.insert(getOptimal(Nimble::Vector2f(DEFAULT_SAVE_SIZE2, DEFAULT_SAVE_SIZE2)));
    m_shouldSave.erase(0);

    m_stack.resize(m_maxLevel+1);

    m_priority = Luminous::Task::PRIORITY_HIGH;
    markImage(m_maxLevel);
    reschedule();

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

    assert(gm);

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
    GPUMipmaps * gpumaps = getGPUMipmaps(r);

    return gpumaps->bind(transform, pixelsize);
  }

  bool CPUMipmaps::isActive()
  {
    Radiant::Guard g(&m_stackMutex);
    for(int i = 0; i <= m_maxLevel; i++) {

      if(m_stack[i].m_state == WAITING)
        return true;
    }

    return false;
  }

  int CPUMipmaps::pixelAlpha(Nimble::Vector2 relLoc)
  {
    // info("CPUMipmaps::pixelAlpha # %f %f", relLoc.x, relLoc.y);

    for(int i = 0; i <= m_maxLevel; ++i) {
      std::shared_ptr<ImageTex> im = getImage(i);

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

    return 255;
  }

  void CPUMipmaps::finish()
  {
    setState(Task::DONE);
    reschedule(0, 0);
  }

  Nimble::Vector2i CPUMipmaps::mipmapSize(int level)
  {
    if(level == 0) return m_nativeSize;
    if(level <= resizes+1) {
      return Nimble::Vector2i(m_firstLevelSize.x >> (level-1),
                              m_firstLevelSize.y >> (level-1));
    } else {
      Nimble::Vector2i v(m_firstLevelSize.x >> resizes,
                         m_firstLevelSize.y >> resizes);
      level -= resizes+1;
      while(level-- > 0) {
        v = v / 2;
        if (v.x == 0 || v.y == 0)
          return Nimble::Vector2i(0, 0);
      }
      return v;
    }
  }

  void CPUMipmaps::doTask()
  {
    if(state() == Task::DONE)
      return;

    double delay = 60.0;
    m_priority = Luminous::Task::PRIORITY_NORMAL;
    reschedule(delay, true);

    StackMap stack;

    for(int i = 0; i <= m_maxLevel; i++) {
      const CPUItem item = getStack(i);
      double time_to_expire = m_timeOut - item.m_unUsed;

      if(time_to_expire > 0) {
        if(item.m_state == WAITING) {
          recursiveLoad(stack, i);
        } else {
          if(time_to_expire < delay)
            delay = time_to_expire;
        }
      } else if(item.m_state == READY) { // unused image
        // info("CPUMipmaps::doTask # Dropping %s %d", m_filename.c_str(), i);
        stack[i] = item;
        stack[i].m_state = WAITING;
        stack[i].m_image.reset();
      }
    }

    /// @todo what if the task has been already re-scheduled from another thread?
    reschedule(delay+0.0001);

    if(!stack.empty()) {
      Radiant::Guard g(&m_stackChangeMutex);
      for(StackMap::iterator it = stack.begin(); it != stack.end(); ++it)
        m_stackChange[it->first] = it->second;
    }
  }

  CPUMipmaps::CPUItem CPUMipmaps::getStack(int index)
  {
    Radiant::Guard g(&m_stackMutex);
    assert(index >= 0 && index < (int) m_stack.size());
    const CPUItem item = m_stack[index];
    return item;
  }

  void CPUMipmaps::cacheFileName(std::string & name, int level)
  {
    char buf[32];

    name = Radiant::FileUtils::path(m_filename);

    if(!name.empty())
      name += "/";
    name += ".imagecache/";

    snprintf(buf, sizeof(buf), "level%02d_", level);

    name += buf;
    name += Radiant::FileUtils::filename(m_filename);

    // Put in the right suffix
    size_t i = name.size() - 1;

    while(i && name[i] != '.' && name[i] != '/')
      i--;

    name.erase(i + 1);

    // always use png
    name += "png";
  }

  void CPUMipmaps::recursiveLoad(StackMap & stack, int level)
  {
    if(getStack(level).m_state == READY)
      return;

    CPUItem & item = stack[level];
    if(item.m_state == READY)
      return;

    if(m_shouldSave.find(level) != m_shouldSave.end()) {
      // Try loading a pre-generated smaller-scale mipmap

      std::string filename;
      cacheFileName(filename, level);

      if(Radiant::FileUtils::fileReadable(filename) &&
         FileUtils::lastModified(filename) > m_fileModified) {

        Luminous::ImageTex * im = new ImageTex();

        if(!im->read(filename.c_str())) {
          error("CPUMipmaps::recursiveLoad # Could not read %s", filename.c_str());
          delete im;
        } else if(mipmapSize(level) != im->size()) {
          // unexpected size (corrupted or just old image)
          error("CPUMipmaps::recursiveLoad # Cache image '%s'' size was (%d, %d), expected (%d, %d)",
                filename.c_str(), im->width(), im->height(), mipmapSize(level).x, mipmapSize(level).y);
          delete im;
        } else {
          if(im->hasAlpha())
            m_hasAlpha = true;

          item.m_image.reset(im);
          item.m_state = READY;
          return;
        }
      }
    }

    if(level == 0) {
      // Load original
      std::shared_ptr<Luminous::ImageTex> im(new ImageTex);

      if(!im->read(m_filename.c_str())) {
        error("CPUMipmaps::recursiveLoad # Could not read %s", m_filename.c_str());
        item.m_state = FAILED;
      }
      else {
        if(im->hasAlpha())
          m_hasAlpha = true;
        item.m_image = im;
        item.m_state = READY;
      }
      return;
    }

    // Load the bigger image from lower level, and scale down from that:
    recursiveLoad(stack, level - 1);

    std::shared_ptr<Luminous::ImageTex> imsrc;
    const CPUItem level1 = getStack(level-1);
    if(level1.m_state == READY) {
      imsrc = level1.m_image;
    } else {
      if(stack[level-1].m_state != READY) {
        error("Failed to get mipmap %d", level - 1);
        item.m_state = FAILED;
        return;
      }
      imsrc = stack[level-1].m_image;
    }

    // Scale down from bigger mipmap
    std::shared_ptr<Luminous::ImageTex> imdest(new Luminous::ImageTex());

    Nimble::Vector2i ss = imsrc->size();
    // Nimble::Vector2i is = level == 1 ? m_firstLevelSize : ss / 2;
    Nimble::Vector2i is = mipmapSize(level);

    if(is * 2 == ss) {
      imdest->quarterSize(*imsrc);
    }
    else {
      imdest->copyResample(*imsrc, is.x, is.y);
    }

    item.m_image = imdest;
    item.m_state = READY;

    // info("Loaded image %s %d", m_filename.c_str(), is.x);

    if(m_shouldSave.find(level) != m_shouldSave.end()) {
      std::string filename;
      cacheFileName(filename, level);
      Directory::mkdir(FileUtils::path(filename));
      imdest->write(filename.c_str());
      // info("wrote cache %s (%d)", filename.c_str(), level);
    }
  }

  void CPUMipmaps::reschedule(double delay, bool allowLater)
  {
    Radiant::Guard g(m_rescheduleMutex);
    Radiant::TimeStamp next = Radiant::TimeStamp::getTime() +
                              Radiant::TimeStamp::createSecondsD(delay);
    if(allowLater || next < scheduled())
      schedule(next);
  }
}
