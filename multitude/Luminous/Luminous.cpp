 /* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#include "Luminous.hpp"
#include "Image.hpp"
#include "CodecRegistry.hpp"


#include "ImageCodecTGA.hpp"
#include "ImageCodecQT.hpp"
#ifndef LUMINOUS_OPENGLES
#include "ImageCodecSVG.hpp"
#include "CPUMipmaps.hpp"

#include "ImageCodecDDS.hpp"
#endif // LUMINOUS_OPENGLES

#include <Luminous/ImageCodecQT.hpp>
#include <QImageWriter>
#include <QImageReader>
#include <QCoreApplication>

#include <Radiant/Trace.hpp>

#include <QString>

namespace Luminous
{
  /// @todo these are related to DummyOpenGL and should not be here!
  void dumymWarn(const char * funcname, const char * file, int line)
  {
    Radiant::error("Unimplemented OpenGL call: %s in %s:%d", funcname, file, line);
  }
  int dummyEnum(const char * file, int line)
  {
    Radiant::error("Unimplemented OpenGL call: %s:%d", file, line);
    return 0;
  }

  bool sampleShadingSupported()
  {
    static bool s_supported = /*GLEW_VERSION_4_0 ||*/ glewIsSupported("GL_ARB_sample_shading");
    return s_supported;
  }

  bool initLuminous(bool initOpenGL)
  {
    // Only run this function once. First from simpleInit then later from
    // RenderThread if the first run fails.
    initDefaultImageCodecs();

    if (initOpenGL) {
      static bool s_ok = true;
      MULTI_ONCE {

        const char * glvendor = (const char *) glGetString(GL_VENDOR);
        const char * glver = (const char *) glGetString(GL_VERSION);

#ifndef MULTI_WITHOUT_GLEW
        GLenum err = glewInit();

        if(err != GLEW_OK) {
          Radiant::error("Failed to initialize GLEW: %s", glewGetErrorString(err));
          s_ok = false;
          return false;
        }

        // Check for DXT support
        bool dxtSupport = glewIsSupported("GL_EXT_texture_compression_s3tc");
        Radiant::info("Hardware DXT texture compression support: %s", dxtSupport ? "yes" : "no");
        Luminous::CPUMipmaps::s_dxtSupported = dxtSupport;

        const char * glsl = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
        if (glsl) {
          Radiant::info("GLSL: %s", glsl);
        } else {
          Radiant::error("GLSL not supported");
          s_ok = false;
        }
        Radiant::info("OpenGL vendor: %s (OpenGL version: %s)", glvendor, glver);

        if (!sampleShadingSupported()) {
          Radiant::warning("OpenGL 4.0 or GL_ARB_sample_shading not supported by this computer, "
                           "some multisampling features will be disabled");
          // this is only a warning, no need to set s_ok to false
        }

        if (!GLEW_VERSION_3_1 && !glewIsSupported("GL_ARB_uniform_buffer_object")) {
          Radiant::error("OpenGL 3.1 or GL_ARB_uniform_buffer_object not supported by this computer");
          /// @todo If we have the extension with older OpenGL, can we call
          ///       BindBufferRange etc or should we call ARB/EXT -versions of those functions?
          s_ok = false;
        }

#else
        Radiant::info("OpenGL without GLEW # %s : %s", glvendor, glver);
#endif
      } // MULTI_ONCE
      return s_ok;
    }

    return true;
  }

  void initDefaultImageCodecs()
  {
    MULTI_ONCE {

#ifdef WIN32
      // Make sure Qt plugins are found
      /// @todo does this work when the SDK is not installed? Where does find the plugins?
      /// @todo should somehow use MultiTouch::cornerstoneRoot(),
      ///       maybe give it as a parameter to this function
      char* dir = getenv("CORNERSTONE_ROOT");
      std::string pluginPath;
      if(dir) {
        pluginPath = std::string(dir) + std::string("\\bin\\plugins");
      } else {
        pluginPath = std::string("..\\lib\\plugins");
      }
      QCoreApplication::addLibraryPath(pluginPath.c_str());
#endif

      {
        debugLuminous("Qt image support (read):");
        QList<QByteArray> formats = QImageReader::supportedImageFormats ();
        for(QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); ++it) {
          QString format(*it);
          debugLuminous("%s", format.toUtf8().data());
        }
      }

      {
        debugLuminous("Qt image support (write):");
        QList<QByteArray> formats = QImageWriter::supportedImageFormats ();
        for(QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); ++it) {
          QString format(*it);
          debugLuminous("%s", format.toUtf8().data());
        }
      }

      /// ImageCodecTGA supports some pixel formats that Qt doesn't support, like
      /// Luminuos::PixelFormat::redUByte(). Give this codec a priority
      Image::codecs()->registerCodec(std::make_shared<ImageCodecTGA>());

      QList<QByteArray> formats = QImageWriter::supportedImageFormats ();
      for(QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); ++it) {
        QByteArray & format = (*it);
        Image::codecs()->registerCodec(std::make_shared<ImageCodecQT>(format.data()));
      }

      Image::codecs()->registerCodec(std::make_shared<ImageCodecQT>("jpg"));

#if !defined(RADIANT_IOS)
      // Image::codecs()->registerCodec(new ImageCodecSVG());
      Image::codecs()->registerCodec(std::make_shared<ImageCodecSVG>());
      Image::codecs()->registerCodec(std::make_shared<ImageCodecDDS>());
#endif

    } // MULTI_ONCE
  }

}
