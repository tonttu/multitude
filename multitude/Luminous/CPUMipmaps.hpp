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
  class LUMINOUS_API CPUMipmaps : public Luminous::Collectable, public Luminous::Task
  {
  public:
    friend class GPUMipmaps;

    CPUMipmaps();
    virtual ~CPUMipmaps();

    /** Drop old CPU mipmaps from memory.

    @param dt delta-time

    @param purgeTime The time-limit for keeping CPUMipmaps in
    memory. If purgeTime is less than zero, the mipmap idle times
    are updated, but they are <B>not</B> deleted from memory.
     */
    void update(float dt, float purgeTime);

    /** Calculates the best-looking mipmap-level for rendering the image with given size.

        @param size The pixel-size to be used for rendering

        @return Returns the index of the mipmap level that would best match
        the actual output pixel resolution.
    */
    int getOptimal(Nimble::Vector2f size);
    /** Get the index of the closest available mipmap-level.

        @param size The on-screen pixel size of the image
        @return Returns the index of the closest available mipmap-level.
    */
    int getClosest(Nimble::Vector2f size);
    /** Gets the mipmap image on level i. If the level does not
        contain a valid mipmap, then 0 is returned.

        @param i The index of the mipmap
        @return Pointer to the image, which may be null.
    */
    std::shared_ptr<ImageTex> getImage(int i);
    /** Mark an image used. This method resets the idle-counter of the
        level, preventing it from being dropped from the memory in the
        near future.

        @param i The index of the mipmap-level to be marked
    */
    void markImage(size_t i);
    /** Check ifthe mipmaps are ready for rendering.
        @return Returns true if the object has loaded enough mipmaps. */
    /// @todo what does "enought" mean?
    bool isReady();

    /** Starts to load given file, and build the mipmaps.

        This function call may take some time, since it will check that the image file exists,
        and obtain its resolution. If the most fluent interaction is required, then
        you should call this function in another thread, for example using the #Luminous::BGThread
        background tasking framework.

        @param filename The name of the image file
        @param immediate True if the files should be loaded immediately (as
        opposed to loading them later, as needed).
        @return True if the image file could be opened successfully.
    */
    bool startLoading(const char * filename, bool immediate);

    /** @return Returns the native size of the image, in pixels. */
    const Nimble::Vector2i & nativeSize() const { return m_nativeSize;}

    /** Fetch corresponding GPU mipmaps from a resource set. If the
        GPUMipmaps object does not exist yet, it is created and
        returned.

        @return The GPUMipmaps that correspond to these CPUMipmaps
    */
    GPUMipmaps * getGPUMipmaps();
    /// @copydoc getGPUMipmaps
    /// @param rs A pointer to the OpenGL resource container
    GPUMipmaps * getGPUMipmaps(GLResources * rs);
    /// Binds a texture that matches the given size parameters.
    bool bind(GLResources *,
                           const Nimble::Matrix3 & transform,
                           Nimble::Vector2 pixelsize);

    /** Check if the mipmaps are still being loaded.

        @return Returns true if the mipmaps are still being loaded.
    */
    bool isActive();
    /** @return Returns the aspect ratio of the image. */
    inline float aspect() const
    { return (float) m_nativeSize.x / (float)m_nativeSize.y; }

    /// @return Returns true if the images have alpha channel
    inline bool hasAlpha() const { return m_hasAlpha; }

    /// Not finished
    int pixelAlpha(Nimble::Vector2 relLoc);

    /// Mark this object as done
    void finish();

    /// Returns the number of images in the stack
    inline unsigned stackSize() const { return (unsigned) m_stack.size(); }

    /// Returns the size of the mipmap level
    Nimble::Vector2i mipmapSize(int level);

    /// Set the time to keep mipmaps in CPU memory
    inline void setTimeOut(float timeout) { m_timeOut = timeout; }

  protected:
    virtual void doTask();

  private:
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

    CPUItem getStack(int index);

    /// writes cache filename for level to given string
    void cacheFileName(std::string & str, int level);

    void recursiveLoad(StackMap & stack, int level);
    void reschedule(double delay = 0.0, bool allowLater = false);

    std::string m_filename;
    unsigned long int m_fileModified;

    StackMap         m_stackChange;
    Radiant::MutexAuto m_stackMutex;
    Radiant::MutexAuto m_stackChangeMutex;
    Radiant::MutexAuto m_rescheduleMutex;

    std::vector<CPUItem> m_stack;
    Nimble::Vector2i m_nativeSize;
    Nimble::Vector2i m_firstLevelSize;
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
