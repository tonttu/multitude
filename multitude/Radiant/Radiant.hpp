/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_RADIANT_HPP
#define RADIANT_RADIANT_HPP

#include <type_traits>

/** Radiant library is a collection of C++ utility classes.

    Radiant is a collection of C++ classes geared at wrapping
    platform-dependent programming features (threads, mutexes,
    sockets, etc.). Radiant also includes a collection of utilities
    for handling some vary basic string/file manipulation that is
    missing from C/C++ standard libraries.

    \b Copyright: The Radiant library has been developed by Helsinki
    Institute for Information Technology (HIIT, 2006-2008) and
    MultiTouch Oy (2007-2011).

    Radiant is released under the GNU Lesser General Public License
    (LGPL), version 2.1.

    @author Tommi Ilmonen, Esa Nuuros, Jarmo Hiipakka, Juha Laitinen,
    Jari Kleimola, George Whale
*/
namespace Radiant {

  /// Creates a null object for aritchmetic types
  /// @return zero cast to proper type
  /// @tparam Y Type of object whose null value is created
  template<typename Y>
  typename std::enable_if<std::is_arithmetic<Y>::value, Y>::type createNull()
  {
    return Y(0);
  }

  /// Create a null object for non-arithmetic types
  /// @return the return value of the static null() method of the type
  /// @tparam Y Type of object whose null value is created
  template<typename Y>
  typename std::enable_if<!std::is_arithmetic<Y>::value, Y>::type createNull()
  {
    return Y::null();
  }

  class BGThread;
  class BinaryData;
  class BinaryStream;
  class CSVDocument;
  class CameraDriver;
  class Condition;
  class DateTime;
  class Directory;
  class DropEvent;
  class DropListener;
  class FT2xxStream;
  class FileUtils;
  class FileWriter;
  class FunctionTask;
  class ImageConversion;
  class KeyEvent;
  class LockFile;
  class Log;
  class MemChecker;
  class MimeManager;
  class MimeType;
  class MouseEvent;
  class Mutex;
  class Plane;
  class SMRingBuffer;
  class Semaphore;
  class SerialPort;
  class Sleep;
  class SleepSync;
  class SocketUtilPosix;
  class TCPServerSocket;
  class TCPSocket;
  class Task;
  class Thread;
  class ThreadPool;
  class TimeStamp;
  class UDPSocket;
  class Variant;
  class VideoCamera;
  class VideoCamera1394;
  class VideoCameraCMU;
  class VideoCameraPTGrey;
  class VideoImage;
  class VideoInput;
  class WatchDog;
}

#define debugRadiant(...) (Radiant::trace("Radiant", Radiant::Trace::DEBUG, __VA_ARGS__))

#endif
