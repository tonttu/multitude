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

#include "Export.hpp"
#include "CPUMipmapStore.hpp"

#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

namespace Luminous {

  using namespace Radiant;

  static Radiant::Mutex s_mutex;

  static std::map<std::pair<std::string, unsigned long>, std::weak_ptr<CPUMipmaps> > s_mipmaps;

  std::shared_ptr<CPUMipmaps> CPUMipmapStore::acquire(const std::string & filename,
                                                      bool compressed_mipmaps)
  {
    Radiant::Guard g( s_mutex);

    // Check the timestamp
    unsigned long lastMod = Radiant::FileUtils::lastModified(filename);
    const std::pair<std::string, unsigned long> key = std::make_pair(filename, lastMod);

    std::weak_ptr<CPUMipmaps> & mipmap_weak = s_mipmaps[key];

    // Check if ptr still points to something valid
    std::shared_ptr<CPUMipmaps> mipmap_shared = mipmap_weak.lock();
    if (mipmap_shared)
      return mipmap_shared;

    mipmap_shared.reset(new CPUMipmaps);

    if(!mipmap_shared->startLoading(filename.c_str(), compressed_mipmaps)) {
      return std::shared_ptr<CPUMipmaps>();
    }

    // store new weak pointer
    mipmap_weak = mipmap_shared;

    debugLuminous("CPUMipmapStore::acquire # Created new for [%s, %ld] (%ld links)", filename.c_str(), lastMod, s_mipmaps[key].use_count());

    return mipmap_shared;
  }

  void CPUMipmapStore::release(std::shared_ptr<CPUMipmaps>)
  {
  }

  std::shared_ptr<CPUMipmaps> CPUMipmapStore::copy(std::shared_ptr<CPUMipmaps> mipmaps)
  {
    return std::shared_ptr<CPUMipmaps>(mipmaps);
  }

}
