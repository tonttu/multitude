include(../../cornerstone.pri)

HEADERS += TempFailureRetry.hpp \
    ThreadChecks.hpp
HEADERS += BlockRingBuffer.hpp
HEADERS += ArrayMap.hpp
HEADERS += ThreadPoolExecutor.hpp
HEADERS += BGThreadExecutor.hpp
HEADERS += ObjectPool.hpp
HEADERS += CommandLineArguments.hpp
HEADERS += SerialPortHelpers.hpp \
    SynchronizedMultiQueue.hpp \
    ProcessRunner.hpp
HEADERS += ArraySet.hpp
HEADERS += Flags.hpp
HEADERS += FutureBool.hpp
HEADERS += DropEvent.hpp
HEADERS += PenEvent.hpp
HEADERS += BGThread.hpp
HEADERS += MovingAverage.hpp
HEADERS += Mime.hpp
HEADERS += Timer.hpp
HEADERS += SynchronizedQueue.hpp
HEADERS += CameraDriver.hpp
HEADERS += Defines.hpp
HEADERS += ThreadPool.hpp
HEADERS += CSVDocument.hpp
HEADERS += UDPSocket.hpp
HEADERS += BinaryData.hpp
HEADERS += BinaryStream.hpp
HEADERS += Color.hpp
HEADERS += ColorUtils.hpp
HEADERS += Condition.hpp
HEADERS += ConfigReader.hpp
HEADERS += ConfigReaderTmpl.hpp
HEADERS += cycle.h
HEADERS += CycleRecord.hpp
HEADERS += DateTime.hpp
HEADERS += Directory.hpp
HEADERS += Export.hpp
HEADERS += FileUtils.hpp
HEADERS += Grid.hpp
HEADERS += ImageConversion.hpp
HEADERS += IODefs.hpp
HEADERS += KeyEvent.hpp
HEADERS += LockFile.hpp
HEADERS += Log.hpp
HEADERS += MemCheck.hpp
HEADERS += CallStack.hpp
HEADERS += Memory.hpp
HEADERS += Allocators.hpp
HEADERS += Mutex.hpp
HEADERS += Platform.hpp
HEADERS += PlatformUtils.hpp
HEADERS += Radiant.hpp
HEADERS += RefObj.hpp
HEADERS += IntrusivePtr.hpp
HEADERS += Task.hpp
HEADERS += RingBuffer.hpp
HEADERS += SafeBool.hpp
HEADERS += Semaphore.hpp
HEADERS += SerialPort.hpp
HEADERS += Sleep.hpp
HEADERS += SHMDuplexPipe.hpp
HEADERS += SHMPipe.hpp
HEADERS += SMRingBuffer.hpp
HEADERS += StringUtils.hpp
HEADERS += SocketUtilPosix.hpp
HEADERS += TCPServerSocket.hpp
HEADERS += TCPSocket.hpp
HEADERS += Thread.hpp
HEADERS += TimeStamp.hpp
HEADERS += Trace.hpp
HEADERS += Types.hpp
HEADERS += TouchEvent.hpp
HEADERS += VectorStorage.hpp
HEADERS += VideoImage.hpp
HEADERS += VideoInput.hpp
HEADERS += WatchDog.hpp
HEADERS += VideoCamera.hpp
HEADERS += SocketWrapper.hpp
HEADERS += Singleton.hpp
HEADERS += VideoCamera1394.hpp
HEADERS += WinTypes.h
HEADERS += DeviceMonitor.hpp
HEADERS += SymbolRegistry.hpp

SOURCES += Mime.cpp \
    ThreadPoolExecutor.cpp \
    BGThreadExecutor.cpp \
    ThreadChecks.cpp
SOURCES += ObjectPool.cpp
SOURCES += CommandLineArguments.cpp
SOURCES += SynchronizedMultiQueue.cpp \
    ProcessRunner.cpp
