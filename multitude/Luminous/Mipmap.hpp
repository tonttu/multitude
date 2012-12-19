#ifndef LUMINOUS_MIPMAP_HPP
#define LUMINOUS_MIPMAP_HPP

#include "Luminous/Luminous.hpp"
#include "Radiant/Task.hpp"

#include <Nimble/Vector2.hpp>
#include <Nimble/Matrix4.hpp>

#include <Valuable/Node.hpp>

#include <QString>
#include <QFuture>

namespace Luminous
{
  class Mipmap : public Valuable::Node,
                 public std::enable_shared_from_this<Mipmap>
  {
  public:
    LUMINOUS_API ~Mipmap();

    /// Gets the texture mipmap.
    /// @param level The mipmap level
    /// @return Pointer to the texture or null if image if mipmap level isn't loaded yet
    LUMINOUS_API Texture * texture(unsigned int level = 0, unsigned int * returnedLevel = nullptr,
                                   int priorityChange = 0);

#if 0
    LUMINOUS_API Image * image(unsigned int level = 0) const;

#ifndef LUMINOUS_OPENGLES
    /** Gets the compressed image on given level.
        @param level the mipmap level
        @return shared pointer to the mipmap */
    LUMINOUS_API CompressedImage * compressedImage(unsigned int level = 0);
#endif // LUMINOUS_OPENGLES

#endif

    /// Calculate the ideal mipmap level
    LUMINOUS_API unsigned int level(const Nimble::Matrix4 & transform, Nimble::Vector2 pixelSize,
                                    float * trilinearBlending = nullptr) const;
    LUMINOUS_API unsigned int level(Nimble::Vector2 pixelSize,
                                    float * trilinearBlending = nullptr) const;

    /** @return Returns the native size of the image, in pixels. */
    LUMINOUS_API const Nimble::Vector2i & nativeSize() const;
    /// Returns the aspect ratio of the image in its native size (width/height)
    /** If the native height is zero (e.g. no file was loaded) this returns 1.*/
    LUMINOUS_API float aspect() const;


    LUMINOUS_API void onHeaderReady(std::function<void(void)> callback, bool once=false, ListenerType type=AFTER_UPDATE);

    LUMINOUS_API bool isHeaderReady() const;

    /// Mipmap is not ready, if it still has PingTask running/waiting
    /// After the mipmap is ready, nativeSize() returns the correct size
    LUMINOUS_API bool isReady(bool f=false) const OVERRIDE;

    LUMINOUS_API bool isValid() const;

    /// Check if the mipmap loading was cancelled.
    /// @return true if loading was cancelled; otherwise false
    LUMINOUS_API bool isLoadCancelled() const;

    /// @return Returns true if the images have alpha channel
    LUMINOUS_API bool hasAlpha() const;

    /// Not finished
    LUMINOUS_API float pixelAlpha(Nimble::Vector2 relLoc);

    /// Sets the loading priority for this set of mipmaps
    /// @param priority new priority
    LUMINOUS_API void setLoadingPriority(Radiant::Priority priority);

    /// Returns the size of the mipmap level
    LUMINOUS_API Nimble::Vector2i mipmapSize(unsigned int level);

    /// Returns the absolute filename of the image.
    /// @return filename from which the mipmaps have been created
    LUMINOUS_API const QString & filename() const;

    /// Gets a shared pointer to an image file CPU-side mipmap.
    /// @param filename The name of the image file
    /// @param compressedMipmaps control whether compressed mipmaps should be used
    LUMINOUS_API static std::shared_ptr<Mipmap> acquire(const QString & filename,
                                                        bool compressedMipmaps);

    /// Returns cache filename for given source file name.
    /// @param src The original image filename
    /// @param level Mipmap level, ignored if negative
    /// @param suffix File format of the cache file name, usually png or dds.
    /// @return cache filename
    LUMINOUS_API static QString cacheFileName(const QString & src, int level = -1,
                                              const QString & suffix = "png");

    void cancelLoading();
  private:
    Mipmap(const QString & filenameAbs);

    void mipmapReady(const ImageInfo & imginfo);
    bool startLoading(bool compressedMipmaps);

  private:
    friend class PingTask;
    friend class LoadImageTask;
    friend class MipmapReleaseTask;
    class D;
    D * m_d;
  };
  typedef std::shared_ptr<Mipmap> MipmapPtr;
}

#endif // LUMINOUS_MIPMAP_HPP
