######################################################################
# Automatically generated by qmake (3.0) Mon May 23 17:09:21 2016
######################################################################
include(../../multitude.pri)

macx:DEFINES += SYSTEM_DARWIN

TEMPLATE = lib
CONFIG += staticlib
TARGET = glbinding
INCLUDEPATH += source/glbinding/include

win32 {
  # Use multi-byte character set instead of unicode
  DEFINES -= UNICODE
  DEFINES += _MBCS
  DEFINES += SYSTEM_WINDOWS

  # Work around C1128
  QMAKE_CXXFLAGS += /bigobj
}

# Build static library
DEFINES += GLBINDING_STATIC_DEFINE

# Input
HEADERS += source/glbinding/source/Binding_pch.h \
           source/glbinding/source/callbacks_private.h \
           source/glbinding/source/glrevision.h \
           source/glbinding/source/logging_private.h \
           source/glbinding/source/Meta_Maps.h \
           source/glbinding/source/RingBuffer.h \
           source/glbinding/source/RingBuffer.hpp \
           source/include/glbinding/glbinding-version.h \
           source/glbinding/include/glbinding/AbstractFunction.h \
           source/glbinding/include/glbinding/AbstractValue.h \
           source/glbinding/include/glbinding/Binding.h \
           source/glbinding/include/glbinding/CallbackMask.h \
           source/glbinding/include/glbinding/callbacks.h \
           source/glbinding/include/glbinding/ContextHandle.h \
           source/glbinding/include/glbinding/ContextInfo.h \
           source/glbinding/include/glbinding/Function.h \
           source/glbinding/include/glbinding/Function.hpp \
           source/glbinding/include/glbinding/FunctionCall.h \
           source/glbinding/include/glbinding/glbinding_api.h \
           source/glbinding/include/glbinding/glbinding_features.h \
           source/glbinding/include/glbinding/logging.h \
           source/glbinding/include/glbinding/Meta.h \
           source/glbinding/include/glbinding/nogl.h \
           source/glbinding/include/glbinding/ProcAddress.h \
           source/glbinding/include/glbinding/SharedBitfield.h \
           source/glbinding/include/glbinding/SharedBitfield.hpp \
           source/glbinding/include/glbinding/Value.h \
           source/glbinding/include/glbinding/Value.hpp \
           source/glbinding/include/glbinding/Version.h \
           source/glbinding/include/glbinding/gl/bitfield.h \
           source/glbinding/include/glbinding/gl/boolean.h \
           source/glbinding/include/glbinding/gl/enum.h \
           source/glbinding/include/glbinding/gl/extension.h \
           source/glbinding/include/glbinding/gl/functions-patches.h \
           source/glbinding/include/glbinding/gl/functions.h \
           source/glbinding/include/glbinding/gl/gl.h \
           source/glbinding/include/glbinding/gl/types.h \
           source/glbinding/include/glbinding/gl/values.h \
           source/glbinding/include/glbinding/gl10/bitfield.h \
           source/glbinding/include/glbinding/gl10/boolean.h \
           source/glbinding/include/glbinding/gl10/enum.h \
           source/glbinding/include/glbinding/gl10/functions.h \
           source/glbinding/include/glbinding/gl10/gl.h \
           source/glbinding/include/glbinding/gl10/types.h \
           source/glbinding/include/glbinding/gl10/values.h \
           source/glbinding/include/glbinding/gl10ext/bitfield.h \
           source/glbinding/include/glbinding/gl10ext/boolean.h \
           source/glbinding/include/glbinding/gl10ext/enum.h \
           source/glbinding/include/glbinding/gl10ext/functions.h \
           source/glbinding/include/glbinding/gl10ext/gl.h \
           source/glbinding/include/glbinding/gl10ext/types.h \
           source/glbinding/include/glbinding/gl10ext/values.h \
           source/glbinding/include/glbinding/gl11/bitfield.h \
           source/glbinding/include/glbinding/gl11/boolean.h \
           source/glbinding/include/glbinding/gl11/enum.h \
           source/glbinding/include/glbinding/gl11/functions.h \
           source/glbinding/include/glbinding/gl11/gl.h \
           source/glbinding/include/glbinding/gl11/types.h \
           source/glbinding/include/glbinding/gl11/values.h \
           source/glbinding/include/glbinding/gl11ext/bitfield.h \
           source/glbinding/include/glbinding/gl11ext/boolean.h \
           source/glbinding/include/glbinding/gl11ext/enum.h \
           source/glbinding/include/glbinding/gl11ext/functions.h \
           source/glbinding/include/glbinding/gl11ext/gl.h \
           source/glbinding/include/glbinding/gl11ext/types.h \
           source/glbinding/include/glbinding/gl11ext/values.h \
           source/glbinding/include/glbinding/gl12/bitfield.h \
           source/glbinding/include/glbinding/gl12/boolean.h \
           source/glbinding/include/glbinding/gl12/enum.h \
           source/glbinding/include/glbinding/gl12/functions.h \
           source/glbinding/include/glbinding/gl12/gl.h \
           source/glbinding/include/glbinding/gl12/types.h \
           source/glbinding/include/glbinding/gl12/values.h \
           source/glbinding/include/glbinding/gl12ext/bitfield.h \
           source/glbinding/include/glbinding/gl12ext/boolean.h \
           source/glbinding/include/glbinding/gl12ext/enum.h \
           source/glbinding/include/glbinding/gl12ext/functions.h \
           source/glbinding/include/glbinding/gl12ext/gl.h \
           source/glbinding/include/glbinding/gl12ext/types.h \
           source/glbinding/include/glbinding/gl12ext/values.h \
           source/glbinding/include/glbinding/gl13/bitfield.h \
           source/glbinding/include/glbinding/gl13/boolean.h \
           source/glbinding/include/glbinding/gl13/enum.h \
           source/glbinding/include/glbinding/gl13/functions.h \
           source/glbinding/include/glbinding/gl13/gl.h \
           source/glbinding/include/glbinding/gl13/types.h \
           source/glbinding/include/glbinding/gl13/values.h \
           source/glbinding/include/glbinding/gl13ext/bitfield.h \
           source/glbinding/include/glbinding/gl13ext/boolean.h \
           source/glbinding/include/glbinding/gl13ext/enum.h \
           source/glbinding/include/glbinding/gl13ext/functions.h \
           source/glbinding/include/glbinding/gl13ext/gl.h \
           source/glbinding/include/glbinding/gl13ext/types.h \
           source/glbinding/include/glbinding/gl13ext/values.h \
           source/glbinding/include/glbinding/gl14/bitfield.h \
           source/glbinding/include/glbinding/gl14/boolean.h \
           source/glbinding/include/glbinding/gl14/enum.h \
           source/glbinding/include/glbinding/gl14/functions.h \
           source/glbinding/include/glbinding/gl14/gl.h \
           source/glbinding/include/glbinding/gl14/types.h \
           source/glbinding/include/glbinding/gl14/values.h \
           source/glbinding/include/glbinding/gl14ext/bitfield.h \
           source/glbinding/include/glbinding/gl14ext/boolean.h \
           source/glbinding/include/glbinding/gl14ext/enum.h \
           source/glbinding/include/glbinding/gl14ext/functions.h \
           source/glbinding/include/glbinding/gl14ext/gl.h \
           source/glbinding/include/glbinding/gl14ext/types.h \
           source/glbinding/include/glbinding/gl14ext/values.h \
           source/glbinding/include/glbinding/gl15/bitfield.h \
           source/glbinding/include/glbinding/gl15/boolean.h \
           source/glbinding/include/glbinding/gl15/enum.h \
           source/glbinding/include/glbinding/gl15/functions.h \
           source/glbinding/include/glbinding/gl15/gl.h \
           source/glbinding/include/glbinding/gl15/types.h \
           source/glbinding/include/glbinding/gl15/values.h \
           source/glbinding/include/glbinding/gl15ext/bitfield.h \
           source/glbinding/include/glbinding/gl15ext/boolean.h \
           source/glbinding/include/glbinding/gl15ext/enum.h \
           source/glbinding/include/glbinding/gl15ext/functions.h \
           source/glbinding/include/glbinding/gl15ext/gl.h \
           source/glbinding/include/glbinding/gl15ext/types.h \
           source/glbinding/include/glbinding/gl15ext/values.h \
           source/glbinding/include/glbinding/gl20/bitfield.h \
           source/glbinding/include/glbinding/gl20/boolean.h \
           source/glbinding/include/glbinding/gl20/enum.h \
           source/glbinding/include/glbinding/gl20/functions.h \
           source/glbinding/include/glbinding/gl20/gl.h \
           source/glbinding/include/glbinding/gl20/types.h \
           source/glbinding/include/glbinding/gl20/values.h \
           source/glbinding/include/glbinding/gl20ext/bitfield.h \
           source/glbinding/include/glbinding/gl20ext/boolean.h \
           source/glbinding/include/glbinding/gl20ext/enum.h \
           source/glbinding/include/glbinding/gl20ext/functions.h \
           source/glbinding/include/glbinding/gl20ext/gl.h \
           source/glbinding/include/glbinding/gl20ext/types.h \
           source/glbinding/include/glbinding/gl20ext/values.h \
           source/glbinding/include/glbinding/gl21/bitfield.h \
           source/glbinding/include/glbinding/gl21/boolean.h \
           source/glbinding/include/glbinding/gl21/enum.h \
           source/glbinding/include/glbinding/gl21/functions.h \
           source/glbinding/include/glbinding/gl21/gl.h \
           source/glbinding/include/glbinding/gl21/types.h \
           source/glbinding/include/glbinding/gl21/values.h \
           source/glbinding/include/glbinding/gl21ext/bitfield.h \
           source/glbinding/include/glbinding/gl21ext/boolean.h \
           source/glbinding/include/glbinding/gl21ext/enum.h \
           source/glbinding/include/glbinding/gl21ext/functions.h \
           source/glbinding/include/glbinding/gl21ext/gl.h \
           source/glbinding/include/glbinding/gl21ext/types.h \
           source/glbinding/include/glbinding/gl21ext/values.h \
           source/glbinding/include/glbinding/gl30/bitfield.h \
           source/glbinding/include/glbinding/gl30/boolean.h \
           source/glbinding/include/glbinding/gl30/enum.h \
           source/glbinding/include/glbinding/gl30/functions.h \
           source/glbinding/include/glbinding/gl30/gl.h \
           source/glbinding/include/glbinding/gl30/types.h \
           source/glbinding/include/glbinding/gl30/values.h \
           source/glbinding/include/glbinding/gl30ext/bitfield.h \
           source/glbinding/include/glbinding/gl30ext/boolean.h \
           source/glbinding/include/glbinding/gl30ext/enum.h \
           source/glbinding/include/glbinding/gl30ext/functions.h \
           source/glbinding/include/glbinding/gl30ext/gl.h \
           source/glbinding/include/glbinding/gl30ext/types.h \
           source/glbinding/include/glbinding/gl30ext/values.h \
           source/glbinding/include/glbinding/gl31/bitfield.h \
           source/glbinding/include/glbinding/gl31/boolean.h \
           source/glbinding/include/glbinding/gl31/enum.h \
           source/glbinding/include/glbinding/gl31/functions.h \
           source/glbinding/include/glbinding/gl31/gl.h \
           source/glbinding/include/glbinding/gl31/types.h \
           source/glbinding/include/glbinding/gl31/values.h \
           source/glbinding/include/glbinding/gl31ext/bitfield.h \
           source/glbinding/include/glbinding/gl31ext/boolean.h \
           source/glbinding/include/glbinding/gl31ext/enum.h \
           source/glbinding/include/glbinding/gl31ext/functions.h \
           source/glbinding/include/glbinding/gl31ext/gl.h \
           source/glbinding/include/glbinding/gl31ext/types.h \
           source/glbinding/include/glbinding/gl31ext/values.h \
           source/glbinding/include/glbinding/gl32/bitfield.h \
           source/glbinding/include/glbinding/gl32/boolean.h \
           source/glbinding/include/glbinding/gl32/enum.h \
           source/glbinding/include/glbinding/gl32/functions.h \
           source/glbinding/include/glbinding/gl32/gl.h \
           source/glbinding/include/glbinding/gl32/types.h \
           source/glbinding/include/glbinding/gl32/values.h \
           source/glbinding/include/glbinding/gl32core/bitfield.h \
           source/glbinding/include/glbinding/gl32core/boolean.h \
           source/glbinding/include/glbinding/gl32core/enum.h \
           source/glbinding/include/glbinding/gl32core/functions.h \
           source/glbinding/include/glbinding/gl32core/gl.h \
           source/glbinding/include/glbinding/gl32core/types.h \
           source/glbinding/include/glbinding/gl32core/values.h \
           source/glbinding/include/glbinding/gl32ext/bitfield.h \
           source/glbinding/include/glbinding/gl32ext/boolean.h \
           source/glbinding/include/glbinding/gl32ext/enum.h \
           source/glbinding/include/glbinding/gl32ext/functions.h \
           source/glbinding/include/glbinding/gl32ext/gl.h \
           source/glbinding/include/glbinding/gl32ext/types.h \
           source/glbinding/include/glbinding/gl32ext/values.h \
           source/glbinding/include/glbinding/gl33/bitfield.h \
           source/glbinding/include/glbinding/gl33/boolean.h \
           source/glbinding/include/glbinding/gl33/enum.h \
           source/glbinding/include/glbinding/gl33/functions.h \
           source/glbinding/include/glbinding/gl33/gl.h \
           source/glbinding/include/glbinding/gl33/types.h \
           source/glbinding/include/glbinding/gl33/values.h \
           source/glbinding/include/glbinding/gl33core/bitfield.h \
           source/glbinding/include/glbinding/gl33core/boolean.h \
           source/glbinding/include/glbinding/gl33core/enum.h \
           source/glbinding/include/glbinding/gl33core/functions.h \
           source/glbinding/include/glbinding/gl33core/gl.h \
           source/glbinding/include/glbinding/gl33core/types.h \
           source/glbinding/include/glbinding/gl33core/values.h \
           source/glbinding/include/glbinding/gl33ext/bitfield.h \
           source/glbinding/include/glbinding/gl33ext/boolean.h \
           source/glbinding/include/glbinding/gl33ext/enum.h \
           source/glbinding/include/glbinding/gl33ext/functions.h \
           source/glbinding/include/glbinding/gl33ext/gl.h \
           source/glbinding/include/glbinding/gl33ext/types.h \
           source/glbinding/include/glbinding/gl33ext/values.h \
           source/glbinding/include/glbinding/gl40/bitfield.h \
           source/glbinding/include/glbinding/gl40/boolean.h \
           source/glbinding/include/glbinding/gl40/enum.h \
           source/glbinding/include/glbinding/gl40/functions.h \
           source/glbinding/include/glbinding/gl40/gl.h \
           source/glbinding/include/glbinding/gl40/types.h \
           source/glbinding/include/glbinding/gl40/values.h \
           source/glbinding/include/glbinding/gl40core/bitfield.h \
           source/glbinding/include/glbinding/gl40core/boolean.h \
           source/glbinding/include/glbinding/gl40core/enum.h \
           source/glbinding/include/glbinding/gl40core/functions.h \
           source/glbinding/include/glbinding/gl40core/gl.h \
           source/glbinding/include/glbinding/gl40core/types.h \
           source/glbinding/include/glbinding/gl40core/values.h \
           source/glbinding/include/glbinding/gl40ext/bitfield.h \
           source/glbinding/include/glbinding/gl40ext/boolean.h \
           source/glbinding/include/glbinding/gl40ext/enum.h \
           source/glbinding/include/glbinding/gl40ext/functions.h \
           source/glbinding/include/glbinding/gl40ext/gl.h \
           source/glbinding/include/glbinding/gl40ext/types.h \
           source/glbinding/include/glbinding/gl40ext/values.h \
           source/glbinding/include/glbinding/gl41/bitfield.h \
           source/glbinding/include/glbinding/gl41/boolean.h \
           source/glbinding/include/glbinding/gl41/enum.h \
           source/glbinding/include/glbinding/gl41/functions.h \
           source/glbinding/include/glbinding/gl41/gl.h \
           source/glbinding/include/glbinding/gl41/types.h \
           source/glbinding/include/glbinding/gl41/values.h \
           source/glbinding/include/glbinding/gl41core/bitfield.h \
           source/glbinding/include/glbinding/gl41core/boolean.h \
           source/glbinding/include/glbinding/gl41core/enum.h \
           source/glbinding/include/glbinding/gl41core/functions.h \
           source/glbinding/include/glbinding/gl41core/gl.h \
           source/glbinding/include/glbinding/gl41core/types.h \
           source/glbinding/include/glbinding/gl41core/values.h \
           source/glbinding/include/glbinding/gl41ext/bitfield.h \
           source/glbinding/include/glbinding/gl41ext/boolean.h \
           source/glbinding/include/glbinding/gl41ext/enum.h \
           source/glbinding/include/glbinding/gl41ext/functions.h \
           source/glbinding/include/glbinding/gl41ext/gl.h \
           source/glbinding/include/glbinding/gl41ext/types.h \
           source/glbinding/include/glbinding/gl41ext/values.h \
           source/glbinding/include/glbinding/gl42/bitfield.h \
           source/glbinding/include/glbinding/gl42/boolean.h \
           source/glbinding/include/glbinding/gl42/enum.h \
           source/glbinding/include/glbinding/gl42/functions.h \
           source/glbinding/include/glbinding/gl42/gl.h \
           source/glbinding/include/glbinding/gl42/types.h \
           source/glbinding/include/glbinding/gl42/values.h \
           source/glbinding/include/glbinding/gl42core/bitfield.h \
           source/glbinding/include/glbinding/gl42core/boolean.h \
           source/glbinding/include/glbinding/gl42core/enum.h \
           source/glbinding/include/glbinding/gl42core/functions.h \
           source/glbinding/include/glbinding/gl42core/gl.h \
           source/glbinding/include/glbinding/gl42core/types.h \
           source/glbinding/include/glbinding/gl42core/values.h \
           source/glbinding/include/glbinding/gl42ext/bitfield.h \
           source/glbinding/include/glbinding/gl42ext/boolean.h \
           source/glbinding/include/glbinding/gl42ext/enum.h \
           source/glbinding/include/glbinding/gl42ext/functions.h \
           source/glbinding/include/glbinding/gl42ext/gl.h \
           source/glbinding/include/glbinding/gl42ext/types.h \
           source/glbinding/include/glbinding/gl42ext/values.h \
           source/glbinding/include/glbinding/gl43/bitfield.h \
           source/glbinding/include/glbinding/gl43/boolean.h \
           source/glbinding/include/glbinding/gl43/enum.h \
           source/glbinding/include/glbinding/gl43/functions.h \
           source/glbinding/include/glbinding/gl43/gl.h \
           source/glbinding/include/glbinding/gl43/types.h \
           source/glbinding/include/glbinding/gl43/values.h \
           source/glbinding/include/glbinding/gl43core/bitfield.h \
           source/glbinding/include/glbinding/gl43core/boolean.h \
           source/glbinding/include/glbinding/gl43core/enum.h \
           source/glbinding/include/glbinding/gl43core/functions.h \
           source/glbinding/include/glbinding/gl43core/gl.h \
           source/glbinding/include/glbinding/gl43core/types.h \
           source/glbinding/include/glbinding/gl43core/values.h \
           source/glbinding/include/glbinding/gl43ext/bitfield.h \
           source/glbinding/include/glbinding/gl43ext/boolean.h \
           source/glbinding/include/glbinding/gl43ext/enum.h \
           source/glbinding/include/glbinding/gl43ext/functions.h \
           source/glbinding/include/glbinding/gl43ext/gl.h \
           source/glbinding/include/glbinding/gl43ext/types.h \
           source/glbinding/include/glbinding/gl43ext/values.h \
           source/glbinding/include/glbinding/gl44/bitfield.h \
           source/glbinding/include/glbinding/gl44/boolean.h \
           source/glbinding/include/glbinding/gl44/enum.h \
           source/glbinding/include/glbinding/gl44/functions.h \
           source/glbinding/include/glbinding/gl44/gl.h \
           source/glbinding/include/glbinding/gl44/types.h \
           source/glbinding/include/glbinding/gl44/values.h \
           source/glbinding/include/glbinding/gl44core/bitfield.h \
           source/glbinding/include/glbinding/gl44core/boolean.h \
           source/glbinding/include/glbinding/gl44core/enum.h \
           source/glbinding/include/glbinding/gl44core/functions.h \
           source/glbinding/include/glbinding/gl44core/gl.h \
           source/glbinding/include/glbinding/gl44core/types.h \
           source/glbinding/include/glbinding/gl44core/values.h \
           source/glbinding/include/glbinding/gl44ext/bitfield.h \
           source/glbinding/include/glbinding/gl44ext/boolean.h \
           source/glbinding/include/glbinding/gl44ext/enum.h \
           source/glbinding/include/glbinding/gl44ext/functions.h \
           source/glbinding/include/glbinding/gl44ext/gl.h \
           source/glbinding/include/glbinding/gl44ext/types.h \
           source/glbinding/include/glbinding/gl44ext/values.h \
           source/glbinding/include/glbinding/gl45/bitfield.h \
           source/glbinding/include/glbinding/gl45/boolean.h \
           source/glbinding/include/glbinding/gl45/enum.h \
           source/glbinding/include/glbinding/gl45/functions.h \
           source/glbinding/include/glbinding/gl45/gl.h \
           source/glbinding/include/glbinding/gl45/types.h \
           source/glbinding/include/glbinding/gl45/values.h \
           source/glbinding/include/glbinding/gl45core/bitfield.h \
           source/glbinding/include/glbinding/gl45core/boolean.h \
           source/glbinding/include/glbinding/gl45core/enum.h \
           source/glbinding/include/glbinding/gl45core/functions.h \
           source/glbinding/include/glbinding/gl45core/gl.h \
           source/glbinding/include/glbinding/gl45core/types.h \
           source/glbinding/include/glbinding/gl45core/values.h \
           source/glbinding/include/glbinding/gl45ext/bitfield.h \
           source/glbinding/include/glbinding/gl45ext/boolean.h \
           source/glbinding/include/glbinding/gl45ext/enum.h \
           source/glbinding/include/glbinding/gl45ext/functions.h \
           source/glbinding/include/glbinding/gl45ext/gl.h \
           source/glbinding/include/glbinding/gl45ext/types.h \
           source/glbinding/include/glbinding/gl45ext/values.h