SOURCES += DropEvent.cpp
SOURCES += PenEvent.cpp
SOURCES += BGThread.cpp
SOURCES += CameraDriver.cpp
SOURCES += SocketUtilPosix.cpp
SOURCES += ThreadPoolQt.cpp
SOURCES += CSVDocument.cpp
SOURCES += BinaryData.cpp
SOURCES += VideoCamera.cpp
SOURCES += Color.cpp
SOURCES += ColorUtils.cpp
SOURCES += CycleRecord.cpp
SOURCES += MutexQt.cpp
SOURCES += ThreadQt.cpp
SOURCES += Task.cpp
SOURCES += ConfigReader.cpp
SOURCES += DateTime.cpp
SOURCES += DirectoryCommon.cpp
SOURCES += DirectoryQt.cpp
SOURCES += FileUtils.cpp
SOURCES += ImageConversion.cpp
SOURCES += KeyEvent.cpp
SOURCES += Log.cpp
SOURCES += MemCheck.cpp
SOURCES += CallStackLinux.cpp
SOURCES += Sleep.cpp
SOURCES += SemaphoreQt.cpp
SOURCES += SHMDuplexPipe.cpp
SOURCES += SHMPipe.cpp
SOURCES += SMRingBuffer.cpp
SOURCES += StringUtils.cpp
SOURCES += TimeStamp.cpp
SOURCES += TouchEvent.cpp
SOURCES += Trace.cpp
SOURCES += VideoImage.cpp
SOURCES += VideoInput.cpp
SOURCES += WatchDog.cpp
SOURCES += Singleton.cpp
SOURCES += TCPServerSocketPosix.cpp
SOURCES += TCPSocketPosix.cpp
SOURCES += UDPSocketPosix.cpp
SOURCES += PlatformUtilsLinux.cpp
SOURCES += PlatformUtilsOSX.cpp
SOURCES += SerialPortPosix.cpp
SOURCES += LockFilePosix.cpp
SOURCES += PlatformUtilsWin32.cpp
SOURCES += SerialPortWin32.cpp
SOURCES += LockFileWin32.cpp
SOURCES += CallStackW32.cpp
SOURCES += VideoCamera1394.cpp
SOURCES += IntrusivePtr.cpp
SOURCES += SymbolRegistry.cpp

linux*:SOURCES += ProcessRunnerPosix.cpp
win32:SOURCES += ProcessRunnerWin32.cpp

# ios:OTHER_FILES += PlatformUtilsIOS.mm
ios {
  OBJECTIVE_SOURCES += PlatformUtilsIOS.mm

}
linux*:SOURCES += DeviceMonitor.cpp

LIBS += $$LIB_NIMBLE $$LIB_PATTERNS $$LIB_V8
LIBS += $$LIB_FTD2XX
LIBS += $$LIB_FOLLY_FUTURES

linux-* {
  LIBS += -lX11
  PKGCONFIG += libudev
}

macx:LIBS += -framework CoreFoundation

DEFINES += RADIANT_EXPORT

unix {
  LIBS += -lpthread $$LIB_RT -ldl
  CONFIG += qt
  QT = core network gui
}

contains(WITH_FTD2XX,yes) {
  HEADERS += FT2xxStream.hpp
  SOURCES += FT2xxStream.cpp

  # Create symlinks to the relevant library:
  linux-* {
    libftdlink.target = ../Linux/lib/libftd2xx.so
    libftdlink.commands = ln -sf libftd2xx.so.1.1.0 ../Linux/lib/libftd2xx.so
    libftdlink.depends =
    QMAKE_EXTRA_TARGETS += libftdlink
    PRE_TARGETDEPS += ../Linux/lib/libftd2xx.so
  }
  macx {
    libftdlink.target = ../OSX/lib/libftd2xx.dylib
    libftdlink.commands = ln -sf libftd2xx.1.1.0.dylib ../OSX/lib/libftd2xx.dylib
    libftdlink.depends =
    QMAKE_EXTRA_TARGETS += libftdlink
    PRE_TARGETDEPS += ../OSX/lib/libftd2xx.dylib
  }
}

win32 {
    LIBS += Ws2_32.lib \
        ShLwApi.lib \
        shell32.lib \
        psapi.lib \
        Advapi32.lib \
        Ole32.lib \
        Winmm.lib \
        Setupapi.lib
    CONFIG += qt
    QT = core network opengl gui

    PTGREY_PATH = "C:\\Program Files\\Point Grey Research\\FlyCapture2"
    !exists($$PTGREY_PATH/include):message("PTGrey driver not installed, not building CameraDriverPTGrey")
    exists($$PTGREY_PATH/include) {
        DEFINES += CAMERA_DRIVER_PGR
        message(Using PTGrey camera drivers)
        INCLUDEPATH += $$PTGREY_PATH/include

        # 64bit libs have different path
        QMAKE_LIBDIR += $$PTGREY_PATH/lib64
        LIBS += FlyCapture2.lib
    }

  HEADERS += DeviceUtilsWin.hpp
  SOURCES += DeviceUtilsWin.cpp
}

include(../library.pri)
