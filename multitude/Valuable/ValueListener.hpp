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

#ifndef VALUABLE_VALUE_LISTENER_HPP
#define VALUABLE_VALUE_LISTENER_HPP

#include <Valuable/Export.hpp>

#include <list>

namespace Valuable
{
  class ValueObject;
  class ValueListeners;

  /// Base class for classes that need to listen on the changes on ValueObjects  
  class VALUABLE_API ValueListener
  {
  public:

    friend class ValueListeners;

    ValueListener() {}
    virtual ~ValueListener();

    /// This method is called when some of the listened values changes
    /// @param o the changed value
    virtual void valueChanged(ValueObject * o) = 0;
    /// This method is called when some of the listened values is deleted
    /// @param o the changed value
    virtual void valueDeleted(ValueObject * o);

  private:

    void removeObject(ValueListeners *);


    typedef std::list<ValueListeners *> vlcontainer;
    typedef vlcontainer::iterator vliterator;

    vlcontainer m_listening;
  };


  /// Container class that is used to store multiple ValueListener objects
  /** This is class is mostly a helper for ValueObject, so that this
      logic can be separated into another class. */
  class VALUABLE_API ValueListeners
  {
  public:
    /// A List of listeners
    typedef std::list<ValueListener *> container;
    /// Iterator for the list
    typedef container::iterator iterator;

    ValueListeners() : m_list(0) {}
    /// Constructs a copy
    ValueListeners(const ValueListeners & that);
    ~ValueListeners();

    /// Adds a listener to the listener list
    void push_back(ValueListener * listener);
    /// Removes a listener frmo the listener list
    void remove(ValueListener * listener);

    /// Returns the number of listeners
    unsigned size() const { return m_list ? (unsigned) m_list->size(): 0; }

    /// Returns true if there are no listeners
    bool empty() const { return m_list ? false : m_list->empty(); }

    /// Returns an iterator to the first listener
    iterator begin() { return m_list->begin(); }
    /// Returns an iterator to the "after-the-last" listener
    iterator end() { return m_list->end(); }

    /// Invokes the valueChanged- method of all the listeners in the list
    void emitChange(ValueObject *);
    /// Invokes the valueDeleted- method of all the listeners in the list
    void emitDelete(ValueObject *);

    /// Copies a listener
    ValueListeners & operator = (const ValueListeners & that);

  private:

    // Make sure that the container list exists
    void makeList()
    {
      if(!m_list)
    m_list = new container;
    }
    /* The container list. This member is pointer rather than an
       object so that in most cases is uses as little memory as
       possible (we assume that most valuebojects will not have
       listeners. */
    container * m_list;
  };

}

#endif