SOURCES += source/glbinding/source/AbstractFunction.cpp \
           source/glbinding/source/AbstractValue.cpp \
           source/glbinding/source/Binding.cpp \
           source/glbinding/source/Binding_list.cpp \
           source/glbinding/source/Binding_objects_a.cpp \
           source/glbinding/source/Binding_objects_b.cpp \
           source/glbinding/source/Binding_objects_c.cpp \
           source/glbinding/source/Binding_objects_d.cpp \
           source/glbinding/source/Binding_objects_e.cpp \
           source/glbinding/source/Binding_objects_f.cpp \
           source/glbinding/source/Binding_objects_g.cpp \
           source/glbinding/source/Binding_objects_h.cpp \
           source/glbinding/source/Binding_objects_i.cpp \
           source/glbinding/source/Binding_objects_l.cpp \
           source/glbinding/source/Binding_objects_m.cpp \
           source/glbinding/source/Binding_objects_n.cpp \
           source/glbinding/source/Binding_objects_o.cpp \
           source/glbinding/source/Binding_objects_p.cpp \
           source/glbinding/source/Binding_objects_q.cpp \
           source/glbinding/source/Binding_objects_r.cpp \
           source/glbinding/source/Binding_objects_s.cpp \
           source/glbinding/source/Binding_objects_t.cpp \
           source/glbinding/source/Binding_objects_u.cpp \
           source/glbinding/source/Binding_objects_v.cpp \
           source/glbinding/source/Binding_objects_w.cpp \
           source/glbinding/source/CallbackMask.cpp \
           source/glbinding/source/callbacks.cpp \
           source/glbinding/source/ContextHandle.cpp \
           source/glbinding/source/ContextInfo.cpp \
           source/glbinding/source/FunctionCall.cpp \
           source/glbinding/source/logging.cpp \
           source/glbinding/source/Meta.cpp \
           source/glbinding/source/Meta_BitfieldsByString.cpp \
           source/glbinding/source/Meta_BooleansByString.cpp \
           source/glbinding/source/Meta_EnumsByString.cpp \
           source/glbinding/source/Meta_ExtensionsByFunctionString.cpp \
           source/glbinding/source/Meta_ExtensionsByString.cpp \
           source/glbinding/source/Meta_FunctionStringsByExtension.cpp \
           source/glbinding/source/Meta_getStringByBitfield.cpp \
           source/glbinding/source/Meta_ReqVersionsByExtension.cpp \
           source/glbinding/source/Meta_StringsByBitfield.cpp \
           source/glbinding/source/Meta_StringsByBoolean.cpp \
           source/glbinding/source/Meta_StringsByEnum.cpp \
           source/glbinding/source/Meta_StringsByExtension.cpp \
           source/glbinding/source/ProcAddress.cpp \
           source/glbinding/source/Value.cpp \
           source/glbinding/source/Version.cpp \
           source/glbinding/source/Version_ValidVersions.cpp \
           source/glbinding/source/gl/functions-patches.cpp \
           source/glbinding/source/gl/functions_a.cpp \
           source/glbinding/source/gl/functions_b.cpp \
           source/glbinding/source/gl/functions_c.cpp \
           source/glbinding/source/gl/functions_d.cpp \
           source/glbinding/source/gl/functions_e.cpp \
           source/glbinding/source/gl/functions_f.cpp \
           source/glbinding/source/gl/functions_g.cpp \
           source/glbinding/source/gl/functions_h.cpp \
           source/glbinding/source/gl/functions_i.cpp \
           source/glbinding/source/gl/functions_l.cpp \
           source/glbinding/source/gl/functions_m.cpp \
           source/glbinding/source/gl/functions_n.cpp \
           source/glbinding/source/gl/functions_o.cpp \
           source/glbinding/source/gl/functions_p.cpp \
           source/glbinding/source/gl/functions_q.cpp \
           source/glbinding/source/gl/functions_r.cpp \
           source/glbinding/source/gl/functions_s.cpp \
           source/glbinding/source/gl/functions_t.cpp \
           source/glbinding/source/gl/functions_u.cpp \
           source/glbinding/source/gl/functions_v.cpp \
           source/glbinding/source/gl/functions_w.cpp \
           source/glbinding/source/gl/types.cpp

DESTDIR = ../../lib

include(../../library.pri)
