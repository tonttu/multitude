
include(../multitude.pri)

HEADERS += WindowConfig.hpp
HEADERS += Window.hpp
macx:HEADERS += CocoaWindow.hpp
HEADERS += XWindow.hpp
HEADERS += QtWindow.hpp
HEADERS += WindowEventHook.hpp
HEADERS += Flags.hpp
HEADERS += Mime.hpp
HEADERS += Timer.hpp
HEADERS += TimerW32.hpp
HEADERS += TimerPosix.hpp
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
HEADERS += ConfigReaderEmpty.hpp
HEADERS += ConfigReaderTmpl.hpp
HEADERS += cycle.h
HEADERS += CycleRecord.hpp
HEADERS += DateTime.hpp
HEADERS += Directory.hpp
HEADERS += Export.hpp
HEADERS += FileUtils.hpp
HEADERS += Functional.hpp
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
HEADERS += RGBA.hpp
HEADERS += RefObj.hpp
HEADERS += RefPtr.hpp
HEADERS += IntrusivePtr.hpp
HEADERS += ResourceLocator.hpp
HEADERS += RingBuffer.hpp
HEADERS += RingBufferImpl.hpp
HEADERS += Semaphore.hpp
HEADERS += SerialPort.hpp
HEADERS += Size2D.hpp
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
HEADERS += VectorStorage.hpp
HEADERS += VideoImage.hpp
HEADERS += VideoInput.hpp
HEADERS += WatchDog.hpp
HEADERS += ClonablePtr.hpp
HEADERS += VideoCamera.hpp
HEADERS += SocketWrapper.hpp
HEADERS += Singleton.hpp
HEADERS += XFaker.hpp
HEADERS += VideoCameraCMU.hpp
HEADERS += VideoCamera1394.hpp
HEADERS += VideoCameraPTGrey.hpp

SOURCES += WindowConfig.cpp
SOURCES += Window.cpp
macx:SOURCES += CocoaWindow.mm
SOURCES += XWindow.cpp
SOURCES += QtWindow.cpp
SOURCES += Mime.cpp
SOURCES += IntrusivePtr.cpp
SOURCES += CameraDriver.cpp
SOURCES += SocketUtilPosix.cpp
SOURCES += ThreadPoolQt.cpp
SOURCES += CSVDocument.cpp
SOURCES += BinaryData.cpp
SOURCES += VideoCamera.cpp
SOURCES += Color.cpp
SOURCES += ColorUtils.cpp
SOURCES += MutexQt.cpp
SOURCES += ThreadQt.cpp
SOURCES += ConfigReader.cpp
SOURCES += DateTime.cpp
SOURCES += DirectoryCommon.cpp
SOURCES += DirectoryQt.cpp
SOURCES += FileUtils.cpp
SOURCES += ImageConversion.cpp
SOURCES += Log.cpp
SOURCES += MemCheck.cpp
SOURCES += CallStackLinux.cpp
SOURCES += ResourceLocator.cpp
SOURCES += RingBuffer.cpp
SOURCES += Size2D.cpp
SOURCES += Sleep.cpp
SOURCES += SemaphoreQt.cpp
SOURCES += SHMDuplexPipe.cpp
SOURCES += SHMPipe.cpp
SOURCES += SMRingBuffer.cpp
SOURCES += StringUtils.cpp
SOURCES += TimeStamp.cpp
SOURCES += Trace.cpp
SOURCES += VideoImage.cpp
SOURCES += VideoInput.cpp
SOURCES += WatchDog.cpp
SOURCES += Singleton.cpp
SOURCES += TCPServerSocketPosix.cpp
SOURCES += TCPSocketPosix.cpp
SOURCES += UDPSocketPosix.cpp
SOURCES += PlatformUtilsLinux.cpp
SOURCES += XFaker.cpp
SOURCES += PlatformUtilsOSX.cpp
SOURCES += SerialPortPosix.cpp
SOURCES += LockFilePosix.cpp
SOURCES += PlatformUtilsWin32.cpp
SOURCES += SerialPortWin32.cpp
SOURCES += LockFileWin32.cpp
SOURCES += CallStackW32.cpp
SOURCES += VideoCameraCMU.cpp
SOURCES += VideoCamera1394.cpp
SOURCES += VideoCameraPTGrey.cpp

# ios:OTHER_FILES += PlatformUtilsIOS.mm
ios {
  OBJECTIVE_SOURCES += PlatformUtilsIOS.mm

}

LIBS += $$LIB_NIMBLE $$LIB_PATTERNS $$LIB_V8

linux-*: LIBS += -lX11 -lXtst

macx:LIBS += -framework,CoreFoundation

DEFINES += RADIANT_EXPORT

unix {
  LIBS += $$LIB_RT -ldl
  PKGCONFIG += libdc1394-2
  !mobile*:DEFINES += CAMERA_DRIVER_1394
  CONFIG += qt
  QT = core network opengl gui
}

win32 {
    message(Radiant on Windows)
    # CMU driver is only 32-bit
    !win64 {
       DEFINES += CAMERA_DRIVER_CMU
       LIBS += 1394camera.lib
    }
    LIBS += Ws2_32.lib \
        ShLwApi.lib \
        shell32.lib \
        psapi.lib
    CONFIG += qt
    QT = core network opengl gui

    PTGREY_PATH = "C:\\Program Files\\Point Grey Research\\FlyCapture2"
    !exists($$PTGREY_PATH/include):warning("PTGrey driver not installed, not building CameraDriverPTGrey")
    exists($$PTGREY_PATH/include) {
        DEFINES += CAMERA_DRIVER_PGR
        message(Using PTGrey camera drivers)
        INCLUDEPATH += $$PTGREY_PATH/include

        # 64bit libs have different path
        win64:QMAKE_LIBDIR += $$PTGREY_PATH/lib64
        else:QMAKE_LIBDIR += $$PTGREY_PATH/lib
        LIBS += FlyCapture2.lib
    }
}

include(../library.pri)




