#ifndef GPUASSOCIATION_HPP
#define GPUASSOCIATION_HPP

#include "Export.hpp"
#include "Luminous.hpp"

#include <Patterns/NotCopyable.hpp>

namespace Luminous
{  
  class OpenGLContextHandle : public Patterns::NotCopyable
  {
  public:
    OpenGLContextHandle();
    ~OpenGLContextHandle();

    bool getCurrentContext();

    void* getRawHandle();

  private:
    class D;
    D * m_d;

    friend class GPUAssociation;
  };


  class GPUAssociation
  {
  public:
    static LUMINOUS_API bool isSupported();

    static LUMINOUS_API unsigned int numGPUs();
    static LUMINOUS_API unsigned int gpuId(OpenGLContextHandle *handle);
  };
}

#endif // GPUASSOCIATION_HPP
