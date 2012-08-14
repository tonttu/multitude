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

#if defined(USE_QT45)
//&& !defined(RADIANT_IOS)
#include <Luminous/ImageCodecQT.hpp>
#include <QImageWriter>
#include <QImageReader>
#include <QCoreApplication>
#endif

#include <Radiant/Trace.hpp>

#include <QString>
#include <sstream>

namespace Luminous
{
  using namespace Radiant;

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

  bool initLuminous(bool initOpenGL)
  {
    // Only run this function once. First from simpleInit then later from
    // RenderThread if the first run fails.
    MULTI_ONCE {

      initDefaultImageCodecs();

      if(initOpenGL) {

        const char * glvendor = (const char *) glGetString(GL_VENDOR);
        const char * glver = (const char *) glGetString(GL_VERSION);

#ifndef MULTI_WITHOUT_GLEW
        GLenum err = glewInit();

        std::ostringstream versionMsg;

        if(err != GLEW_OK) {
          Radiant::error("Failed to initialize GLEW: %s", glewGetErrorString(err));
          return false;
        }

        // Check the OpenGL version
        bool warn = true;
        versionMsg << "Luminous initialized: ";

        if(GLEW_VERSION_2_1) {
          warn = false;
          versionMsg << "OpenGL 2.1 supported";
        }
        else if(GLEW_VERSION_2_0) {
          warn = false;
          versionMsg << "OpenGL 2.0 supported";
        }
        else if(GLEW_VERSION_1_5) versionMsg << "OpenGL 1.5 supported";
        else if(GLEW_VERSION_1_4) versionMsg << "OpenGL 1.4 supported";
        else if(GLEW_VERSION_1_3) versionMsg << "OpenGL 1.3 supported";
        else if(GLEW_VERSION_1_2) versionMsg << "OpenGL 1.2 supported";
        else if(GLEW_VERSION_1_1) versionMsg << "OpenGL 1.1 supported";

        char * glsl = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
        const char * glslMsg = (glsl ? glsl : "GLSL not supported");

        Radiant::info("%s (%s)", versionMsg.str().c_str(), glslMsg);
        Radiant::info("%s (%s)", glvendor, glver);

        if(warn) {
          Radiant::error("OpenGL 2.0 is not supported by this computer, "
                         "some applications may fail.");
          return false;
        }
#else
        info("OpenGL without GLEW # %s : %s", glvendor, glver);
#endif

        // Check for DXT support
#ifndef MULTI_WITHOUT_GLEW
        bool dxtSupport = glewIsSupported("GL_EXT_texture_compression_s3tc");
        Radiant::info("Hardware DXT texture compression support: %s", dxtSupport ? "yes" : "no");
        Luminous::CPUMipmaps::s_dxtSupported = dxtSupport;
#endif
      }

    } // MULTI_ONCE

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
        pluginPath = std::string("..\\lib\\Plugins");
      }
      QCoreApplication::addLibraryPath(pluginPath.c_str());
#endif

      {
        debugLuminous("Qt image support (read):");
        QList<QByteArray> formats = QImageReader::supportedImageFormats ();
        for(QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); it++) {
          QString format(*it);
          debugLuminous("%s", format.toUtf8().data());
        }
      }

      {
        debugLuminous("Qt image support (write):");
        QList<QByteArray> formats = QImageWriter::supportedImageFormats ();
        for(QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); it++) {
          QString format(*it);
          debugLuminous("%s", format.toUtf8().data());
        }
      }

      /// ImageCodecTGA supports some pixel formats that Qt doesn't support, like
      /// Luminuos::PixelFormat::redUByte(). Give this codec a priority
      Image::codecs()->registerCodec(std::make_shared<ImageCodecTGA>());

      QList<QByteArray> formats = QImageWriter::supportedImageFormats ();
      for(QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); it++) {
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
