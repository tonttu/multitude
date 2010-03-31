/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RADIANT_GRID_TMPL_HPP
#define RADIANT_GRID_TMPL_HPP

#include "Grid.hpp"

namespace Radiant {

  template <class T>
  GridMemT<T>::GridMemT(unsigned w, unsigned h)
    : m_width(w), m_height(h)
  {
    unsigned s = w * h;
    if(s)
      m_data = new T[s];
    else
      m_data = 0;
  }

  template <class T>
  GridMemT<T>::~GridMemT()
  {
    delete [] m_data;
  }

  template <class T>
  void GridMemT<T>::resize(unsigned w, unsigned h)
  {
    unsigned s = w * h;
    unsigned smy = m_width * m_height;
    
    m_width = w;
    m_height = h;
    
    if(s == smy)
      return;
    
    delete [] m_data;
    
    if(s)
      m_data = new T[s];
    else
      m_data = 0;
  }
}

#endif
