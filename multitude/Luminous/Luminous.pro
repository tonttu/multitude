include(../multitude.pri)
HEADERS += BGThread.hpp \
    ContextVariable.hpp \
    ContextVariables.hpp \
    ContextVariableImpl.hpp \
    Shader.hpp \
    ImageCodecSVG.hpp
HEADERS += CodecRegistry.hpp
HEADERS += Collectable.hpp
HEADERS += CPUMipmaps.hpp
HEADERS += CPUMipmapStore.hpp
HEADERS += DynamicTexture.hpp
HEADERS += EnableStep.hpp
HEADERS += Error.hpp
HEADERS += Export.hpp
HEADERS += FramebufferObject.hpp
HEADERS += GarbageCollector.hpp
HEADERS += GLKeyStone.hpp
HEADERS += GLResource.hpp
HEADERS += GLResources.hpp
HEADERS += GLSLProgramObject.hpp
HEADERS += GLSLShaderObject.hpp
HEADERS += GPUMipmaps.hpp
HEADERS += ImageCodec.hpp
HEADERS += ImageCodecTGA.hpp
HEADERS += Image.hpp
HEADERS += ImagePyramid.hpp
HEADERS += Luminous.hpp
HEADERS += MatrixStep.hpp
HEADERS += MultiHead.hpp
HEADERS += Path.hpp
HEADERS += PixelFormat.hpp
HEADERS += RenderContext.hpp
HEADERS += Task.hpp
HEADERS += TCBSpline.hpp
HEADERS += Texture.hpp
HEADERS += Transformer.hpp
HEADERS += Utils.hpp
HEADERS += VertexBuffer.hpp
HEADERS += VertexBufferImpl.hpp
SOURCES += BGThread.cpp \
    ContextVariable.cpp \
    Shader.cpp \
    ImageCodecSVG.cpp
SOURCES += CodecRegistry.cpp
SOURCES += Collectable.cpp
SOURCES += CPUMipmaps.cpp
SOURCES += CPUMipmapStore.cpp
SOURCES += DynamicTexture.cpp
SOURCES += Error.cpp
SOURCES += FramebufferObject.cpp
SOURCES += GarbageCollector.cpp
SOURCES += GLKeyStone.cpp
SOURCES += GLResource.cpp
SOURCES += GLResources.cpp
SOURCES += GLSLProgramObject.cpp
SOURCES += GLSLShaderObject.cpp
SOURCES += GPUMipmaps.cpp
SOURCES += ImageCodecTGA.cpp
SOURCES += Image.cpp
SOURCES += Luminous.cpp
SOURCES += MultiHead.cpp
SOURCES += Path.cpp
SOURCES += PixelFormat.cpp
SOURCES += RenderContext.cpp
SOURCES += Task.cpp
SOURCES += TCBSpline.cpp
SOURCES += Texture.cpp
SOURCES += Transformer.cpp
SOURCES += Utils.cpp
SOURCES += VertexBuffer.cpp
LIBS += $$LIB_RADIANT \
    $$LIB_OPENGL \
    $$LIB_VALUABLE \
    $$LIB_GLU \
    $$LIB_NIMBLE \
    $$LIB_PATTERNS \
    $$LIB_GLEW
unix { 
    LIBS += -ljpeg \
        -lpng
    !contains(HAS_QT_45,YES) { 
        HEADERS += ImageCodecPNG.hpp
        HEADERS += ImageCodecTGA.hpp
        SOURCES += ImageCodecJPEG.cpp
        SOURCES += ImageCodecPNG.cpp
    }
}
win32:DEFINES += LUMINOUS_EXPORT
contains(HAS_QT_45,YES) { 
    message(Including QT Image codecs)
    HEADERS += ImageCodecQT.hpp
    SOURCES += ImageCodecQT.cpp
    CONFIG += qt
    QT += gui
    QT += svg
    qt_plugin_install.path += /bin
    qt_plugin_install.files = $$[QT_INSTALL_PLUGINS]
    INSTALLS += qt_plugin_install
}
include(../library.pri)
