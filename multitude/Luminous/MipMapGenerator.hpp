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

#ifndef LUMINOUS_MIPMAPGENERATOR_HPP
#define LUMINOUS_MIPMAPGENERATOR_HPP

#include "Export.hpp"
#include "Task.hpp"
#include "PixelFormat.hpp"

#include <Radiant/RefPtr.hpp>

#include <vector>

namespace Luminous {
  class Image;
  class CPUMipmaps;

  /// Task that generates mipmaps to global imagecache for source image.
  /// Will only create DDS/DXT mipmaps. CPUMipmaps uses this class if compressed
  /// mipmaps are requested, there is usually no need to use this class directly.
  class LUMINOUS_API MipMapGenerator : public Task
  {
  public:
    /// Generates a new task for new image. Mipmaps will be saved with one of
    /// the DXT image formats, depending on the source image format.
    /// @param src The filename of the original image, for example a PNG file
    MipMapGenerator(const std::string & src);

    /// Generates a new task for new image with explicit mipmap pixelformat.
    /// @param src The filename of the original image, for example a PNG file
    /// @param mipmapFormat The mipmap output format. Only DXT compressed formats are supported.
    MipMapGenerator(const std::string & src, const PixelFormat & mipmapFormat);

    /// Run the task. Generate the mipmap file and inform the listener when the
    /// task is ready.
    virtual void doTask();

    /// Set a listener to this task. Listener is informed by mipmapsReady()-call
    /// when the mipmaps are ready.
    void setListener(std::shared_ptr<CPUMipmaps> mipmaps) { m_listener = mipmaps; }

    /// Chooses automatically the best pixelformat for source image
    /// @param img The image whose ideal mipmap format we are deducing.
    static PixelFormat chooseMipmapFormat(const Image & img);

  private:
    void resize(const Image & img, const int level);

    const std::string m_src;
    PixelFormat m_mipmapFormat;

    std::vector<unsigned char> m_outBuffer;
    unsigned char * m_out;

    std::shared_ptr<CPUMipmaps> m_listener;

    int m_flags;
  };

}
#endif // LUMINOUS_MIPMAPGENERATOR_HPP
