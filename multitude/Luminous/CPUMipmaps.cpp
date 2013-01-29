/* COPYRIGHT
 */

#include "CPUMipmaps.hpp"

#ifndef LUMINOUS_OPENGLES
# include "MipMapGenerator.hpp"
#endif

#include <Luminous/RenderContext.hpp>

#include <Radiant/PlatformUtils.hpp>
#include <Radiant/Directory.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

#include <cassert>
#include <fstream>

#include <QFileInfo>
#include <QCryptographicHash>
#include <QDir>

PUSH_IGNORE_DEPRECATION_WARNINGS

namespace {
  // after first resize modify the dimensions so that we can resize
  // 5 times with quarterSize
  const int resizes = 5;
}

#ifdef CPUMIPMAPS_PROFILING
struct ProfileData
{
  ProfileData() : totalTime(0.0), timesLoaded(0) {}
  double totalTime;
  int timesLoaded;
  std::string filename;
};
class Profiler
{
public:
  ProfileData & next()
  {
    Radiant::Guard g(m_mutex);
    m_lst.push_back(ProfileData());
    return m_lst.back();
  }

  ~Profiler()
  {
    std::multimap<double, ProfileData*> sorted;
    for(std::list<ProfileData>::iterator it = m_lst.begin(); it != m_lst.end(); ++it) {
      sorted.insert(std::make_pair(-it->totalTime, &*it));
    }
    for(std::multimap<double, ProfileData*>::iterator it = sorted.begin(); it != sorted.end(); ++it) {
      const ProfileData & d = *it->second;
      std::cout << d.filename.toUtf8().data() << " : " << d.totalTime << " (" << d.timesLoaded << ")" << std::endl;
    }
  }

private:
  Radiant::Mutex m_mutex;
  std::list<ProfileData> m_lst;
};
static Profiler s_profiler;
#endif

namespace Luminous {

  // DXT support is tested in Luminous::initLuminous()
  bool CPUMipmaps::s_dxtSupported = true;

  static Radiant::Mutex s_storeMutex;

  static std::map<std::pair<QString, unsigned long>, std::weak_ptr<CPUMipmaps> > s_mipmaps;

  std::shared_ptr<CPUMipmaps> CPUMipmaps::acquire(const QString & filename,
                                                  bool compressed_mipmaps)
  {
    Radiant::Guard g(s_storeMutex);

    // Check the timestamp
    Radiant::TimeStamp lastMod = Radiant::FileUtils::lastModified(filename);
    const std::pair<QString, unsigned long> key = std::make_pair(filename, lastMod.value());

    /// @todo filename should be resolved (with ResourceLocator) absolute path
    std::weak_ptr<CPUMipmaps> & mipmap_weak = s_mipmaps[key];

    // Check if ptr still points to something valid
    std::shared_ptr<CPUMipmaps> mipmap_shared = mipmap_weak.lock();
    if (mipmap_shared)
      return mipmap_shared;

    mipmap_shared.reset(new CPUMipmaps);

    if(!mipmap_shared->startLoading(filename.toUtf8().data(), compressed_mipmaps)) {
      return std::shared_ptr<CPUMipmaps>();
    }

    // store new weak pointer
    mipmap_weak = mipmap_shared;

//    debugLuminous("CPUMipmaps::acquire # Created new for [%s %ld] (%ld links)",
//                  filename.toUtf8().data(), lastMod, s_mipmaps[key].use_count());

    return mipmap_shared;
  }

  CPUMipmaps::CPUMipmaps()
    : Task(),
    m_fileModified(0),
    m_stack(1),
    m_nativeSize(0, 0),
    m_firstLevelSize(0, 0),
    m_maxLevel(0),
    m_hasAlpha(false),
    m_timeOutCPU(5.0f),
    m_timeOutGPU(5.0f),
    m_keepMaxLevel(true),
    m_loadingPriority(PRIORITY_NORMAL),
    m_compressedMipmaps(false)
  #ifdef CPUMIPMAPS_PROFILING
    , m_profile(s_profiler.next())
  #endif
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

