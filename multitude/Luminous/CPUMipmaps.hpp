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

#ifndef LUMINOUS_CPU_MIPMAPS_HPP
#define LUMINOUS_CPU_MIPMAPS_HPP

#include <Luminous/BGThread.hpp>
#include <Luminous/Collectable.hpp>
#include <Luminous/Image.hpp>
#include <Luminous/Task.hpp>

#include <Nimble/Matrix3.hpp>

#include <Radiant/RefPtr.hpp>

namespace Luminous {

  class GLResources;
  class GPUMipmaps;

  /** Collection of image mipmaps in the RAM/disk of the
      computer. This class is used to load and scale images from the
      disk in the background.

      It is usually used in conjunction with Luminous::GPUMipmaps.

      CPU/GPUMipmaps are typically used to handle the loading of
      images in the background. They ease the handling of large
      amounts of images, so that neither the CPU or the GPU memory is
      exceeded. The classes work in both single- and multi-threaded
      environments.
  */
  /// @todo examples
  class CPUMipmaps : Collectable
  {
  private:
  public:

    friend class GPUMipmaps;

    LUMINOUS_API CPUMipmaps();
    LUMINOUS_API virtual ~CPUMipmaps();

    /** Drop old CPU mipmaps from memory.

    @param dt delta-time

    @param purgeTime The time-limit for keeping CPUMipmaps in
    memory. If purgeTime is less than zero, the mipmap idle times
    are updated, but they are <B>not</B> deleted from memory.
     */
    LUMINOUS_API void update(float dt, float purgeTime);

    /** Returns the index of the mipmap level that would best match
    the actual output pixel resolution. */
    LUMINOUS_API int getOptimal(Nimble::Vector2f size);
    /** Returns the index of the closest available mipmap-level. */
    LUMINOUS_API int getClosest(Nimble::Vector2f size);
    /** Gets the mipmap image on level i. If the level does not
    contain a valid mipmap, then 0 is returned. */
    LUMINOUS_API Image * getImage(int i);
    /** Mark an image used. This method resets the idle-counter of the
    level, preventing it from being dropped from the memory in the
    near future. */
    LUMINOUS_API void markImage(int i);
    /** Returns true if the object has loaded enough mipmaps. */
    LUMINOUS_API bool isReady();

    /** Starts to load given file, and build the mipmaps. */
    LUMINOUS_API bool startLoading(const char * filename, bool immediate);

    /** Returns the native size of the image, in pixels. */
    const Nimble::Vector2i & nativeSize() const { return m_nativeSize;}

    /** Fetch corresponding GPU mipmaps from a resource set. If the
    GPUMipmaps object does not exist yet, it is created and
    returned. */
    LUMINOUS_API GPUMipmaps * getGPUMipmaps(GLResources *);
    LUMINOUS_API GPUMipmaps * getGPUMipmaps();
    LUMINOUS_API bool bind(GLResources *,
               const Nimble::Matrix3 & transform,
               Nimble::Vector2 pixelsize);

    /** Returns the highest possible mipmap-level. This is basically
    the level of the mipmap with native resolution. */
    int maxLevel() const { return m_maxLevel; }
    /** Returns the lowest mipmap level that is ever going to be
    created. */
    static int lowestLevel() { return 5; }
    /** Returns true if the mipmaps are still being loaded. */
    LUMINOUS_API bool isActive();
    /** Returns the aspect ratio of the image. */
    inline float aspect() const
    { return (float)m_nativeSize.x / (float)m_nativeSize.y; }

    inline bool hasAlpha() const { return m_hasAlpha; }

    /// Not finished
    LUMINOUS_API int pixelAlpha(Nimble::Vector2 relLoc);
  private:

    class CPUItem;

    class Loader : public Task
    {
    public:
      friend class CPUItem;
      Loader(Luminous::Priority prio,
         CPUMipmaps * master, CPUItem * dest, const std::string file);
      virtual ~Loader();

      virtual void doTask();

    private:
      CPUMipmaps * m_master;
      CPUItem    * m_dest;
      std::string  m_file;
    };

    class Scaler : public Task
    {
    public:
      friend class CPUMipmaps;
      friend class CPUItem;

      Scaler(Luminous::Priority prio,
         CPUMipmaps *, CPUItem * dest, CPUItem * source, int level);

      virtual ~Scaler();

      virtual void doTask();

    private:

      CPUMipmaps * m_master;
      CPUItem * m_source;
      CPUItem * m_dest;
      int       m_level;
      std::string m_file;
      bool      m_quartering;
    };

    enum {
      DEFAULT_MAP1 = 6,
      DEFAULT_MAP2 = 9,
      MAX_MAPS = 13
    };

    enum ItemState {
      WAITING,
      WORKING,
      FINISHED,
      FAILED
    };

    class CPUItem
    {
    public:
      friend class CPUMipmaps;
      friend class Loader;
      friend class Scaler;

      CPUItem();

      ~CPUItem();
      inline bool needsLoader() const
      { return (m_state != FINISHED) && (m_scaler == 0) && (m_loader == 0); }

      inline bool working() const
      {
    return (m_scaler != 0) || (m_loader != 0) || (m_state != FINISHED);
      }

    private:
      ItemState m_state;
      Scaler  * m_scalerOut;
      Scaler  * m_scaler;
      Loader  * m_loader;
      Radiant::RefPtr<Image> m_image;
      float     m_unUsed;
    };


    Luminous::Priority levelPriority(int level)
    {
      if(level <= DEFAULT_MAP1)
    return Luminous::Task::PRIORITY_NORMAL +
      Luminous::Task::PRIORITY_OFFSET_BIT_HIGHER;

      return Luminous::Task::PRIORITY_NORMAL;
    }

    bool savebleMipmap(int i)
    { return i == DEFAULT_MAP1 || i == DEFAULT_MAP2; }

    void createLevelScalers(int level);

    void cacheFileName(std::string &, int level);

    BGThread * bgt() { return BGThread::instance(); }

    bool needsLoader(int i)
    {
      CPUItem * ci = m_stack[i].ptr();
      if(!ci)
        return true;
      return ci->needsLoader();
    }

    std::string m_filename;

    Radiant::RefPtr<CPUItem> m_stack[MAX_MAPS];
    Nimble::Vector2i m_nativeSize;
    int              m_maxLevel;
    // What level files are available as mip-maps.
    uint32_t         m_fileMask;
    bool             m_hasAlpha;
    Radiant::TimeStamp m_startedLoading;

    Luminous::ImageInfo m_info;
    bool                m_ok;
  };


}

#endif
