/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_MIPMAPGENERATOR_HPP
#define LUMINOUS_MIPMAPGENERATOR_HPP

#ifdef LUMINOUS_OPENGLES
# error "MipMapGenerator cannot be used in OpenGL ES"
#endif

#include "Export.hpp"
#include "PixelFormat.hpp"
#include "Image.hpp"

#include <Radiant/Task.hpp>

#include <memory>

#include <vector>

namespace Luminous {

  /// Task that generates mipmaps to global imagecache for source image.
  /// Will only create DDS/DXT mipmaps. CPUMipmaps uses this class if compressed
  /// mipmaps are requested, there is usually no need to use this class directly.
  class MipMapGenerator : public Radiant::Task
  {
  public:
    /// Generates a new task for new image. Mipmaps will be saved with one of
    /// the DXT image formats, depending on the source image format.
    /// @param src The filename of the original image, for example a PNG file
    /// @param target The filename of the target image
    LUMINOUS_API MipMapGenerator(const QString & src, const QString & target);

    /// Generates a new task for new image with explicit mipmap pixelformat.
    /// @param src The filename of the original image, for example a PNG file
    /// @param target The filename of the target image
    /// @param mipmapFormat The mipmap output format. Only DXT compressed formats are supported.
    LUMINOUS_API MipMapGenerator(const QString & src, const QString & target,
                    const PixelFormat & mipmapFormat);

    LUMINOUS_API virtual ~MipMapGenerator();

    /// Run the task. Generate the mipmap file and inform the listener when the
    /// task is ready.
    LUMINOUS_API virtual void doTask() OVERRIDE;

    /// Set a listener to this task. Listener is called when the mipmaps are ready.
    /// @param func the listener
    LUMINOUS_API void setListener(std::function<void (bool ok, const ImageInfo &)> func);

    /// Chooses automatically the best pixel format for source image
    /// @param img The image whose ideal mipmap format we are deducing.
    /// @return ideal pixel format
    LUMINOUS_API static PixelFormat chooseMipmapFormat(const Image & img);

    LUMINOUS_API static int defaultPriority();

  private:
    void resize(const Image & img, const int level);

    const QString m_src;
    QString m_target;
    PixelFormat m_mipmapFormat;

    std::vector<unsigned char> m_outBuffer;
    unsigned char * m_out;

    std::function<void (bool ok, const ImageInfo &)> m_listener;

    int m_flags;
  };

}
#endif // LUMINOUS_MIPMAPGENERATOR_HPP
