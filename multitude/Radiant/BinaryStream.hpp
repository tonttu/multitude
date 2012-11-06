/* COPYRIGHT
 */



#ifndef RADIANT_BINARY_STREAM_HPP
#define RADIANT_BINARY_STREAM_HPP

#include "Export.hpp"

namespace Radiant {

  /// Abstract base class for binary streams
  class RADIANT_API BinaryStream
  {
  public:
    BinaryStream() {}
    virtual ~BinaryStream() {}

    /// Read bytes from the stream
    virtual int read(void * buffer, int bytes, bool waitfordata = true) = 0;
    /// Write bytes to the stream
    virtual int write(const void * buffer, int bytes) = 0;
    virtual bool isPendingInput(unsigned int waitMicroSeconds = 0)
    { (void) waitMicroSeconds; return false; }

    /// Returns true if the stream has been closed
    virtual bool isHungUp() const { return false; }

    /// Close the stream
    virtual bool close() { return true; }
  };

}


#endif
