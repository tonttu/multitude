include(../../cornerstone.pri)

HEADERS += TempFailureRetry.hpp \
    ThreadChecks.hpp \
    TraceDuplicateFilter.hpp \
    TraceStdFilter.hpp \
    TraceSeverityFilter.hpp \
    TraceQIODeviceFilter.hpp \
    SystemCpuTime.hpp \
    ReentrantVector.hpp \
    TemporaryDir.hpp
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
HEADERS += VideoImage.hpp
HEADERS += VideoInput.hpp
HEADERS += WatchDog.hpp
HEADERS += VideoCamera.hpp
HEADERS += SocketWrapper.hpp
HEADERS += Singleton.hpp
HEADERS += VideoCamera1394.hpp
HEADERS += DeviceMonitor.hpp
HEADERS += SymbolRegistry.hpp
HEADERS += CrashHandler.hpp
HEADERS += Version.hpp
HEADERS += VersionGenerated.hpp
HEADERS += VersionString.hpp
HEADERS += TraceCrashHandlerFilter.hpp
HEADERS += fast_atof.h

SOURCES += Mime.cpp \
    ThreadPoolExecutor.cpp \
    BGThreadExecutor.cpp \
    ThreadChecks.cpp \
    TraceDuplicateFilter.cpp \
    TraceStdFilter.cpp \
    TraceSeverityFilter.cpp \
    TraceQIODeviceFilter.cpp \
    TemporaryDir.cpp
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
SOURCES += SetupSearchPaths.cpp
SOURCES += Version.cpp
SOURCES += VersionString.cpp
SOURCES += CrashHandlerCommon.cpp
SOURCES += TraceCrashHandlerFilter.cpp

linux*:SOURCES += ProcessRunnerPosix.cpp
win32:SOURCES += ProcessRunnerWin32.cpp

# ios:OTHER_FILES += PlatformUtilsIOS.mm
ios {
  OBJECTIVE_SOURCES += PlatformUtilsIOS.mm

}
linux*:SOURCES += DeviceMonitor.cpp

LIBS += $$LIB_NIMBLE $$LIB_PATTERNS $$LIB_V8
LIBS += $$LIB_FOLLY_FUTURES

linux-* {
  LIBS += -lX11
  PKGCONFIG += libudev

  HEADERS += TraceSyslogFilter.hpp
  SOURCES += TraceSyslogFilter.cpp

  SOURCES += SystemCpuTimeLinux.cpp

  # CrashHandler requirements
  PKGCONFIG += breakpad-client
  SOURCES += CrashHandlerBreakpad.cpp
}

macx {
  LIBS += -framework CoreFoundation

  SOURCES += SystemCpuTimeOSX.cpp

  SOURCES += CrashHandlerDummy.cpp
}

DEFINES += RADIANT_EXPORT

unix {
  SOURCES += CallStackUnix.cpp

  LIBS += -lpthread $$LIB_RT -ldl
  CONFIG += qt
  QT = core network gui
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

  HEADERS += DeviceUtilsWin.hpp
  SOURCES += DeviceUtilsWin.cpp

  HEADERS += TraceWindowsDebugConsoleFilter.hpp
  SOURCES += TraceWindowsDebugConsoleFilter.cpp

  SOURCES += SystemCpuTimeWin32.cpp

  # CrashHandler requirements
  SOURCES += CrashHandlerCrashpad.cpp
  INCLUDEPATH += $$CORNERSTONE_DEPS_PATH/crashpad/include
  INCLUDEPATH += $$CORNERSTONE_DEPS_PATH/crashpad/include/third_party/mini_chromium/mini_chromium
  QMAKE_LIBDIR += $$CORNERSTONE_DEPS_PATH/crashpad/lib

  CONFIG(release, debug|release) {
    LIBS += -lcrashpad
  } else {
    LIBS += -lcrashpadd
  }

  LIBS += -lAdvapi32 -lRpcrt4 -lShell32
}

include(../../library.pri)
