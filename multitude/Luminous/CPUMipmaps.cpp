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

#include <Luminous/GLResources.hpp>
#include <Luminous/Utils.hpp>

#include <Radiant/PlatformUtils.hpp>
#include <Radiant/Directory.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

#include <cassert>
#include <fstream>

#include <QFileInfo>
#include <QCryptographicHash>

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
    : Task(),
    m_fileModified(0),
    m_stack(1),
    m_nativeSize(0, 0),
    m_firstLevelSize(0, 0),
    m_maxLevel(0),
    m_hasAlpha(false),
    m_timeOut(5.0f),
    m_keepMaxLevel(true)
  {
  }

  CPUMipmaps::~CPUMipmaps()
  {
  }

  int CPUMipmaps::getOptimal(Nimble::Vector2f size)
  {
    float ask = size.maximum();
    // Dimension of the first mipmap level (quarter-size from original)
    float first = m_firstLevelSize.maximum();

    // Use the original image (level 0) if asked for bigger than first level mipmap
    if(ask >= first)
      return 0;

    int bestlevel = Nimble::Math::Floor(log(ask / first) / log(0.5)) + 1;

    if(bestlevel > m_maxLevel)
      bestlevel = m_maxLevel;

    return bestlevel;
  }

  int CPUMipmaps::getClosest(Nimble::Vector2f size)
  {
    Radiant::Guard g(m_stackMutex);

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
    CPUItem item = getStack(i);

    std::shared_ptr<ImageTex> image = item.m_image;
    if(item.m_state != READY) {
      return std::shared_ptr<ImageTex>();
    }

    return image;
  }

  std::shared_ptr<CompressedImageTex> CPUMipmaps::getCompressedImage(int i)
  {
    CPUItem item = getStack(i);

    std::shared_ptr<CompressedImageTex> image = item.m_compressedImage;
    if(item.m_state != READY) {
      return std::shared_ptr<CompressedImageTex>();
    }

    return image;
  }

  void CPUMipmaps::markImage(size_t i)
  {
    assert(i < m_stack.size());
    /// assert(is locked)
    m_stack[i].m_lastUsed = Radiant::TimeStamp::getTime();
  }

  bool CPUMipmaps::isReady()
  {
    Radiant::Guard g(m_stackMutex);
    for(int i = 0; i <= m_maxLevel; i++) {
      CPUItem & ci = m_stack[i];

      if(ci.m_state == WAITING && ci.sinceLastUse() < m_timeOut)
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
      error("CPUMipmaps::startLoading # failed to query image size for %s", filename);
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
    if(m_info.pf.compression()) {
      m_maxLevel = Nimble::Math::Min(m_maxLevel, m_info.mipmaps - 1);
    }

    m_shouldSave.insert(getOptimal(Nimble::Vector2f(SMALLEST_IMAGE, SMALLEST_IMAGE)));
    m_shouldSave.insert(getOptimal(Nimble::Vector2f(DEFAULT_SAVE_SIZE1, DEFAULT_SAVE_SIZE1)));
    m_shouldSave.insert(getOptimal(Nimble::Vector2f(DEFAULT_SAVE_SIZE2, DEFAULT_SAVE_SIZE2)));
    // Don't save the original image as mipmap
    m_shouldSave.erase(0);

    m_stack.resize(m_maxLevel+1);

    m_priority = Luminous::Task::PRIORITY_HIGH;
    markImage(m_maxLevel);
    reschedule();

    return true;
  }

  bool CPUMipmaps::bind(Nimble::Vector2 pixelSize, GLenum textureUnit)
  {
    return bind(GLResources::getThreadResources(), pixelSize, textureUnit);
  }

  bool CPUMipmaps::bind(const Nimble::Matrix3 &transform, Nimble::Vector2 pixelSize, GLenum textureUnit)
  {
    return bind(GLResources::getThreadResources(), transform, pixelSize, textureUnit);
  }

  bool CPUMipmaps::bind(GLResources * resources, const Nimble::Matrix3 &transform, Nimble::Vector2 pixelSize, GLenum textureUnit)
  {
    // Transform the corners and compute the lengths of the sides of the transformed rectangle.
    // We use the maximum of the edge lengths to get sheared textures appear correctly.
    Nimble::Vector2 lb = transform.project(0, 0);
    Nimble::Vector2 rb = transform.project(pixelSize.x, 0);
    Nimble::Vector2 lt = transform.project(0, pixelSize.y);
    Nimble::Vector2 rt = transform.project(pixelSize.x, pixelSize.y);

    float x1 = (rb - lb).length();
    float x2 = (rt - lt).length();

    float y1 = (lt - lb).length();
    float y2 = (rt - rb).length();

    return bind(resources, Nimble::Vector2(std::max(x1, x2), std::max(y1, y2)), textureUnit);
  }

  bool CPUMipmaps::bind(GLResources * resources, Nimble::Vector2 pixelSize, GLenum textureUnit)
  {
    StateInfo & si = m_stateInfo.ref(resources);
    si.binded = -1;
    si.optimal = getOptimal(pixelSize);

    // Find the best available mipmap
    int bestAvailable = getClosest(pixelSize);
    if(bestAvailable < 0)
      return false;

    // Mark the mipmap that it has been used
    markImage(bestAvailable);

    if(m_info.pf.compression()) {
      si.binded = bestAvailable;
      std::shared_ptr<CompressedImageTex> img = getCompressedImage(bestAvailable);
      img->bind(resources, textureUnit);
      return true;
    }

    std::shared_ptr<ImageTex> img = getImage(bestAvailable);

    if(img->isFullyLoadedToGPU()) {
      si.binded = bestAvailable;
      img->bind(resources, textureUnit, false);
      Luminous::Utils::glCheck("GPUMipmaps::bind # 1");
      return true;
    }

    // Limit how many pixels we can upload immediately
    /// @todo this should be a global per frame limit in bytes
    const size_t instantUploadPixelLimit = 1.5e6;

    // Upload the whole texture at once if possible
    const size_t imagePixels = img->width() * img->height();
    if(imagePixels < instantUploadPixelLimit) {

      si.binded = bestAvailable;
      img->bind(resources, textureUnit, false);

      return true;

    } else {
      // Texture is too big, do partial upload
      img->uploadBytesToGPU(resources, instantUploadPixelLimit);

      if(img->isFullyLoadedToGPU()) {
        si.binded = bestAvailable;
        img->bind(resources, textureUnit, false);

        return true;
      }

      // If the requested texture is not fully uploaded, test if there is
      // anything we can use already
      for(size_t i = 0; i < stackSize(); i++) {

        std::shared_ptr<ImageTex> test = getImage(i);
        if(!test)
          continue;

        size_t area = test->width() * test->height();

        // If the texture is fully uploaded or its small enough, we bypass the
        // pixel budget and upload it anyway (ImageTex::bind() uploads the
        // texture as a side-effect).

        if(test->isFullyLoadedToGPU() || (area < (instantUploadPixelLimit / 3))) {
          si.binded = i;
          test->bind(resources, textureUnit, false);

          return true;
        }
      }
    }
    return false;
  }

  CPUMipmaps::StateInfo CPUMipmaps::stateInfo(GLResources * resources)
  {
    return m_stateInfo.ref(resources);
  }

  bool CPUMipmaps::isActive()
  {
    Radiant::Guard g(m_stackMutex);
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

    StackMap removed_stack;

    for(int i = 0; i <= m_maxLevel; i++) {
      const CPUItem item = getStack(i);
      double time_to_expire = m_timeOut - item.sinceLastUse();

      if(time_to_expire > 0) {
        if(item.m_state == WAITING) {
          StackMap stack;
          recursiveLoad(stack, i);
          if(!stack.empty()) {
            Radiant::Guard g(m_stackMutex);
            for(StackMap::iterator it = stack.begin(); it != stack.end(); ++it)
              m_stack[it->first] = it->second;
          }
        } else {
          if(time_to_expire < delay)
            delay = time_to_expire;
        }
      } else if((!m_keepMaxLevel || i != m_maxLevel) && item.m_state == READY) {
        // (time_to_expire <= 0) -> free the image

        //info("CPUMipmaps::doTask # Dropping %s %d", m_filename.c_str(), i);
        removed_stack[i] = item;
        removed_stack[i].m_state = WAITING;
        removed_stack[i].m_image.reset();
      }
    }

    if(!removed_stack.empty()) {
      Radiant::Guard g(m_stackMutex);
      for(StackMap::iterator it = removed_stack.begin(); it != removed_stack.end(); ++it)
        m_stack[it->first] = it->second;
    }

    /// @todo what if the task has been already re-scheduled from another thread?
    /// The little threshold is just for making sure that the image is surely expired by then
    reschedule(delay+0.001);
  }

  CPUMipmaps::CPUItem CPUMipmaps::getStack(int index)
  {
    Radiant::Guard g(m_stackMutex);

    assert(index >= 0 && index < (int) m_stack.size());

    const CPUItem item = m_stack[index];
    return item;
  }

  void CPUMipmaps::cacheFileName(std::string & name, int level)
  {
    QFileInfo fi(QString::fromUtf8(m_filename.c_str()));

    QString basePath = QString::fromUtf8(Radiant::PlatformUtils::getModuleUserDataPath("MultiTouch", false).c_str());

    // Compute MD5 from the absolute path
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(fi.absoluteFilePath().toUtf8());

    QString md5 = hash.result().toHex();

    // Avoid putting all mipmaps into the same folder (because of OS performance)
    QString prefix = md5.left(2);
    QString postfix = QString("level%1.png").arg(level, 2, 10, QLatin1Char('0'));
    QString fullPath = basePath + QString("/imagecache/%1/%2_%3").arg(prefix).arg(md5).arg(postfix);

    name = fullPath.toUtf8().data();

    //Radiant::info("CPUMipmaps::cacheFileName # %s -> %s", m_filename.c_str(), name.c_str());
  }

  void CPUMipmaps::recursiveLoad(StackMap & stack, int level)
  {
    if(getStack(level).m_state == READY)
      return;

    CPUItem & item = stack[level];
    if(item.m_state == READY)
      return;
    item.m_lastUsed = Radiant::TimeStamp::getTime();

    if(m_info.pf.compression()) {
      std::shared_ptr<Luminous::CompressedImageTex> im(new Luminous::CompressedImageTex);
      if(!im->read(m_filename, level)) {
        error("CPUMipmaps::recursiveLoad # Could not read %s level %d", m_filename.c_str(), level);
        item.m_state = FAILED;
      } else {
        m_hasAlpha = true;
        item.m_image.reset();
        item.m_compressedImage = im;
        item.m_state = READY;
      }
      return;
    }

    if(level == 0) {
      // Load original
      std::shared_ptr<Luminous::ImageTex> im(new ImageTex);

      if(!im->read(m_filename.c_str())) {
        error("CPUMipmaps::recursiveLoad # Could not read %s", m_filename.c_str());
        item.m_state = FAILED;
      } else {
        if(im->hasAlpha())
          m_hasAlpha = true;
        //info("Loaded original %s %d from file", m_filename.c_str(), level);

        item.m_image = im;
        item.m_state = READY;
      }
      return;
    }

    // Could the mipmap be already saved on disk?
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

          info("Loaded cache %s %d from file", m_filename.c_str(), level);

          item.m_image.reset(im);
          item.m_state = READY;
          return;
        }
      } else {
        info("Failed to load cache file %s", filename.c_str());
      }
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
      Directory::mkdirRecursive(FileUtils::path(filename));
      imdest->write(filename.c_str());
      // info("wrote cache %s (%d)", filename.c_str(), level);
    }
  }

  void CPUMipmaps::reschedule(double delay, bool allowLater)
  {
    Radiant::TimeStamp next = Radiant::TimeStamp::getTime() +
                              Radiant::TimeStamp::createSecondsD(delay);
    if(allowLater || next < scheduled())
      schedule(next);
  }
}
