include(../../../cornerstone.pri)

INCLUDEPATH += ..

CONFIG += c++11
QT -= gui

win32:QMAKE_CXXFLAGS += /bigobj -w44624

DEFINES += FOLLY_EXPORT

HEADERS += ApplyTuple.h \
           Baton.h \
           CPortability.h \
           ExceptionWrapper.h \
           Executor.h \
           Export.h \
           Hash.h \
           Likely.h \
           Memory.h \
           MicroSpinLock.h \
           MoveWrapper.h \
           Optional.h \
           Portability.h \
           Preprocessor.h \
           ScopeGuard.h \
           SpookyHashV1.h \
           SpookyHashV2.h \
           Traits.h \
           detail/ExceptionWrapper.h \
           detail/Futex.h \
           detail/Sleeper.h \
           detail/UncaughtExceptionCounter.h \
           futures/Barrier.h \
           futures/DrivableExecutor.h \
           futures/Future-inl.h \
           futures/Future-pre.h \
           futures/Future.h \
           futures/FutureException.h \
           futures/helpers.h \
           futures/InlineExecutor.h \
           futures/ManualExecutor.h \
           futures/OpaqueCallbackShunt.h \
           futures/Promise-inl.h \
           futures/Promise.h \
           futures/QueuedImmediateExecutor.h \
           futures/ScheduledExecutor.h \
           futures/SharedPromise-inl.h \
           futures/SharedPromise.h \
           futures/ThreadedExecutor.h \
           futures/Timekeeper.h \
           futures/Try-inl.h \
           futures/Try.h \
           futures/Unit.h \
           futures/detail/Core.h \
           futures/detail/FSM.h \
           futures/detail/SimpleTimekeeper.h \
           futures/detail/Types.h

SOURCES += SpookyHashV1.cpp \
           SpookyHashV2.cpp \
           detail/Futex.cpp \
           futures/Barrier.cpp \
           futures/Future.cpp \
           futures/InlineExecutor.cpp \
           futures/ManualExecutor.cpp \
           futures/QueuedImmediateExecutor.cpp \
           futures/ThreadedExecutor.cpp \
           futures/detail/SimpleTimekeeper.cpp

include(../ThirdParty.pri)
