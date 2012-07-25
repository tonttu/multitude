#ifndef LUMINOUS_CONTEXTARRAY_HPP
#define LUMINOUS_CONTEXTARRAY_HPP

#include "RenderManager.hpp"

#include <QVector>

namespace Luminous
{
  class ContextArray
  {
  public:
    inline ContextArray();
    inline virtual ~ContextArray();

  private:
    virtual void resize(unsigned int threadCount) = 0;
    friend class RenderManager;
  };


  template <typename T>
  class ContextArrayT : public QVector<T>, public ContextArray
  {
  public:
    inline ContextArrayT();

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