    // if the size is really small, the calculation later does funny things
    if(ask <= (int(first) >> m_maxLevel))
      return m_maxLevel;

    int bestlevel = std::floor(log(ask / first) / log(0.5)) + 1;

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
    Radiant::BGThread::instance()->reschedule(shared_from_this(), m_loadingPriority);

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

#ifndef LUMINOUS_OPENGLES
  std::shared_ptr<CompressedImageTex> CPUMipmaps::getCompressedImage(int i)
  {
    CPUItem item = getStack(i);

    std::shared_ptr<CompressedImageTex> image = item.m_compressedImage;
    if(item.m_state != READY) {
      return std::shared_ptr<CompressedImageTex>();
    }

    return image;
  }
#endif // LUMINOUS_OPENGLES

  void CPUMipmaps::markImage(size_t i)
  {
    assert(i < m_stack.size());
    /// assert(is locked)
    m_stack[i].m_lastUsed = Radiant::TimeStamp::currentTime();
  }

  bool CPUMipmaps::isReady()
  {
    Radiant::Guard g(m_stackMutex);
    for(int i = 0; i <= m_maxLevel; i++) {
      CPUItem & ci = m_stack[i];

      if(ci.m_state == WAITING && ci.sinceLastUse() < m_timeOutCPU)
        return false;
    }

    return true;
  }

  bool CPUMipmaps::startLoading(const char * filename, bool compressedMipmaps)
  {
#ifdef CPUMIPMAPS_PROFILING
    m_profile.filename = filename;
#endif

    if(!QFile::exists(filename)) {
      Radiant::error("CPUMipmaps::startLoading # file '%s' does not exist", filename);
      return false;
    }

    m_filename = filename;
    m_compFilename.clear();
    m_fileModified = Radiant::FileUtils::lastModified(m_filename);
    m_info = Luminous::ImageInfo();
    m_shouldSave.clear();
    m_stack.clear();

    // Use DXT compression if it is requested and supported
    m_compressedMipmaps = (compressedMipmaps && s_dxtSupported);

#ifndef LUMINOUS_OPENGLES

    std::shared_ptr<MipMapGenerator> gen;
    if(m_compressedMipmaps) {
      m_compFilename = cacheFileName(filename, -1, "dds");

      Radiant::TimeStamp ts(0);
      if(QFile::exists(m_compFilename))
        ts = Radiant::FileUtils::lastModified(m_compFilename);

      if(ts == Radiant::TimeStamp(0)) {
        // Cache file does not exist. Check if we want to generate mipmaps for
        // this file, or does it have those already
        if(!Luminous::Image::ping(filename, m_info)) {
          Radiant::error("CPUMipmaps::startLoading # failed to query image size for %s", filename);
          return false;
        }
        if(m_info.pf.compression() && (m_info.mipmaps > 1 ||
                                       (m_info.width < 5 && m_info.height < 5))) {
          // We already have compressed image with mipmaps, no need to generate more
          m_compFilename.clear();
          m_compressedMipmaps = false;
        }
      }
      if(m_compressedMipmaps && (ts < m_fileModified ||
                                 !Luminous::Image::ping(m_compFilename.toUtf8().data(), m_info))) {
        gen.reset(new MipMapGenerator(filename, m_compFilename));
        std::shared_ptr<CPUMipmaps> self = std::static_pointer_cast<CPUMipmaps>(shared_from_this());
        gen->setListener([=] (const ImageInfo & info) { self->mipmapsReady(info); } );
      }
    }
#endif // LUMINOUS_OPENGLES

    if(m_info.width == 0 && !Luminous::Image::ping(filename, m_info)) {
      Radiant::error("CPUMipmaps::startLoading # failed to query image size for %s", filename);
      return false;
    }

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
      m_maxLevel = std::min(m_maxLevel, m_info.mipmaps - 1);
    }

