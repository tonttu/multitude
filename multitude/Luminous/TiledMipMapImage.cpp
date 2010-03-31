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


#include "TiledMipMapImage.hpp"

#include "Utils.hpp"

namespace Luminous
{
  using namespace Nimble;

  bool TiledMipMapImage::Tile::render(float level,
				      Nimble::Rect area,
				      TiledMipMapImage * host)
  {
    if(!m_area.intersects(area))
      return true;
    

    if(level > m_level) {
      // Try to render the target using children:
      for(int i = 0; i < 2; i++) {
	for(int j = 0; j < 2; j++) {
	  bool painted;
	  if(!m_children[i][j].ptr()) {
	    Tile * t = new Tile;
	    t->m_area = m_area.quarter(i, j);
	    t->m_level = m_level + 1;
	    painted = t->render(level, area, host); // Starts image loading
	    m_children[i][j] = t;
	    painted = false;
	  }
	  else {
	    painted = m_children[i][j].ptr()->render(level, area, host);
	  }

	  if(!painted && m_texture.ptr()) {
	    const Rect & a = m_children[i][j].ptr()->m_area;
	    m_texture.ptr()->bind();
	    Rect tc(0, 0, 1, 1); // texture coordinates
	    tc = tc.quarter(i, j);
	    glBegin(GL_QUADS);
	    glTexCoord2f(tc.low().x, tc.low().y);
	    glVertex2f(a.low().x, a.low().y);
	    glEnd();
	  }
	}
      }
    }

    if(m_image.width() == 0) {
      // No image in memory, try to load one:
      if(m_loader) {
	handleLoading(host);
      }
      else {
	startLoading(host);
	return false;
      }
    }

    if(!m_texture.ptr())
      return false;

    m_texture.ptr()->bind();

    Utils::glTexRect(m_area.low(), m_area.high());

    return true;
  }

  void TiledMipMapImage::Tile::startLoading(TiledMipMapImage * host)
  {
    if(m_failed) 
      return;

    char buf[256];
    sprintf(buf, host->file(),
	    (int) m_area.low().x, (int) m_area.low().y, m_level);
    m_loader = new Loader(buf);
  }
  
  void TiledMipMapImage::Tile::handleLoading(TiledMipMapImage *)
  {
    if(m_loader->state() == Loadable::LOADED) {
      m_image = m_loader->m_image;
      m_texture = Texture2D::fromImage(m_image, false);
      m_thread->finishedLoading(m_loader);
      m_loader = 0;
    }
    else if(m_loader->state() == Loadable::FAILURE) {
      m_thread->finishedLoading(m_loader);
      m_loader = 0;
      m_failed = true;
    }
  }

  TiledMipMapImage::TiledMipMapImage()
  {}

  TiledMipMapImage::~TiledMipMapImage()
  {}

  void TiledMipMapImage::init(const char * imgpath,
			      int tilesize, int tilen, int levels)
  {
    (void) tilen;
    m_file = imgpath;
    m_root.m_area.set(0, 0, tilesize, tilesize);
    m_levels = levels;
  }

  void TiledMipMapImage::render(Nimble::Rect area, float level)
  {
    if(level > m_levels)
      level = m_levels;

    glColor3f(1, 1, 1);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    // int desiredLevel = (int) level;
    
    m_root.render(level, area, this);
  }
    
}


