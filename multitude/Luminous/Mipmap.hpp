/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_MIPMAP_HPP
#define LUMINOUS_MIPMAP_HPP

#include "Luminous.hpp"

#include "Radiant/Task.hpp"

#include <Nimble/Vector2.hpp>
#include <Nimble/Matrix4.hpp>

#include <Valuable/Node.hpp>
#include <Valuable/State.hpp>

#include <QString>
#include <QFuture>

namespace Luminous
{

  /// This class provides a custom mipmap management for images loaded from
  /// disk.
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

    LUMINOUS_API Image * image(unsigned int level = 0, unsigned int * returnedLevel = nullptr,
                               int priorityChange = 0);

    /// Gets the best available texture with asked size, shorthand function
    /// for level and other texture-function.
    /// @param maxSize Maximum texture size, usually the same as RenderContext::maxTextureSize
    LUMINOUS_API Texture * texture(const Nimble::Matrix4 & transform, Nimble::SizeF pixelSize, unsigned int maxSize);

    LUMINOUS_API bool isLevelAvailable(unsigned int level) const;

#ifndef LUMINOUS_OPENGLES
    /** Gets the compressed image on given level.
        @param level the mipmap level
        @return pointer to the mipmap */
    LUMINOUS_API CompressedImage * compressedImage(unsigned int level = 0, unsigned int * returnedLevel = nullptr,
                                                   int priorityChange = 0);
#endif

    /// Calculate the ideal mipmap level
    /// @param maxSize Maximum image or texture size, usually the same as RenderContext::maxTextureSize
    LUMINOUS_API unsigned int level(const Nimble::Matrix4 & transform, Nimble::SizeF pixelSize,
                                    unsigned int maxSize, float * trilinearBlending = nullptr) const;
    LUMINOUS_API unsigned int level(Nimble::SizeF pixelSize,
                                    float * trilinearBlending = nullptr) const;

    /// Maximum level defined for this image (the smallest mipmap image)
    /// Returns valid value only if isHeaderReady() returns true
    LUMINOUS_API unsigned int maxLevel() const;

    /** @return Returns the native size of the image, in pixels. */
    LUMINOUS_API const Nimble::Size & nativeSize() const;
    /// Returns the aspect ratio of the image in its native size (width/height)
    /** If the native height is zero (e.g. no file was loaded) this returns 1.*/
    LUMINOUS_API float aspect() const;

    /// Mipmap is not "header-ready", if it still has PingTask running/waiting
    /// After the mipmap is header-ready, nativeSize() returns the correct size
    LUMINOUS_API bool isHeaderReady() const;

    /// Mipmap is not ready, if it still has PingTask or MipmapGeneratorTask running/waiting
    LUMINOUS_API bool isReady() const;

    LUMINOUS_API bool isValid() const;

    /// @return Returns true if the images have alpha channel
    LUMINOUS_API bool hasAlpha() const;

    /// Reads alpha value in the most accurate loaded mipmap level in given coordinate
    /// This function doesn't trigger any mipmap loading
    /// @param relLoc relative XY-location, values should be from 0 to 1
    /// @return alpha value from 0 to 1, or 1 if reading fails
    LUMINOUS_API float pixelAlpha(Nimble::Vector2 relLoc) const;

    /// Sets the loading priority for this set of mipmaps
    /// @param priority new priority
    LUMINOUS_API void setLoadingPriority(Radiant::Priority priority);

    /// Expiration time of the mipmap levels after not being used
    /// in tenths of a second.
    LUMINOUS_API int expirationTimeDeciSeconds() const;

    /// Set the expiration time of the mipmap levels
    /// @param deciseconds expiration time in tenths of a second
    LUMINOUS_API void setExpirationTimeDeciSeconds(int deciseconds);

    /// Returns the size of the mipmap level
    LUMINOUS_API Nimble::Size mipmapSize(unsigned int level) const;

    /// Returns the absolute filename of the image.
    /// @return filename from which the mipmaps have been created
    LUMINOUS_API const QString & filename() const;

    LUMINOUS_API Radiant::TaskPtr pingTask();
    LUMINOUS_API Radiant::TaskPtr mipmapGeneratorTask();
    LUMINOUS_API Radiant::TaskPtr loadingTask();

    LUMINOUS_API Valuable::LoadingState & state();
    LUMINOUS_API const Valuable::LoadingState & state() const;

    /// Obsolete mipmap means that the file was changed on disk and reload()
    /// was called. This mipmap shouldn't be used anymore, instead new one
    /// should be generated with acquire().
    LUMINOUS_API bool isObsolete() const;

    /// Gets a shared pointer to an image file CPU-side mipmap.
    /// @param filename The name of the image file
    /// @param compressedMipmaps control whether compressed mipmaps should be used
    LUMINOUS_API static std::shared_ptr<Mipmap> acquire(const QString & filename,
                                                        bool compressedMipmaps);

    /// Sets Mipmap instance that has loaded this file obsolete. Call this if
    /// the file changes and mipmap needs to be reloaded. This won't affect
    /// existing Mipmap instances, since currently it's not thread-safe to
    /// reload existing mipmap, but it makes the mipmap to send "reloaded"
    /// event which tells users to acquire a new Mipmap object.
    /// @param filename File that was changed on disk
    /// @returns true if Mipmap was found and obsoleted
    LUMINOUS_API static bool reload(const QString & filename);

    /// Returns cache filename for given source file name.
    /// @param src The original image filename
    /// @param level Mipmap level, ignored if negative
    /// @param suffix File format of the cache file name, usually png or dds.
    /// @return cache filename
    LUMINOUS_API static QString cacheFileName(const QString & src, int level = -1,
                                              const QString & suffix = "png");

    /// Returns path to dir that contains all cached mipmaps
    LUMINOUS_API static QString imageCachePath();

  private:
    Mipmap(const QString & filenameAbs);

    void setMipmapReady(const ImageInfo & imginfo);
    void startLoading(bool compressedMipmaps);
    void setObsolete();

  private:
    friend class PingTask;
    friend class LoadImageTask;
    friend class MipmapReleaseTask;
    class D;
    D * m_d;
  };

  /// Shared pointer to Mipmap
  typedef std::shared_ptr<Mipmap> MipmapPtr;

}

#endif // LUMINOUS_MIPMAP_HPP
