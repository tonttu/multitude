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
#include "ImageCodecDDS.hpp"
#include "ImageCodecQT.hpp"
#include "ImageCodecSVG.hpp"


#if defined(USE_QT45) && !defined(RADIANT_IOS)
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
#ifdef LUMINOUS_OPENGLES
  void dumymWarn(const char * funcname, const char * file, int line)
  {
    Radiant::error("Unimplemented OpenGL call: %s in %s:%d", funcname, file, line);
  }
  int dummyEnum(const char * file, int line)
  {
    Radiant::error("Unimplemented OpenGL call: %s:%d", file, line);
  }

#endif

  bool initLuminous(bool initOpenGL)
  {
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
    }

    return true;
  }

  void initDefaultImageCodecs()
  {
    static bool done = false;

    if(done)
      return;

    done = true;

#ifdef WIN32
    // Make sure Qt plugins are found
    /// @todo does this work when the SDK is not installed? Where does find the plugins?
    char* dir = getenv("CORNERSTONE_ROOT");
    std::string pluginPath;
    if(dir) {
      pluginPath = std::string(dir) + std::string("\\bin\\plugins");
    } else {
      pluginPath = std::string("..\\lib\\Plugins");
    }
    QCoreApplication::addLibraryPath(pluginPath.c_str());
#endif

#if defined(USE_QT45) && !defined(RADIANT_IOS)
    // Debug output supported image formats
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

    QList<QByteArray> formats = QImageWriter::supportedImageFormats ();
    for(QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); it++) {
      QByteArray & format = (*it);
      Image::codecs()->registerCodec(new ImageCodecQT(format.data()));
    }
    Image::codecs()->registerCodec(new ImageCodecQT("jpg"));
    Image::codecs()->registerCodec(new ImageCodecDDS());
    Image::codecs()->registerCodec(new ImageCodecSVG());
    Image::codecs()->registerCodec(new ImageCodecSVG());
#else
    // Register built-in image codecs
    Image::codecs()->registerCodec(new ImageCodecJPEG());
    Image::codecs()->registerCodec(new ImageCodecPNG());
#endif

    /* TGA has to be last, because its ping may return true even if
       the file has other type. */

    LUMINOUS_IN_FULL_OPENGL(Image::codecs()->registerCodec(new ImageCodecTGA()));

  }

}
