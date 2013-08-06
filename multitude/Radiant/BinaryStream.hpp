/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */



#ifndef RADIANT_BINARY_STREAM_HPP
#define RADIANT_BINARY_STREAM_HPP

#include "Export.hpp"

namespace Radiant {

  /// Abstract base class for binary streams
  class RADIANT_API BinaryStream
  {
  public:
    /// Constructor
    BinaryStream() {}
    /// Destructor
    virtual ~BinaryStream() {}

    /// Read bytes from the stream
    /// @param buffer Buffer to read from
    /// @param bytes Number of bytes to read
    /// @param waitfordata Does this call block until all of the data is available
    /// @return Number of bytes to read
    virtual int read(void * buffer, int bytes, bool waitfordata = true) = 0;

    /// Write bytes to the stream
    /// @param buffer Buffer to write
    /// @param bytes Number of bytes to write
    /// @return Number of bytes actually written
    virtual int write(const void * buffer, int bytes) = 0;

    /// Returns true if the stream has at least one byte waiting to be read; otherwise returns false
    /// @param waitMicroSeconds How many microseconds this call will block at most.
    ///                         In this class the implementation ignores this parameter.
    /// @return True if pending input. Base implementation returns always false.
    virtual bool isPendingInput(unsigned int waitMicroSeconds = 0)
    { (void) waitMicroSeconds; return false; }

    /// Returns true if the stream has been closed
    /// @return True if hung up. Base implementation returns always false.
    virtual bool isHungUp() const { return false; }

    /// Close the stream
    /// @return True if succeeded. Base implementation returns always true
    virtual bool close() { return true; }

    /// Checks whether the stream is still accessible
    /// @return True if the stream is open and accessible
    virtual bool isOpen() const { return true; }
  };

}


#endif
