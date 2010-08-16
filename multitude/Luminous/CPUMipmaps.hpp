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

#include <limits>

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

      Mipmap level 0 is the original image, level 1 is the
      original image * 0.5 etc.
  */
  /// @todo examples
  class CPUMipmaps : public Luminous::Collectable, public Luminous::Task
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
    LUMINOUS_API std::shared_ptr<ImageTex> getImage(int i);
    /** Mark an image used. This method resets the idle-counter of the
    level, preventing it from being dropped from the memory in the
    near future. */
    LUMINOUS_API void markImage(int i);
    /** Returns true if the object has loaded enough mipmaps. */
    /// @todo what does "enought" mean?
    LUMINOUS_API bool isReady();

    /** Starts to load given file, and build the mipmaps. */
    LUMINOUS_API bool startLoading(const char * filename, bool immediate);

    /** Returns the native size of the image, in pixels. */
    const Nimble::Vector2i & nativeSize() const { return m_nativeSize;}

    /** Fetch corresponding GPU mipmaps from a resource set. If the
    GPUMipmaps object does not exist yet, it is created and
    returned. */
    LUMINOUS_API GPUMipmaps * getGPUMipmaps(GLResources *);
    /// @copydoc getGPUMipmaps
    LUMINOUS_API GPUMipmaps * getGPUMipmaps();
    /// Binds a texture that matches the given size parameters.
    LUMINOUS_API bool bind(GLResources *,
                           const Nimble::Matrix3 & transform,
                           Nimble::Vector2 pixelsize);

    /** Returns true if the mipmaps are still being loaded. */
    LUMINOUS_API bool isActive();
    /** Returns the aspect ratio of the image. */
    inline float aspect() const
    { return (float) m_nativeSize.x / (float)m_nativeSize.y; }

    /// Returns true if the images have alpha channel
    inline bool hasAlpha() const { return m_hasAlpha; }

    /// Not finished
    LUMINOUS_API int pixelAlpha(Nimble::Vector2 relLoc);

    /// Mark this object as done
    LUMINOUS_API void finish();

    /// Returns the number of images in the stack
    inline unsigned stackSize() const { return (unsigned) m_stack.size(); }
  protected:

    LUMINOUS_API virtual void doTask();

  private:

    // Load given level from a file
    bool doLoad(int level);
    // Scale down given level from the level above
    bool doScale(int level);

    enum ItemState {
      WAITING,
      READY,
      FAILED
    };

    class CPUItem
    {
    public:
      friend class CPUMipmaps;

      CPUItem() { clear(); }

      void clear()
      {
        m_state = WAITING;
        m_image.reset();
        m_unUsed = std::numeric_limits<float>::max();
      }

    private:
      ItemState m_state;
      std::shared_ptr<ImageTex> m_image;
      float     m_unUsed;
    };

    typedef std::map<int, CPUItem> StackMap;

    /*Luminous::Priority levelPriority(int level)
    {
      if(level <= DEFAULT_MAP1)
        return Luminous::Task::PRIORITY_NORMAL +
            Luminous::Task::PRIORITY_OFFSET_BIT_HIGHER;

      return Luminous::Task::PRIORITY_NORMAL;
    }*/

    /// writes cache filename for level to given string
    void cacheFileName(std::string & str, int level);

    void recursiveLoad(StackMap & stack, int level);

    std::string m_filename;
    unsigned long int m_fileModified;

    StackMap         m_stackChange;
    Radiant::MutexAuto m_stackMutex;

    std::vector<CPUItem> m_stack;
    Nimble::Vector2i m_nativeSize;
    int              m_maxLevel;

    bool             m_hasAlpha;
    float            m_timeOut;

    // keep the smallest mipmap image (biggest mipmap level m_maxLevel) always ready
    bool             m_keepMaxLevel;
    // what levels should be saved to file
    std::set<int>    m_shouldSave;

    // default save sizes
    enum {
      DEFAULT_SAVE_SIZE1 = 64,
      DEFAULT_SAVE_SIZE2 = 512,
      SMALLEST_IMAGE = 32
    };

    Luminous::ImageInfo m_info;
  };


}

#endif
