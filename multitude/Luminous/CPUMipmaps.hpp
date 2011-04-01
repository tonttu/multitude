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
      quarter size image, etc.
  */
  /// @todo examples
  class CPUMipmaps : public Luminous::Collectable, public Luminous::Task
  {
  public:

    friend class GPUMipmaps;

    LUMINOUS_API CPUMipmaps();
    LUMINOUS_API virtual ~CPUMipmaps();

    /** Calculates the best-looking mipmap-level for rendering the image with given size.

        @param size The pixel-size to be used for rendering

        @return Returns the index of the mipmap level that would best match
        the actual output pixel resolution.
    */
    LUMINOUS_API int getOptimal(Nimble::Vector2f size);
    /** Get the index of the closest available mipmap-level.

        @param size The on-screen pixel size of the image
        @return Returns the index of the closest available mipmap-level.
    */
    LUMINOUS_API int getClosest(Nimble::Vector2f size);
    /** Gets the mipmap image on level i. If the level does not
        contain a valid mipmap, then 0 is returned.

        @param i The index of the mipmap
        @return Pointer to the image, which may be null.
    */
    LUMINOUS_API std::shared_ptr<ImageTex> getImage(int i);
    /** Mark an image used. This method resets the idle-counter of the
        level, preventing it from being dropped from the memory in the
        near future. Also determines which mipmap level will loaded next.

        @param i The index of the mipmap-level to be marked
    */
    LUMINOUS_API void markImage(size_t i);
    /** Check ifthe mipmaps are ready for rendering.
        @return Returns true if the object has loaded enough mipmaps. */
    /// @todo what does "enought" mean?
    LUMINOUS_API bool isReady();

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
    LUMINOUS_API bool startLoading(const char * filename, bool immediate);

    /** @return Returns the native size of the image, in pixels. */
    const Nimble::Vector2i & nativeSize() const { return m_nativeSize;}

    /// Binds a texture to the given texture unit. Automatically selects appropriate mipmap from given parameters.
    /// @param pixelSize size of the texture on screen in pixel
    /// @param textureUnit OpenGL texture unit
    /// @return true if a mipmap was succesfully bound, false if no texture was bound
    LUMINOUS_API bool bind(Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0);

    /// @copydoc bind(Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0)
    /// @param transform transformation matrix to multiply the pixelSize with to get final screen size
    LUMINOUS_API bool bind(const Nimble::Matrix3 & transform, Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0);

    /// @copydoc bind(Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0)
    /// @param resources OpenGL resource container for this thread
    LUMINOUS_API bool bind(GLResources * resources, Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0);

    /// @copydoc bind(const Nimble::Matrix3 & transform, Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0);
    /// @param resources OpenGL resource container for this thread
    LUMINOUS_API bool bind(GLResources * resources, const Nimble::Matrix3 & transform, Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0);

    /** Check if the mipmaps are still being loaded.

        @return Returns true if the mipmaps are still being loaded.
    */
    LUMINOUS_API bool isActive();
    /** @return Returns the aspect ratio of the image. */
    inline float aspect() const
    { return (float) m_nativeSize.x / (float)m_nativeSize.y; }

    /// @return Returns true if the images have alpha channel
    inline bool hasAlpha() const { return m_hasAlpha; }

    /// Not finished
    LUMINOUS_API int pixelAlpha(Nimble::Vector2 relLoc);

    /// Mark this object as done
    LUMINOUS_API void finish();

    /// Returns the number of images in the stack
    inline unsigned stackSize() const { return (unsigned) m_stack.size(); }

    /// Returns the size of the mipmap level
    LUMINOUS_API Nimble::Vector2i mipmapSize(int level);

    /// Set the time to keep mipmaps in CPU memory
    inline void setTimeOut(float timeout) { m_timeOut = timeout; }

  protected:
    LUMINOUS_API virtual void doTask();

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
        m_lastUsed = Radiant::TimeStamp::getTime();
      }

      float sinceLastUse() const { return m_lastUsed.sinceSecondsD(); }

    private:
      ItemState m_state;
      std::shared_ptr<ImageTex> m_image;
      Radiant::TimeStamp m_lastUsed;
    };

    typedef std::map<int, CPUItem> StackMap;

    CPUItem getStack(int index);

    /// writes cache filename for level to given string
    void cacheFileName(std::string & str, int level);

    void recursiveLoad(StackMap & stack, int level);
    void reschedule(double delay = 0.0, bool allowLater = false);

    void applyStackChanges();

    std::string m_filename;
    unsigned long int m_fileModified;

    StackMap           m_stackChange;
    Radiant::Mutex m_stackMutex;

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
