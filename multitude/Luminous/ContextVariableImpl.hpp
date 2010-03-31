#ifndef LUMINOUS_CONTEXTVARIABLEIMPL_HPP
#define LUMINOUS_CONTEXTVARIABLEIMPL_HPP

#include <Luminous/ContextVariable.hpp>
#include <Luminous/GLResources.hpp>

namespace Luminous {

  template <class T>
  ContextVariableT<T>::~ContextVariableT()
  {}

  template <class T>
  T & ContextVariableT<T>::ref()
  {
    GLRESOURCE_ENSURE3(T, obj, this);
    return *obj;
  }

  template <class T>
  T & ContextVariableT<T>::ref(GLResources * rs)
  {
    GLRESOURCE_ENSURE(T, obj, this, rs);
    return *obj;
  }
}

#endif // CONTEXTVARIABLEIMPL_HPP
