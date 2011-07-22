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

// #define CPUMIPMAPS_PROFILING

#ifdef CPUMIPMAPS_PROFILING
  struct ProfileData;
#endif

namespace Luminous {

  class GLResources;

  /** Collection of image mipmaps in the RAM/disk of the
      computer. This class is used to load and scale images from the
      disk in the background.

      CPUMipmaps are typically used to handle the loading of
      images in the background. They ease the handling of large
      amounts of images, so that neither the CPU or the GPU memory is
      exceeded. The classes work in both single- and multi-threaded
      environments.

      Mipmap level 0 is the original image, level 1 is the
      quarter size image, etc.
  */
  /// @todo examples
  class LUMINOUS_API CPUMipmaps : public Luminous::Collectable, public Luminous::Task,
                                  public std::enable_shared_from_this<CPUMipmaps>
  {
  public:
    struct StateInfo : public GLResource
    {
    public:
      StateInfo(Luminous::GLResources * host) : GLResource(host), optimal(-1), bound(-1) {}
      bool ready() const { return bound >= 0 && optimal == bound; }
      int optimal;
      int bound;
    };

    friend class GPUMipmaps;

    CPUMipmaps();
    virtual ~CPUMipmaps();

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
    std::shared_ptr<CompressedImageTex> getCompressedImage(int i);
    /** Mark an image used. This method resets the idle-counter of the
        level, preventing it from being dropped from the memory in the
        near future. Also determines which mipmap level will loaded next.

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
        @return True if the image file could be opened successfully.
    */
    bool startLoading(const char * filename, bool compressed_mipmaps);

    /** @return Returns the native size of the image, in pixels. */
    const Nimble::Vector2i & nativeSize() const { return m_nativeSize;}

    /// Binds a texture to the given texture unit. Automatically selects appropriate mipmap from given parameters.
    /// @param pixelSize size of the texture on screen in pixel
    /// @param textureUnit OpenGL texture unit
    /// @return true if a mipmap was succesfully bound, false if no texture was bound
    bool bind(Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0);

    /// @copydoc bind(Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0)
    /// @param transform transformation matrix to multiply the pixelSize with to get final screen size
    bool bind(const Nimble::Matrix3 & transform, Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0);

    /// @copydoc bind(Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0)
    /// @param resources OpenGL resource container for this thread
    bool bind(GLResources * resources, Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0);

    /// @copydoc bind(const Nimble::Matrix3 & transform, Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0);
    /// @param resources OpenGL resource container for this thread
    /// @param transform transformation matrix to multiply the pixelSize with to get final screen size
    bool bind(GLResources * resources, const Nimble::Matrix3 & transform, Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0);

    StateInfo stateInfo(GLResources * resources);

    void setLoadingPriority(Priority priority) { m_loadingPriority = priority; }

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

    /// Set the time to keep mipmaps in CPU and GPU memory
    /// The GPU timeout can be a more like a recommendation than a true limit
    /// @todo Currently m_timeOutCPU can't be smaller than m_timeOutGPU, fix this
    inline void setTimeOut(float timeoutCPU, float timeoutGPU) {
      m_timeOutCPU = timeoutCPU;
      m_timeOutGPU = timeoutGPU;
    }

    inline std::string filename() const { return m_filename; }

    inline bool keepMaxLevel() const { return m_keepMaxLevel; }
    inline void setKeepMaxLevel(bool v) { m_keepMaxLevel = v; }

    void mipmapsReady(const ImageInfo & info);

    inline bool compressedMipmaps() const { return m_compressedMipmaps; }

    /// Returns cache filename for given source file name.
    /// @param src The original image filename
    /// @param level Mipmap level, ignored if negative
    /// @param suffix File format of the cache file name, usually png or dds.
    static std::string cacheFileName(const std::string & src, int level = -1,
                                     const std::string & suffix = "png");

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
        m_compressedImage.reset();
        m_lastUsed = 0;
      }

      void dropFromGPU()
      {
        if(m_image) m_image.reset(m_image->move());
        if(m_compressedImage) m_compressedImage.reset(m_compressedImage->move());
      }

      float sinceLastUse() const { return m_lastUsed.sinceSecondsD(); }

    private:
      ItemState m_state;
      std::shared_ptr<ImageTex> m_image;
      std::shared_ptr<CompressedImageTex> m_compressedImage;
      Radiant::TimeStamp m_lastUsed;
    };

    typedef std::map<int, CPUItem> StackMap;

    CPUItem getStack(int index);

    void recursiveLoad(StackMap & stack, int level);
    void reschedule(double delay = 0.0, bool allowLater = false);

    std::string m_filename;
    std::string m_compFilename;
    unsigned long int m_fileModified;

    Radiant::Mutex m_stackMutex;

    std::vector<CPUItem> m_stack;
    Nimble::Vector2i m_nativeSize;
    Nimble::Vector2i m_firstLevelSize;
    int              m_maxLevel;

    bool             m_hasAlpha;
    float            m_timeOutCPU;
    float            m_timeOutGPU;

    // keep the smallest mipmap image (biggest mipmap level m_maxLevel) always ready
    bool             m_keepMaxLevel;
    // what levels should be saved to file
    std::set<int>    m_shouldSave;

    // Priority using when loading mipmaps
    Priority         m_loadingPriority;

    // default save sizes
    enum {
      DEFAULT_SAVE_SIZE1 = 64,
      DEFAULT_SAVE_SIZE2 = 512,
      SMALLEST_IMAGE = 32
    };

    // updated on every bind()
    ContextVariableT<StateInfo> m_stateInfo;

    Luminous::ImageInfo m_info;

    bool m_compressedMipmaps;

#ifdef CPUMIPMAPS_PROFILING
    ProfileData & m_profile;
#endif
  };

}

#endif
