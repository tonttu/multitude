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

#include <Luminous/Luminous.hpp>
#include <Luminous/Image.hpp>
#include <Luminous/CodecRegistry.hpp>
#include <Luminous/ImageCodecTGA.hpp>


#if defined(USE_QT45) && !defined(RADIANT_IOS)
#include <Luminous/ImageCodecQT.hpp>
#include <QImageWriter>
#include <QImageReader>
#else
#include <Luminous/ImageCodecPNG.hpp>
#include <Luminous/ImageCodecJPEG.hpp>
#endif // USE_QT45

#include <Luminous/ImageCodecSVG.hpp>

#include <Radiant/Trace.hpp>

#include <string>
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
      else if(GLEW_VERSION_1_5) versionMsg << std::string("OpenGL 1.5 supported");
      else if(GLEW_VERSION_1_4) versionMsg << std::string("OpenGL 1.4 supported");
      else if(GLEW_VERSION_1_3) versionMsg << std::string("OpenGL 1.3 supported");
      else if(GLEW_VERSION_1_2) versionMsg << std::string("OpenGL 1.2 supported");
      else if(GLEW_VERSION_1_1) versionMsg << std::string("OpenGL 1.1 supported");

      char * glsl = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
      std::string glslMsg = (glsl ? glsl : "GLSL not supported");

      Radiant::info("%s (%s)", versionMsg.str().c_str(), glslMsg.c_str());
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

#if defined(USE_QT45) && !defined(RADIANT_IOS)
    // Debug output supported image formats
    {
      Radiant::debug("Qt image support (read):");
      QList<QByteArray> formats = QImageReader::supportedImageFormats ();
      for(QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); it++) {
        QString format(*it);
        Radiant::debug("%s", format.toStdString().c_str());
      }
    }

    {
      Radiant::debug("Qt image support (write):");
      QList<QByteArray> formats = QImageWriter::supportedImageFormats ();
      for(QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); it++) {
        QString format(*it);
        Radiant::debug("%s", format.toStdString().c_str());
      }
    }

    QList<QByteArray> formats = QImageWriter::supportedImageFormats ();
    for(QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); it++) {
      QByteArray & format = (*it);
      Image::codecs()->registerCodec(new ImageCodecQT(format.data()));
    }
    Image::codecs()->registerCodec(new ImageCodecQT("jpg"));
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
