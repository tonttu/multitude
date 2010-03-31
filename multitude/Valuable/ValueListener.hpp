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
  /// @todo Options: Document, deprecate, remove.
  class VALUABLE_API ValueListener
  {
  public:

    friend class ValueListeners;

    ValueListener() {}
    virtual ~ValueListener();

    /// This method is called when some of the listened values changes
    virtual void valueChanged(ValueObject *) = 0;
    /// This method is called when some of the listened values is deleted
    virtual void valueDeleted(ValueObject *) = 0;

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
    typedef std::list<ValueListener *> container;
    typedef container::iterator iterator;

    ValueListeners() : m_list(0) {}
    ValueListeners(const ValueListeners & that);

    ~ValueListeners();

    /// Adds a listener to the listener list
    void push_back(ValueListener * listener);
    /// Removes a listener frmo the listener list
    void remove(ValueListener * listener);

    /// Returns the number of listeners
    unsigned size() const { return m_list ? (unsigned) m_list->size(): 0; }

    /// Returns an iterator to the first listener
    iterator begin() { return m_list->begin(); }
    /// Returns an iterator to the "after-the-last" listener
    iterator end() { return m_list->end(); }

    /// Invokes the valueChanged- method of all the listeners in the list
    void emitChange(ValueObject *);
    /// Invokes the valueDeleted- method of all the listeners in the list
    void emitDelete(ValueObject *);

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
