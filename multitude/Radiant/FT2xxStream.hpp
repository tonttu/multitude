#ifndef RADIANT_FT2XXSTREAM_HPP
#define RADIANT_FT2XXSTREAM_HPP

#ifndef MULTI_WITH_FTD2XX
#error "Must have FTD2XX support to compile the FT2xxStream"
#endif

#include "BinaryStream.hpp"

#include <Patterns/NotCopyable.hpp>

#include <list>
#include <string>

namespace Radiant {

  //@cond
  class FT2xxStreamInternal;
  //@endcond

  class RADIANT_API FT2xxStream : public BinaryStream, public Patterns::NotCopyable
  {
  public:

    enum DeviceType {
      /// Ordinary slow FT232 device
      DEVICE_FT232,
      /// High-speed device
      DEVICE_FT232H
    };

    FT2xxStream();
    virtual ~FT2xxStream();

    bool open(int index, int timeoutms);

    virtual int read(void * buffer, int bytes, bool waitfordata = true);
    /// Write bytes to the stream
    virtual int write(const void * buffer, int bytes);
    virtual bool isPendingInput(unsigned int waitMicroSeconds = 0);

    /// Returns true if the stream has been closed
    virtual bool isHungUp() const;

    virtual bool close();

    static bool listDevices(std::list<std::string> & devices);
    static bool describeDevices();

    /// Resets all available FTDI devices
    /** You should close all FT2xxStream instances before calling this function. */
    static int cycleAllDevices();

  private:
    FT2xxStreamInternal * m_data;
  };
}


#endif // FT2XXSTREAM_HPP
