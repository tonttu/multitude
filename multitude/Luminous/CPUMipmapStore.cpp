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

  /* Note that the mipmaps are not deleted upon application exit. This
     is done on purpose: As we are dealing with static data one easily
     gets conflicts with the order of deleting resources. As a result
     it is safest to not delete the resources. Anyhow, the operating
     system should be able to clear the resources as the application
     shuts down. */

  class LUMINOUS_API MipmapItem
  {
  public:
    MipmapItem() : m_linkCount(0), m_mipmaps(0) {}
    /* Do NOT delete m_mipmaps here, we might at the shutdown sequence
       of the application, and deleting the mipmap here may make it
       reference some resources that are not available any more. */
    ~MipmapItem() { /* delete m_mipmaps; */ }

    void incrCount() { m_linkCount++; }
    void decrCount()
    {
      m_linkCount--;
      if(m_linkCount == 0) {
        m_mipmaps->finish();
        m_mipmaps = 0;
      }
    }

    int m_linkCount;
    CPUMipmaps * m_mipmaps;
  };

  static std::map<std::string, MipmapItem> s_mipmaps;
  typedef std::map<std::string, MipmapItem> MipMapItemContainer;

  CPUMipmaps * CPUMipmapStore::acquire(const std::string & filename, bool immediate)
  {
    Radiant::Guard g( s_mutex);

    MipMapItemContainer::iterator it = s_mipmaps.find(filename);

    if(it !=  s_mipmaps.end()) {
      MipmapItem & mmi = (*it).second;
      mmi.incrCount();
      return mmi.m_mipmaps;
    }

    CPUMipmaps * mipmaps = new CPUMipmaps();

    if(!mipmaps->startLoading(filename.c_str(), immediate)) {
      delete mipmaps;
      return 0;
    }

    Luminous::BGThread::instance()->addTask(mipmaps);

    s_mipmaps[filename].m_mipmaps = mipmaps;
    s_mipmaps[filename].incrCount();

    debugLuminous("CPUMipmapStore::acquire # Created new for %s (%d links)",
          filename.c_str(), s_mipmaps[filename].m_linkCount);

    return mipmaps;
  }

  void CPUMipmapStore::release(CPUMipmaps * mipmaps)
  {
    if(!mipmaps)
      return;

    Radiant::Guard g( s_mutex);

    for(MipMapItemContainer::iterator it = s_mipmaps.begin();
    it != s_mipmaps.end(); it++) {
      MipmapItem & mmi = (*it).second;
      if(mmi.m_mipmaps == mipmaps) {
        mmi.decrCount();
        if(!mmi.m_linkCount) {
          // info("Erased mipmaps %p", mipmaps);
          s_mipmaps.erase(it);
        }
        return;
      }
    }
  }

  CPUMipmaps * CPUMipmapStore::copy(CPUMipmaps * mipmaps)
  {
    if(!mipmaps)
      return 0;

    Radiant::Guard g( s_mutex);

    for(MipMapItemContainer::iterator it = s_mipmaps.begin();
    it != s_mipmaps.end(); it++) {
      MipmapItem & mmi = (*it).second;
      if(mmi.m_mipmaps == mipmaps) {
        mmi.incrCount();
        return mipmaps;
      }
    }
    return 0;
  }

  unsigned CPUMipmapStore::count()
  {
    Radiant::Guard g( s_mutex);
    return (unsigned) s_mipmaps.size();
  }

}
