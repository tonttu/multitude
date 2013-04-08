/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef LUMINOUS_CONTEXTARRAY_HPP
#define LUMINOUS_CONTEXTARRAY_HPP

#include "RenderManager.hpp"

#include <QVector>

namespace Luminous
{
  /// This class is a utility class that provides easier handling of OpenGL
  /// context-specific variables. To the developer it looks and acts like a
  /// single variable, but internally it stores a unique value for every
  /// rendering thread. This class is usually not used directly, but the
  /// templated ContextArrayT instead.
  class ContextArray
  {
  public:
    /// Constructor
    inline ContextArray();
    /// Destructor
    inline virtual ~ContextArray();

  private:
    /// Resize the context array to the given number of threads
    /// @param threadCount number of threads
    virtual void resize(unsigned int threadCount) = 0;

    friend class RenderManager;
  };

  /// This class is a utility for handling variables specific to rendering threads.
  /// @sa ContextArray
  template <typename T>
  class ContextArrayT : public QVector<T>, public ContextArray
  {
  public:
    /// Constructor
    inline ContextArrayT();

    /// Get a pointer to an object instance associated with the calling thread
    /// @return pointer to object in the calling thread
    T * operator->() { return &(*this)[RenderManager::threadIndex()]; }
    /// Get a pointer to an object instance associated with the calling thread
    /// @return pointer to object in the calling thread
    const T * operator->() const { return &(*this)[RenderManager::threadIndex()]; }

    /// Get a reference to an object instance associated with the calling thread
    /// @return reference to object in the calling thread
    T & operator*() { return (*this)[RenderManager::threadIndex()]; }
    /// Get a reference to an object instance associated with the calling thread
    /// @return reference to object in the calling thread
    const T & operator*() const { return (*this)[RenderManager::threadIndex()]; }

  private:
    virtual void resize(unsigned int threadCount) OVERRIDE;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ContextArray::ContextArray()
  {
    RenderManager::addContextArray(this);
  }

  ContextArray::~ContextArray()
  {
    RenderManager::removeContextArray(this);
  }


  template <typename T>
  ContextArrayT<T>::ContextArrayT()
  {
    resize(RenderManager::driverCount());
  }

  template <typename T>
  void ContextArrayT<T>::resize(unsigned int threadCount)
  {
    this->clear();
    QVector<T>::resize(threadCount);
  }
}
#endif // LUMINOUS_CONTEXTARRAY_HPP
