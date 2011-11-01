#include "FT2xxStream.hpp"

#include "Trace.hpp"

#include "ftd2xx.h"

#include <strings.h>

#include <vector>

namespace Radiant {


  class FT2xxStreamInternal
  {
  public:
    FT2xxStreamInternal() : m_handle(0) {}
    ~FT2xxStreamInternal()
    {
      if(m_handle)
        FT_Close(m_handle);
    }

    FT_HANDLE m_handle;
  };

  FT2xxStream::FT2xxStream()
    : m_data(new FT2xxStreamInternal())
  {}

  FT2xxStream::~FT2xxStream()
  {
    delete m_data;
  }

  bool FT2xxStream::open(int index, int timeoutms)
  {
    FT_STATUS status = FT_Open(index,& m_data->m_handle);

    if(status != FT_OK) {
      return false;
    }

    /*
    status = FT_SetBitMode(m_data->m_handle, 0x1, 0x2);

    if(status == FT_OK)
      info("FT2xxStream::open # bit mode adjusted");
    else
      error("FT2xxStream::open # Could not set bit mode");
      */
    /*
     // This does not work, possibly due to the lack of an EEPROM (or something else)
    UCHAR mode = 0xFF;

    status = FT_GetBitMode(m_data->m_handle, & mode);

    if(status == FT_OK)
      info("FT2xxStream::open # bit mode = %x", (int) mode);
    else
      error("FT2xxStream::open # Could not get bit mode");
      */

    // status = FT_SetBaudRate(m_data->m_handle, FT_BAUD_921600);
    // int baud = 10000000;
    int baud = FT_BAUD_921600 * 13;
    // int baud = FT_BAUD_921600 * 8;
    status = FT_SetBaudRate(m_data->m_handle, baud);

    if(status != FT_OK) {
      error("FT2xxStream::open # Could not set baud rate to %d", baud);
      close();
      return false;
    }

    FT_SetTimeouts(m_data->m_handle, timeoutms, timeoutms);


    return true;
  }

  int FT2xxStream::read(void * buffer, int bytes, bool/* waitfordata*/ )
  {
    DWORD n = 0;
    FT_Read(m_data->m_handle, buffer, bytes, & n);

    return n;
  }

  int FT2xxStream::write(const void * buffer, int bytes)
  {
    DWORD n = 0;
    FT_Write(m_data->m_handle, (void *) buffer, bytes, & n);
    return n;
  }

  bool FT2xxStream::isPendingInput(unsigned int /*waitMicroSeconds*/)
  {
    DWORD inqueue = 0;
    DWORD outqueue = 0;
    DWORD foo;

    FT_GetStatus(m_data->m_handle, & inqueue, & outqueue, & foo);

    return inqueue != 0;
  }

  bool FT2xxStream::isHungUp() const
  {
    return false;
  }

  bool FT2xxStream::close()
  {
    if(!m_data->m_handle)
      return false;

    FT_Close(m_data->m_handle);
    m_data->m_handle = 0;
    return true;
  }

  bool FT2xxStream::listDevices(std::list<std::string> & devices)
  {
    DWORD n = 0;

    devices.clear();

    FT_STATUS status = FT_ListDevices(&n, NULL, FT_LIST_NUMBER_ONLY);

    if(status != FT_OK) {
      return false;
    }

    for(DWORD i = 0; i < n; i++) {
      char buffer[64];
      buffer[0] = 0;
      status = FT_ListDevices((PVOID) i, buffer,
                             FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);

      if(status == FT_OK) {
        devices.push_back(buffer);
        // info("FT2xxStream::listDevices # %d: [%s]", (int) i, buffer);
      }
      else {
        error("FT2xxStream::listDevices # Failed to get serial number for %d", (int) i);
      }
    }

    return true;
  }

  bool FT2xxStream::describeDevices()
  {
    std::vector<FT_DEVICE_LIST_INFO_NODE> infos;

    DWORD n;

    // create the device information list
    FT_STATUS status = FT_CreateDeviceInfoList(&n);
    if (status != FT_OK) {
      return false;
    }

    if(!n)
      return true;

    if (n > 0) {
      infos.resize(n);
      bzero(& infos.at(0), sizeof(infos[0]));
      status = FT_GetDeviceInfoList(& infos.at(0),&n);
      if (status == FT_OK) {
        for (DWORD i = 0; i < n; i++) {

          FT_DEVICE_LIST_INFO_NODE & in = infos[i];

          info("Dev %d:", i);
          info(" Flags = 0x%x", in.Flags);
          info(" Type = 0x%x", in.Type);
          info(" ID = 0x%x", in.ID);
          info(" LocId = 0x%x", in.LocId);
          info(" SerialNumber = %s", in.SerialNumber);
          info(" Description = %s", in.Description);
          info(" ftHandle = 0x%p", in.ftHandle);
        }
      }
    }

    return true;
  }

}
