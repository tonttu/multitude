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

  class LUMINOUS_API MipMapGenerator : public Task
  {
  public:
    MipMapGenerator(const std::string & src);
    MipMapGenerator(const std::string & src, const PixelFormat & mipmapFormat);

    virtual void doTask();

    void setListener(std::shared_ptr<CPUMipmaps> mipmaps) { m_listener = mipmaps; }

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
