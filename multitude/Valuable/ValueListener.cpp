/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "ValueListener.hpp"

#include <algorithm>

#include <Radiant/Trace.hpp>

namespace Valuable
{  


  ValueListener::~ValueListener()
  {
    for(vliterator it = m_listening.begin(); it != m_listening.end(); it++)
      (*it)->remove(this);
  }

  void ValueListener::removeObject(ValueListeners * obj)
  {
    vliterator it = std::find(m_listening.begin(), m_listening.end(), obj);

    if(it != m_listening.end()) {
      m_listening.erase(it);
    }
    else {
      Radiant::error(
"ValueListener::removeObject # Object %p not found", obj);
    }
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ValueListeners::ValueListeners(const ValueListeners & that)
  {
    if(that.size())
      m_list = new container(*that.m_list);
    else
      m_list = 0;
  }

  ValueListeners::~ValueListeners()
  {
    delete m_list;
  }

  void ValueListeners::push_back(ValueListener * listener)
  { 
    makeList();
    m_list->push_back(listener);
    listener->m_listening.push_back(this);
  }

  void ValueListeners::remove(ValueListener * listener)
  {
    if(!m_list)
      return;

    iterator it = std::find(begin(), end(), listener);

    if(it != end())
      m_list->erase(it);
  }

  void ValueListeners::emitChange(ValueObject * obj)
  {
    if(!m_list)
      return;

    for(iterator it = begin(); it != end(); it++) {
      (*it)->valueChanged(obj);
    }
  }

  void ValueListeners::emitDelete(ValueObject * obj)
  {
    if(!m_list)
      return;

    for(iterator it = begin(); it != end(); it++) {
      ValueListener * vl = (*it);
      vl->valueDeleted(obj);
      vl->removeObject(this);
    }
  }
  

  ValueListeners & ValueListeners::operator = (const ValueListeners & that)
  {
    if(that.size()) {
      makeList();
      *m_list = *that.m_list;
    }
    else {
      delete m_list;
      m_list = 0;
    }

    return * this;
  }
}

