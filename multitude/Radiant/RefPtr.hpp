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

#ifndef RADIANT_REF_PTR_HPP
#define RADIANT_REF_PTR_HPP

#include <Radiant/Export.hpp>

namespace Radiant {
  
  /// Helper class for RefPtr
  template <typename T>
  class RefPtrInt
  {
  public:
    RefPtrInt(T * obj = 0) : m_object(obj), m_count(1) {}
    ~RefPtrInt() { delete m_object; }
  
    T *m_object;
    unsigned m_count;
  };

  /// Smart pointer with reference counter
  /** This class is used to hold a pointer to some object. The object
      is deleted as the last link to that object is deleted.

      The object type "T" can be an abstract class if necessary.

      You have to be quite careful, not to create multiple RefPtr
      objects that link to the same object, without sharing the
      reference counter.
  */
  template <typename T>
  class RefPtr
  {
  public:
    /// Create a reference to pointer, with NULL pointer
    RefPtr() : m_holder(0) {}
    /// Create a reference to a given pointer
    /** The object will be deleted by RefPtr, so you need to be
	careful with not deleting the objec yourself. */
    RefPtr(T *obj) 
      : m_holder(obj ? new RefPtrInt<T>(obj) : 0) { }
    /// Share a link with another RefPtr
    RefPtr(const RefPtr &that)
    {
      m_holder = ((RefPtr *) & that)->m_holder; 
      if(m_holder) m_holder->m_count++;
    }
  
    /// Deletes this object
    /** The object will be deleted if this is the last RefPtr linking
	to it. */
    ~RefPtr() { breakLink(); }

    T * ptr() { return m_holder ? m_holder->m_object : 0; }
    const T * ptr() const { return m_holder ? m_holder->m_object : 0; }

    /// Operator that returns a pointer to the project
    T * operator *() { return m_holder ? m_holder->m_object : 0; }
    /// Operator that returns a constant pointer to the project
    const T & operator *() const { return m_holder ? m_holder->m_object : 0; }

    RefPtr &link(T * obj) { return ((*this) = obj); }
    /// Clears the link, calling breakLink.
    void clear() { breakLink(); }

    /// Link to the source object
    RefPtr & operator = (const RefPtr &that)
    { 
      if(m_holder == that.m_holder) return *this;

      breakLink(); 
      m_holder = ((RefPtr *) & that)->m_holder; 
      if(m_holder)
	m_holder->m_count++;

      return *this;
    }

    /// Link to the given object
    /** If this RefPtr already links to another, the link is removed. */
    RefPtr & operator = (T *obj)
    { 
      if(m_holder)
	if(obj == m_holder->m_object)
	  return * this;

      breakLink(); 
      if(obj) 
	m_holder = new RefPtrInt<T>(obj); 
      return * this;
    }

    /// Break the link to the object, potentially deleting the object
    void breakLink()
    { 
      if(!m_holder) return; 
      if(! --m_holder->m_count) 
	delete m_holder; 
      m_holder = 0;
    }

    /** Returns true if the pointers of these two objects are
	identical. */
    bool operator == (const RefPtr &that) const
    { return ptr() == that.ptr(); }

    /** Returns true if this RefPtr links to the given pointer. */
    bool operator == (const T * that) const
    { return ptr() == that; }

    T * operator -> () { return m_holder->m_object; }
    const T * operator -> () const { return m_holder->m_object; }

  private:
  
    RefPtrInt<T> *m_holder;
  };


}

#endif
