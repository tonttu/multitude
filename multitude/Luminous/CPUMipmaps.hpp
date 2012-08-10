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
  /// @deprecated use Luminous::Mipmap instead
  class LUMINOUS_API CPUMipmaps : public Luminous::Collectable, public Luminous::Task,
                                  public std::enable_shared_from_this<CPUMipmaps>
  {
    MEMCHECKED_USING(Luminous::Collectable);
  public:
    /// A structure providing information about the current state of a mipmap.
    struct StateInfo : public GLResource
    {
    public:
      /// Constructs a new state info for the given OpenGL resource collection
      /// @param host OpenGL resource collection in which to track the state
      StateInfo(Luminous::RenderContext * host) : GLResource(host), optimal(-1), bound(-1) {}

      /// Returns true if the currently requested level has been loaded and bound
      bool ready() const { return bound >= 0 && optimal == bound; }
      /// Currently requested mipmap level
      int optimal;
      /// Currently bound mipmap level
      int bound;
    };

    MULTI_ATTR_DEPRECATED("CPUMipmaps is deprecated. Use Luminous::Mipmap instead.", CPUMipmaps());
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
#ifndef LUMINOUS_OPENGLES
    /** Gets the compressed image on given level.
        @param i the mipmap level
        @return shared pointer to the mipmap */
    std::shared_ptr<CompressedImageTex> getCompressedImage(int i);
    /** Mark an image used. This method resets the idle-counter of the
        level, preventing it from being dropped from the memory in the
        near future. Also determines which mipmap level will loaded next.

        @param i The index of the mipmap-level to be marked
    */
#endif // LUMINOUS_OPENGLES
    void markImage(size_t i);
    /** Check ifthe mipmaps are ready for rendering.
        @return Returns true if the object has loaded enough mipmaps. */
    /// @todo what does "enought" mean?
    bool isReady();

    /** Starts to load given file, and build the mipmaps.

        This function call may take some time, since it will check that the
        image file exists, and obtain its resolution. If the most fluent
        interaction is required, then you should call this function in another
        thread, for example using the #Luminous::BGThread background tasking
        framework.

        Optionally compressed mipmaps can be created. This will cause the
        CPUMipmaps to generate DXT compressed mipmaps that are stored in DDS
        format. Compressed mipmaps will take longer to generate, but once they
        have been generated they are much faster to load and use. The image
        quality is somewhat decreased when compressed mipmaps are used. This is
        mainly noticable with gradient images.

        @param filename The name of the image file
        @param compressed_mipmaps control if compressed mipmaps should be used
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
    bool bind(RenderContext * resources, Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0);

    /// @copydoc bind(const Nimble::Matrix3 & transform, Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0);
    /// @param resources OpenGL resource container for this thread
    /// @param transform transformation matrix to multiply the pixelSize with to get final screen size
    bool bind(RenderContext * resources, const Nimble::Matrix3 & transform, Nimble::Vector2 pixelSize, GLenum textureUnit = GL_TEXTURE0);

    /// Query the mipmap state in the given OpenGL resource collection. This
    /// function can be used to query the state of the mipmaps in a specific
    /// rendering thread.
    /// @param resources resource collection in which to query the state
    /// @return state of the mipmap collection in the given resource collection
    StateInfo stateInfo(RenderContext * resources);

    /// Sets the loading priority for this set of mipmaps
    /// @param priority new priority
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
    /// @param timeoutCPU timeout to keep mipmaps in CPU memory after last use
    /// @param timeoutGPU timeout to keep mipmaps in GPU memory after last use
    inline void setTimeOut(float timeoutCPU, float timeoutGPU) {
      m_timeOutCPU = timeoutCPU;
      m_timeOutGPU = timeoutGPU;
    }

    /// Returns the original filename of the image.
    /// @return filename from which the mipmaps have been created
    inline QString filename() const { return m_filename; }

    /// Check if the maximum mipmap level, i.e. the smallest mipmap, is kept in memory
    /// @return true if the smallest mipmap is kept in memory
    inline bool keepMaxLevel() const { return m_keepMaxLevel; }
    /// Sets whether the smallest mipmap is kept in memory regardless of use
    /// @param v true to always keep the smallest mipmap in memory
    inline void setKeepMaxLevel(bool v) { m_keepMaxLevel = v; }

    /// Returns true if compressed mipmaps are in use
    /// @return true if compressed mipmaps are used
    inline bool compressedMipmaps() const { return m_compressedMipmaps; }

    /// Returns cache filename for given source file name.
    /// @param src The original image filename
    /// @param level Mipmap level, ignored if negative
    /// @param suffix File format of the cache file name, usually png or dds.
    /// @return cache filename
    static QString cacheFileName(const QString & src, int level = -1,
                                 const QString & suffix = "png");

    /** Gets a shared pointer to an image file CPU-side mipmap.
        @sa Luminous::CPUMipmaps::startLoading

        @param filename The name of the image file

        @param compressed_mipmaps control whether compressed mipmaps should be used

        @return If the file is already open, then a shared pointer is
        returned. Otherwise this will create a new
        #Luminous::CPUMipmaps object, and return a shared pointer to that (if opened successfully).
     */
    static std::shared_ptr<CPUMipmaps> acquire(const QString & filename, bool compressed_mipmaps);

    static bool s_dxtSupported;
  protected:
    virtual void doTask();

  private:
    friend class MipMapGenerator;

    void mipmapsReady(const ImageInfo & info);

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
        LUMINOUS_IN_FULL_OPENGL(m_compressedImage.reset());
        m_lastUsed = 0;
      }

      void dropFromGPU()
      {
        if(m_image) m_image.reset(m_image->move());
        LUMINOUS_IN_FULL_OPENGL(if(m_compressedImage) m_compressedImage.reset(m_compressedImage->move());)
      }

      float sinceLastUse() const { return m_lastUsed.sinceSecondsD(); }

    private:
      ItemState m_state;
      std::shared_ptr<ImageTex> m_image;
#ifndef LUMINOUS_OPENGLES
      std::shared_ptr<CompressedImageTex> m_compressedImage;
#endif // LUMINOUS_OPENGLES
      Radiant::TimeStamp m_lastUsed;
    };

    typedef std::map<int, CPUItem> StackMap;

    CPUItem getStack(int index);

    void recursiveLoad(StackMap & stack, int level);
    void reschedule(double delay = 0.0, bool allowLater = false);

    QString m_filename;
    QString m_compFilename;
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
