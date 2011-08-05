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

#include <Radiant/Trace.hpp>

namespace Luminous {

  using namespace Radiant;

  static Radiant::Mutex s_mutex;

  static std::map<QString, std::weak_ptr<CPUMipmaps> > s_mipmaps;


  std::shared_ptr<CPUMipmaps> CPUMipmapStore::acquire(const QString & filename,
                                                      bool compressed_mipmaps)
  {
    Radiant::Guard g( s_mutex);

    std::weak_ptr<CPUMipmaps> & mipmap_weak = s_mipmaps[filename];

    // Check if ptr still points to something valid
    std::shared_ptr<CPUMipmaps> mipmap_shared = mipmap_weak.lock();
    if (mipmap_shared)
      return mipmap_shared;

    mipmap_shared.reset(new CPUMipmaps);

    if(!mipmap_shared->startLoading(filename.toUtf8().data(), compressed_mipmaps)) {
      return std::shared_ptr<CPUMipmaps>();
    }

    if(!compressed_mipmaps)
      Luminous::BGThread::instance()->addTask(mipmap_shared);

    // store new weak pointer
    mipmap_weak = mipmap_shared;

    debugLuminous("CPUMipmapStore::acquire # Created new for %s (%ld links)",
          filename.toUtf8().data(), s_mipmaps[filename].use_count());

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
