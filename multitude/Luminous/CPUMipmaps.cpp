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

#include "CPUMipmaps.hpp"

#include "GPUMipmaps.hpp"

#include <Luminous/GLResources.hpp>

#include <Radiant/Directory.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

#include <cassert>
#include <fstream>

namespace Luminous {

  using namespace Radiant;

  CPUMipmaps::Loader::Loader(Luminous::Priority prio,
			     CPUMipmaps * master, 
			     CPUItem * dest, const std::string file)
    : Task(prio),
      m_master(master),
      m_dest(dest),
      m_file(file)
  {
    assert(dest);
    // info("CPUMipmaps::Loader::Loader # %s", file.c_str());
  }

  CPUMipmaps::Loader::~Loader()
  {}

  void CPUMipmaps::Loader::doTask()
  {
    m_state = RUNNING;

    if(!m_dest) {
      m_state = DONE;
      return;
    }

    Image * image = new Image();

    bool ok = image->read(m_file.c_str());
    
    /* trace("CPUMipmaps::Loader::doTask # Loaded %s : %d",
          m_file.c_str(), (int) ok);
    */
    /* Now we may need to skip some pixels so that dimensions are
       multiples of 4. Why NVidia, why??? */
    
    while(image->height() & 0x3)
      image->forgetLastLine();

    int xrem = image->width() & 0x3;

    if(xrem) {
      image->forgetLastPixels(xrem);
    }

    Guard g(generalMutex());

    m_master->m_hasAlpha = image->hasAlpha();

    if(m_dest) {
      assert(m_dest->m_loader == this);
      assert(m_dest->m_state != FINISHED);

      if(!ok) {
        error("CPUMipmaps::Loader::doTask # Loading failed for %s",
	      m_file.c_str());
        m_dest->m_state = FAILED;
	delete image;
	m_master->m_ok = false;
      }
      else {
        m_dest->m_image = image;
        m_dest->m_state = FINISHED;
      }
      
      m_dest->m_loader = 0;
    }
    else {
      // trace("Item was deleted");
      delete image;
    }
      
    m_state = DONE;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  CPUMipmaps::Scaler::Scaler(Luminous::Priority prio,
			     CPUMipmaps * master,
			     CPUItem * dest, CPUItem * source, int level) 
    : Task(prio),
      m_master(master),
      m_source(source),
      m_dest(dest), 
      m_level(level),
      m_quartering(true)
  {
    assert(dest && source);
  }

  CPUMipmaps::Scaler::~Scaler()
  {}

  void CPUMipmaps::Scaler::doTask()
  {
    m_state = RUNNING;

    Image * dimage = new Image(); // Destination
    
    RefPtr<Image> dref(dimage);
    RefPtr<Image> sref;

    {
      Guard g(generalMutex());

      if(!m_dest || !m_source) {
        m_state = DONE;
        return;
      }

      if(m_source->m_state == FAILED) {
        error("CPUMipmaps::Scaler::doTask # Source has failed , aborting");
        m_dest->m_state = FAILED;
        m_source->m_scalerOut = 0;
        m_dest->m_scaler = 0;
        m_state = DONE;
        return;
      }

      if(m_source->m_state != FINISHED) {
        scheduleFromNowSecs(0.05);
        return;
      }

      if(!m_source->m_image.ptr()) {
        error("CPUMipmaps::Scaler::doTask # Source has no image!");
        m_state = DONE;
        return;
      }
      
      /* Make a link copy so that we do not need to lock the mutex for
         a longer time. */
      
      sref = m_source->m_image;
    }

    Image * simage = sref.ptr();

    // trace("CPUMipmaps::Scaler::doTask # creating level %d", m_level);
    
    if(m_quartering) {
      dimage->quarterSize(*simage);
    }
    else {
      int w, h, maxdim = 1 << m_level;

      float aspect = simage->width() / (float) simage->height();

      if(aspect >= 1.0f) {
        w = maxdim;
        h = int(maxdim / aspect);

        while(h & 0x3F)
          h++;
      }
      else {
        h = maxdim;
        w = int(maxdim * aspect);

        while(w & 0x3F)
          w++;
      }

      dimage->copyResample(*simage, w, h);
    }
    
    // trace("CPUMipmaps::Scaler::doTask # created level %d", m_level);

    if(m_file.size()) {

      if(!Directory::mkdir(FileUtils::path(m_file))) {
        /* This is just debug. The mkdir may fail if the directory
           already exist, which is not really a problem. */
	debug("Could not create directory %s", FileUtils::path(m_file).c_str());
      }

      bool ok;

      if(dimage->pixelFormat() == PixelFormat::rgbaUByte()) {
        // if(FileUtils::suffix(m_file) == "PNG")
        //  m_file = FileUtils::baseFileName(m_file) + ".png";
        ok = dimage->write(m_file.c_str());
      } else
        ok = dimage->write(m_file.c_str());

      if(ok)
        debug("CPUMipmaps::Scaler::doTask # Saved mipmap %s", m_file.c_str());
      else
        error("CPUMipmaps::Scaler::doTask # Failed saving %s", m_file.c_str());
    }

    Guard g(generalMutex());

    if(m_dest) {
      assert(m_dest->m_scaler == this);

      m_dest->m_scaler = 0;
      m_dest->m_image = dref;
      m_dest->m_state = FINISHED;
    }

    if(m_file.size())
      m_master->m_fileMask = m_master->m_fileMask | (1 << m_level);

    // Break the links explicitly, while protected by the guard.
    dref.breakLink();
    sref.breakLink();

    m_state = DONE;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  // volatile static int __cpcount = 0;
  
  CPUMipmaps::CPUItem::CPUItem()
    : m_state(WAITING),
      m_scalerOut(0),
      m_scaler(0),
      m_loader(0),
      m_image(0),
      m_unUsed(0)
  {
    //trace("CPUMipmaps::CPUItem::CPUqItem # %d", ++__cpcount);
  }

  CPUMipmaps::CPUItem::~CPUItem()
  {
    //trace("CPUMipmaps::CPUItem::~CPUItem # %d", --__cpcount);

    Guard g(BGThread::instance()->generalMutex());

    if(m_scaler) {
      m_scaler->m_dest = 0;
    }

    if(m_scalerOut) {
      m_scalerOut->m_source = 0;
    }

    if(m_loader) {
      m_loader->m_dest = 0;
    }

    m_image.breakLink();
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  
  CPUMipmaps::CPUMipmaps()
    : m_nativeSize(100, 100),
      m_maxLevel(0),
      m_fileMask(0),
      m_hasAlpha(false),
      m_ok(true)
  {}

  CPUMipmaps::~CPUMipmaps()
  {
    
  }
  
  void CPUMipmaps::update(float dt, float purgeTime)
  {
    for(int i = DEFAULT_MAP1; i <= m_maxLevel; i++) {
      CPUItem * item = m_stack[i].ptr();

      if(!item)
        continue;
      item->m_unUsed += dt;

      if(item->m_unUsed > purgeTime && purgeTime >= 0) {
	//&& item->m_state == FINISHED && 
	// trace("CPUMipmaps:: # Dropping level %d from CPU", i);
        m_stack[i].clear();
      }
    }
  }

  int CPUMipmaps::getOptimal(Nimble::Vector2f size)
  {
    float bigdim = Nimble::Math::Min(size.maximum(),
				     (float) m_nativeSize.maximum());

    int bestlevel = Nimble::Math::Round(log(bigdim) / log(2.0) + 0.0f);
    
    if(bestlevel > m_maxLevel)
      bestlevel = m_maxLevel;
    else if(bestlevel < lowestLevel())
      bestlevel = lowestLevel();
      
    //trace("CPUMipmaps::getOptimal # %dx%d -> %d",
    // (int) size.x, (int) size.y, bestlevel);

    return bestlevel;
  }

  int CPUMipmaps::getClosest(Nimble::Vector2f size)
  {
    int bestlevel = getOptimal(size);
    
    // trace("CPUMipmaps::getClosest # Best would be %d", bestlevel);

    CPUItem * item = m_stack[bestlevel].ptr();

    if(item && item->m_state == FINISHED) {
      /* This is the happy case when the desired level is readily
	 available. */
      return bestlevel;
    }
    if(!item || item->needsLoader()) {
      createLevelScalers(bestlevel);      
    }

    // Scan for the best available mip-map.

    for(int i = 1; i <= m_maxLevel; i++) {
      
      int high = bestlevel + i;

      if(high <= m_maxLevel) {

        item = m_stack[high].ptr();
        
        if(item && item->m_state == FINISHED)
          return high;

        /*if(item)
          trace("Not %d %d %p %p", high, (int) item->m_state,
                item->m_scaler, item->m_loader);
	*/
      }

      int low = bestlevel - i;

      if(low >= 0) {

        item = m_stack[low].ptr();
        
        if(item && item->m_state == FINISHED)
          return low;

	/*
        if(item)
          trace("Neither %d %d", low, (int) item->m_state);
	*/
      }
      
    }

    return -1;
  }

  Image * CPUMipmaps::getImage(int i)
  {
    CPUItem * item = m_stack[i].ptr();

    if(!item)
      return 0;

    item->m_unUsed = 0.0f;

    return item->m_image.ptr();
  }

  void CPUMipmaps::markImage(int i)
  {
    CPUItem * item = m_stack[i].ptr();

    if(item)
      item->m_unUsed = 0.0f;
  }

  bool CPUMipmaps::isReady()
  {
    // return true;
    
    float dt = Radiant::TimeStamp
      (Radiant::TimeStamp::getTime() - m_startedLoading).secondsD();

    if(dt > 3.0f)
      return true;
    
    for(int i = lowestLevel(); i < m_maxLevel; i++) {
      CPUItem * ci = m_stack[i].ptr();
      if(!ci)
	return false;

      if(ci->working())
	return false;
    }
    
    return true;
  }

  bool CPUMipmaps::startLoading(const char * filename, bool immediate)
  {
    debug("CPUMipmaps::startLoading # %s, %d", filename, immediate);
    m_startedLoading = Radiant::TimeStamp::getTime();

    m_filename = filename;

    for(int i = 0; i < MAX_MAPS; i++) {
      m_stack[i].clear();
    }

    m_info = Luminous::ImageInfo();

    if(!Luminous::Image::ping(filename, m_info)) {
      error("CPUMipmaps::startLoading # failed to query image size for %s",
	    filename);
      return false;
    }

    m_nativeSize.make(m_info.width, m_info.height);

    if(!m_nativeSize.minimum())
      return false;

    // Now calculate the image sizes:

    // float as = aspect();
    unsigned maxdim = m_nativeSize.maximum();

    unsigned i, d;

    for(i = 0; i < MAX_MAPS; i++) {
      d = 1 << i;
      
      if(d >= maxdim)
        break;
    }
    
    m_maxLevel = i < MAX_MAPS ? i : MAX_MAPS - 1;

    for(i = lowestLevel(); i <= (unsigned) m_maxLevel; i++) {
      if(savebleMipmap(i)) {
	
	std::string cachefile;
	cacheFileName(cachefile, i);
	
	if(FileUtils::fileReadable(cachefile.c_str())) {
	  m_fileMask = m_fileMask | (1 << i);
	}
      }
    }

    if(immediate) {
      // Now start loading the base image and create the mipmaps.
      for(i = lowestLevel(); i <= (unsigned) m_maxLevel; i++) 
	createLevelScalers(i);
    }
    return true;
  }
  
  GPUMipmaps * CPUMipmaps::getGPUMipmaps(GLResources * resources)
  {
    GLResource * r = resources->getResource(this);

    if(!r) {
      GPUMipmaps * gm = new GPUMipmaps(this, resources);
      resources->addResource(this, gm);
      return gm;
    }

    GPUMipmaps * gm = dynamic_cast<GPUMipmaps *> (r);
    
    assert(gm != 0);

    return gm;
  }
  
  GPUMipmaps * CPUMipmaps::getGPUMipmaps()
  {
    return getGPUMipmaps(GLResources::getThreadResources());
  }

  bool CPUMipmaps::bind(GLResources * r,
			const Nimble::Matrix3 & transform, 
			Nimble::Vector2 pixelsize)
  {
    if(!this)
      return false;

    GPUMipmaps * gpumaps = getGPUMipmaps(r);

    gpumaps->bind(transform, pixelsize);

    return true;
  }

  bool CPUMipmaps::isActive()
  {
    for(int i = 0; i <= m_maxLevel; i++) {
      CPUItem * item = m_stack[i].ptr();
      
      if(item) {
	if(item->working())
	  return true;
      }
    }

    return false;
  }
    
  int CPUMipmaps::pixelAlpha(Nimble::Vector2 relLoc)
  {
    for(int i = MAX_MAPS - 1; i >= 1; i--) {
      Image * im = getImage(i);

      if(!im) continue;
      
      Nimble::Vector2f locf(im->size());
      locf.scale(relLoc);
      
      Vector2i loci(locf);

      loci.x = Nimble::Math::Clamp(loci.x, 0, im->width() - 1);
      loci.y = Nimble::Math::Clamp(loci.y, 0, im->height() - 1);
      
      if(im->pixelFormat() == PixelFormat::rgbaUByte()) {
	const uint8_t * pixels = im->data();
	return pixels[(loci.x + loci.y * im->width()) * 4 + 3];
      }
      else if(im->pixelFormat() == PixelFormat::alphaUByte()) {
	const uint8_t * pixels = im->data();
	return pixels[loci.x + loci.y * im->width() + 3];
      }
      else {
	error("CPUMipmaps::pixelAlpha # Unsupported pixel format");
	return 255;
      }
    }

    info("No mipmaps for alpha calculus");
    
    return 255;
  }
 

  void CPUMipmaps::createLevelScalers(int level)
  {
    // trace("CPUMipmaps::createLevelScalers # %d", level);
    if(!m_ok)
      return;

    CPUItem * item = m_stack[level].ptr();

    if(item && !m_stack[level].ptr()->needsLoader())
      return;
    
    // Try to start loading/conversion:
    
    if(!item) {
      item = new CPUItem;
      m_stack[level] = item;
    }

    // Check if higher-level mipmaps would be available:

    int higher = -1;

    // Check where to get the image from
    for(int i = level + 1; i < m_maxLevel; i++) {
      CPUItem * ci = m_stack[i].ptr();
      if(ci && (ci->m_state == FINISHED || ci->m_scaler || ci->m_loader)) {
	higher = i;
	break;
      }
      if(!ci)
        m_stack[i] = new CPUItem();
    }
    
    if(higher < 0) {

      // See if we should get a lower-level bitmap quickly for preview
      bool gotlower = false;
      for(int i = level; i >= DEFAULT_MAP1; i--) {
        // CPUItem * ci = m_stack[i].ptr();
        if(!needsLoader(i)) {
          gotlower = true;
          break;
        }
      }

      if(!gotlower && (m_fileMask & (1 << DEFAULT_MAP1)) &&
         needsLoader(DEFAULT_MAP1)) {

        // Now load the lowest mip-map from file, with higher priority.
        
        std::string cachefile;
        cacheFileName(cachefile, DEFAULT_MAP1);

        Guard g(bgt()->generalMutex());

        if(!m_stack[DEFAULT_MAP1].ptr())
          m_stack[DEFAULT_MAP1] = new CPUItem();
        
        CPUItem * ci = m_stack[DEFAULT_MAP1].ptr();

        Loader * load = new Loader(levelPriority(DEFAULT_MAP1),
                                   this, ci, cachefile);
        ci->m_loader = load;
        bgt()->addTask(load);

        // trace("Added loader thumb # %p %d", ci, DEFAULT_MAP1);
      }

      /* There are no higher-level mipmaps available, lets try to load
	 one. We assume that the mipmap exists. */
      for(int i = level; i <= m_maxLevel; i++) {
	if((savebleMipmap(i) || (i == m_maxLevel)) && needsLoader(i)) {

	  std::string cachefile;
	  cacheFileName(cachefile, i);

          // if(FileUtils::fileReadable(cachefile.c_str())) {
          if(m_fileMask & (1 << i)) {
          
            // trace("CPUMipmaps::createLevelScalers # Loading cache %d", i);
            
            std::string cachefile;
            cacheFileName(cachefile, i);

	    Guard g(bgt()->generalMutex());
            
            Loader * load = new Loader(levelPriority(i),
				       this, m_stack[i].ptr(), cachefile);
            m_stack[i].ptr()->m_loader = load;
            higher = i;
            bgt()->addTask(load);

            // trace("Added loader 2 # %p %d", m_stack[i].ptr(), i);
            
            if(i == level)
              return;
            
            break;
          }
        }
      }


      if(higher < 0 && needsLoader(m_maxLevel)) {
        // Need to load the original:
	
        // trace("CPUMipmaps::createLevelScalers # Loading original");

	Guard g(bgt()->generalMutex());

        if(!m_stack[m_maxLevel].ptr())
          m_stack[m_maxLevel] = new CPUItem();

	CPUItem * ci = m_stack[m_maxLevel].ptr();

	if(ci->needsLoader()) {
	  
	  Loader * load =
            new Loader(levelPriority(m_maxLevel), this, ci, m_filename);

	  ci->m_loader = load;
	  
	  higher = m_maxLevel;
	  bgt()->addTask(load);

          // trace("Added loader 3 %p %d", m_stack[m_maxLevel].ptr(), m_maxLevel);
	}
      }
    }

    if(higher >= 0) {
      /* Now we can downsample from the higher-level mipmap. */
      for(int i = higher - 1; i >= level; i--) {

        /*trace("CPUMipmaps::createLevelScalers # Scaling %d %s",
	      i, m_filename.c_str());
	*/
	int higher = i + 1;

	Guard g(bgt()->generalMutex());

	if(m_stack[i].ptr()->m_scaler)
	  continue;

	Scaler * s = new Scaler(levelPriority(i), this,
				m_stack[i].ptr(), m_stack[higher].ptr(), i);
	m_stack[higher].ptr()->m_scalerOut = s;
	m_stack[i].ptr()->m_scaler = s;
	m_stack[i].ptr()->m_state = WORKING;

	uint32_t mask = 1 << i;

        if(savebleMipmap(i) && ((m_fileMask & mask) == 0)) {
	  cacheFileName(s->m_file, i);
        }

        if(higher == m_maxLevel)
          s->m_quartering = false;

        bgt()->addTask(s);
      }
    }
  }

  void CPUMipmaps::cacheFileName(std::string & name, int level)
  {
    char buf[32];

    name = Radiant::FileUtils::path(m_filename);

    if(name.empty())
      sprintf(buf, ".imagecache/%.2d_", level);
    else
      sprintf(buf, "/.imagecache/%.2d_", level);

    name += buf;
    name += Radiant::FileUtils::filename(m_filename);
    
    // Put in the right suffix

    unsigned i = name.size() - 1;

    while(i && name[i] != '.' && name[i] != '/')
      i--;

    name.erase(i + 1);

    if(m_info.pf.layout() == PixelFormat::LAYOUT_RGB ||
       m_info.pf.layout() == PixelFormat::LAYOUT_LUMINANCE)
      name += "jpg";
    else // if(m_info.pf.layout() == PixelFormat::LAYOUT_RGBA)
      name += "png";
  }

}