    m_shouldSave.insert(getOptimal(Nimble::Vector2f(SMALLEST_IMAGE, SMALLEST_IMAGE)));
    m_shouldSave.insert(getOptimal(Nimble::Vector2f(DEFAULT_SAVE_SIZE1, DEFAULT_SAVE_SIZE1)));
    m_shouldSave.insert(getOptimal(Nimble::Vector2f(DEFAULT_SAVE_SIZE2, DEFAULT_SAVE_SIZE2)));
    // Don't save the original image as mipmap
    m_shouldSave.erase(0);

    m_stack.resize(m_maxLevel+1);

    m_priority = PRIORITY_HIGH;
    markImage(m_maxLevel);
    reschedule();

#ifndef LUMINOUS_OPENGLES
    if(gen)
      Radiant::BGThread::instance()->addTask(gen);
    else
#endif // LUMINOUS_OPENGLES
      Radiant::BGThread::instance()->addTask(shared_from_this());

    return true;
  }

  bool CPUMipmaps::bind(Nimble::Vector2 pixelSize, GLenum textureUnit)
  {
    return bind(RenderContext::getThreadContext(), pixelSize, textureUnit);
  }

  bool CPUMipmaps::bind(const Nimble::Matrix3 &transform, Nimble::Vector2 pixelSize, GLenum textureUnit)
  {
    return bind(RenderContext::getThreadContext(), transform, pixelSize, textureUnit);
  }

  bool CPUMipmaps::bind(RenderContext * resources, const Nimble::Matrix3 &transform, Nimble::Vector2 pixelSize, GLenum textureUnit)
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

  bool CPUMipmaps::bind(RenderContext * resources, Nimble::Vector2 pixelSize, GLenum textureUnit)
  {
    StateInfo & si = m_stateInfo.ref(resources);
    si.bound = -1;
    si.optimal = getOptimal(pixelSize);

    // Find the best available mipmap
    int bestAvailable = getClosest(pixelSize);
    if(bestAvailable < 0)
      return false;

    // Mark the mipmap that it has been used
    markImage(bestAvailable);

#ifndef LUMINOUS_OPENGLES
    // Handle compressed images
    if(m_info.pf.compression()) {
      si.bound = bestAvailable;
      std::shared_ptr<CompressedImageTex> img = getCompressedImage(bestAvailable);
      img->bind(resources, textureUnit);
      return true;
    }
#endif // LUMINOUS_OPENGLES

    // Handle non-compressed images
    std::shared_ptr<ImageTex> img = getImage(bestAvailable);

    if(img->isFullyLoadedToGPU()) {
      si.bound = bestAvailable;
      img->bind(resources, textureUnit, false);
      //Luminous::Utils::glCheck("GPUMipmaps::bind # 1");
      return true;
    }

    // Do progressive upload
    Luminous::Texture2D & tex = img->ref(resources);

    // We must allocate the texture memory before we can upload anything
    if(tex.generation() != img->generation()) {

      //Radiant::warning("CPUMipmaps::bind # texture and image generations don't match, reallocate texture memory");

      // Let the driver decide what internal format to use
      GLenum internalFormat = img->pixelFormat().numChannels();

      tex.loadBytes(internalFormat, img->width(), img->height(), NULL, img->pixelFormat(), false);
      tex.setGeneration(img->generation());
    }

    bool fullyUploaded = tex.progressiveUpload(resources, textureUnit, *img);

    if(fullyUploaded)
      return true;

    // If the requested texture is not fully uploaded, test if there is
    // anything we can use already
    for(size_t i = 0; i < stackSize(); i++) {

      std::shared_ptr<ImageTex> test = getImage((int) i);
      if(!test)
        continue;

      if(test->isFullyLoadedToGPU(resources) && test->bind(resources, textureUnit, false)) {
        si.bound = (int)i;
        return true;
      }
    }

    return false;
  }

  CPUMipmaps::StateInfo CPUMipmaps::stateInfo(RenderContext * resources)
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
    // Radiant::info("CPUMipmaps::pixelAlpha # %f %f", relLoc.x, relLoc.y);

    for(int i = 0; i <= m_maxLevel; ++i) {
#ifndef LUMINOUS_OPENGLES
      if(m_info.pf.compression()) {
        std::shared_ptr<CompressedImage> c = getCompressedImage(i);
        if(!c) continue;

        Nimble::Vector2i pixel(relLoc.x * c->width(), relLoc.y * c->height());

        return 255 * c->readAlpha(pixel);
      }
#endif // LUMINOUS_OPENGLES

      std::shared_ptr<ImageTex> im = getImage(i);

      if(!im) continue;

      Nimble::Vector2f locf(im->size().x, im->size().y);
      locf.scale(relLoc);

      Vector2i loci(locf.x, locf.y);

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
        Radiant::error("CPUMipmaps::pixelAlpha # Unsupported pixel format");
        return 255;
      }
    }

    return 255;
  }

  void CPUMipmaps::finish()
  {
    setState(Task::DONE);
    m_priority = PRIORITY_LOW;
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

  void CPUMipmaps::mipmapsReady(const ImageInfo & info)
  {
    m_info = info;
    Radiant::BGThread::instance()->addTask(shared_from_this());
    reschedule();
  }

  void CPUMipmaps::doTask()
  {
    if(state() == Task::DONE)
      return;

    double delay = 3600.0;
    m_priority = PRIORITY_LOW;
    // sets the m_scheduled time to somewhere future, it can be decreased later
    reschedule(delay, true);

    StackMap removed_stack;

    for(int i = 0; i <= m_maxLevel; i++) {
      CPUItem item = getStack(i);
      double time_to_expire_cpu = m_timeOutCPU - item.sinceLastUse();
      double time_to_expire_gpu = m_timeOutGPU - item.sinceLastUse();

      if(time_to_expire_cpu > 0) {
        if(item.m_state == WAITING) {
          StackMap stack;
#ifdef CPUMIPMAPS_PROFILING
          Radiant::TimeStamp ts(Radiant::TimeStamp::currentTime());
#endif
          recursiveLoad(stack, i);
#ifdef CPUMIPMAPS_PROFILING
          m_profile.totalTime += ts.sinceSecondsD()*1000.0;
          m_profile.timesLoaded++;
#endif
          if(!stack.empty()) {
            item = stack[i];
            Radiant::Guard g(m_stackMutex);
            for(StackMap::iterator it = stack.begin(); it != stack.end(); ++it)
              m_stack[it->first] = it->second;
          }
        }
        if(time_to_expire_cpu < delay)
          delay = time_to_expire_cpu;

        if(time_to_expire_gpu < 0) {
          removed_stack[i] = item;
          removed_stack[i].dropFromGPU();
        } else if(time_to_expire_gpu < delay)
          delay = time_to_expire_gpu;
      } else if((!m_keepMaxLevel || i != m_maxLevel) && item.m_state == READY) {
        // (time_to_expire <= 0) -> free the image

        //info("CPUMipmaps::doTask # Dropping %s %d", m_filename.c_str(), i);
        /// @todo should only drop the cpu part, but keep gpu untouched (unless time_to_expire_gpu > 0)
        removed_stack[i].clear();
      }
    }

    if(!removed_stack.empty()) {
      Radiant::Guard g(m_stackMutex);
      for(StackMap::iterator it = removed_stack.begin(); it != removed_stack.end(); ++it)
        m_stack[it->first] = it->second;
    }

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

  QString CPUMipmaps::cacheFileName(const QString & src, int level,
                                    const QString & suffix)
  {
    QFileInfo fi(src);

    static QString s_basePath;

    MULTI_ONCE {
      QString basePath = Radiant::PlatformUtils::getModuleUserDataPath("MultiTouch", false) + "/imagecache";
      if(!QDir().mkpath(basePath)) {
        basePath = QDir::tempPath() + "/cornerstone-imagecache";
        QDir().mkpath(basePath);
      }
      s_basePath = basePath;
    }

    // Compute MD5 from the absolute path
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(fi.absoluteFilePath().toUtf8());

    const QString md5 = hash.result().toHex();

    // Avoid putting all mipmaps into the same folder (because of OS performance)
    const QString prefix = md5.left(2);
    const QString postfix = level < 0 ? QString(".%1").arg(suffix) :
        QString("_level%1.%2").arg(level, 2, 10, QLatin1Char('0')).arg(suffix);

    const QString fullPath = s_basePath + QString("/%1/%2%3").arg(prefix).arg(md5).arg(postfix);

    return fullPath;
  }

  void CPUMipmaps::recursiveLoad(StackMap & stack, int level)
  {
    if(getStack(level).m_state == READY)
      return;

    CPUItem & item = stack[level];
    if(item.m_state == READY)
      return;
    item.m_lastUsed = Radiant::TimeStamp::currentTime();

#ifndef LUMINOUS_OPENGLES
    if(m_info.pf.compression()) {
      std::shared_ptr<Luminous::CompressedImageTex> im(new Luminous::CompressedImageTex);
      QString filename = m_compFilename.isEmpty() ? m_filename : m_compFilename;
      if(!im->read(filename, level)) {
        Radiant::error("CPUMipmaps::recursiveLoad # Could not read %s level %d", filename.toUtf8().data(), level);
        item.m_state = FAILED;
      } else {
        /// @todo this is wrong, compressed images doesn't always have alpha channel
        m_hasAlpha = true;
        item.m_image.reset();
        item.m_compressedImage = im;
        item.m_state = READY;
      }
      return;
    }
#endif // LUMINOUS_OPENGLES

    if(level == 0) {
      // Load original
      std::shared_ptr<Luminous::ImageTex> im(new ImageTex);

      if(!im->read(m_filename.toUtf8().data())) {
        Radiant::error("CPUMipmaps::recursiveLoad # Could not read %s", m_filename.toUtf8().data());
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
      QString filename = cacheFileName(m_filename, level);

      if(Radiant::FileUtils::fileReadable(filename) &&
         Radiant::FileUtils::lastModified(filename) > m_fileModified) {

        Luminous::ImageTex * im = new ImageTex();

        if(!im->read(filename.toUtf8().data())) {
          Radiant::error("CPUMipmaps::recursiveLoad # Could not read %s", filename.toUtf8().data());
          delete im;
        } else if(mipmapSize(level) != im->size()) {
          // unexpected size (corrupted or just old image)
          Radiant::error("CPUMipmaps::recursiveLoad # Cache image '%s'' size was (%d, %d), expected (%d, %d)",
                filename.toUtf8().data(), im->width(), im->height(), mipmapSize(level).x, mipmapSize(level).y);
          delete im;
        } else {
          if(im->hasAlpha())
            m_hasAlpha = true;

          // Radiant::info("Loaded cache %s %d from file", m_filename.c_str(), level);

          item.m_image.reset(im);
          item.m_state = READY;
          return;
        }
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
        Radiant::error("Failed to get mipmap %d", level - 1);
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
      bool ok = imdest->quarterSize(*imsrc);
      if(!ok)
        Radiant::error("CPUMipmaps::recursiveLoad # failed to resize image");
      assert(ok);
    }
    else {
      //imdest->copyResample(*imsrc, is.x, is.y);
      imdest->minify(*imsrc, is.x, is.y);
    }

    item.m_image = imdest;
    item.m_state = READY;

    // Radiant::info("Loaded image %s %d", m_filename.c_str(), is.x);

    if(m_shouldSave.find(level) != m_shouldSave.end()) {
      QString filename = cacheFileName(m_filename, level);
      Radiant::Directory::mkdirRecursive(Radiant::FileUtils::path(filename));
      imdest->write(filename.toUtf8().data());
      // Radiant::info("wrote cache %s (%d)", filename.c_str(), level);
    }
  }

  void CPUMipmaps::reschedule(double delay, bool allowLater)
  {
    Radiant::TimeStamp next = Radiant::TimeStamp::currentTime() +
                              Radiant::TimeStamp::createSeconds(delay);
    if(allowLater || next < scheduled())
      schedule(next);
  }
}

POP_IGNORE_DEPRECATION_WARNINGS
