#if !defined(LUMINOUS_HARDWAREBUFFERIMPL_HPP)
#define LUMINOUS_HARDWAREBUFFERIMPL_HPP

#include "Luminous/HardwareBuffer.hpp"
#include "Luminous/Luminous.hpp"

#include <memory>

namespace Luminous
{
  class HardwareBuffer::D
  {
  public:
    struct Data
    {
      HardwareBuffer * owner;
      size_t size;
      const char * data;
      BufferUsage usage;
    };

    typedef std::shared_ptr<Data> DataPtr;
  public:
    D(HardwareBuffer * owner)
      : data(std::make_shared<Data>())
    {
      data->owner = owner;
      data->size = 0;
      data->data = nullptr;
      data->usage = BufferUsage_Static_Draw;
    }

    DataPtr data;
  };
}

#endif // LUMINOUS_HARDWAREBUFFERIMPL_HPP