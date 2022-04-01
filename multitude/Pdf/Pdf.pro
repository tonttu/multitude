include(../../cornerstone.pri)

#CONFIG += qt
#QT += gui

DEFINES += PDF_EXPORT

linux-* {
  # Make sure pdfium is available
  !exists(/opt/multitaction-pdfium2):error(multitaction-libpdfium2-dev is required to build PDF support)
  INCLUDEPATH += /opt/multitaction-pdfium2/include
  QMAKE_LIBDIR += /opt/multitaction-pdfium2/lib
  LIBS += -lmultitaction-pdfium2
}

macx {
  INCLUDEPATH += /opt/homebrew/include/pdfium
  LIBS += -lpdfium
}

win32 {
  LIBS += -lpdfium

  # pdfium doesn't have all debug symbols. We don't care
  QMAKE_LFLAGS += /ignore:4099
}

HEADERS += PDFManager.hpp
SOURCES += PDFManager.cpp

enable-luminous {
  DEFINES += ENABLE_LUMINOUS
  LIBS += $$LIB_LUMINOUS
}

LIBS += $$LIB_RADIANT $$LIB_PUNCTUAL $$LIB_FOLLY

include(../../library.pri)
