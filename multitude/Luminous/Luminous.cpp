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
#include "ImageCodecSVG.hpp"
#include "ImageCodecDDS.hpp"
#include "ImageCodecQT.hpp"
#include "ImageCodecCS.hpp"

#include <QImageWriter>
#include <QImageReader>
#include <QCoreApplication>

#include <Radiant/Condition.hpp>
#include <Radiant/Trace.hpp>

#include <QString>
#include <QLibrary>

namespace Luminous
{
  Radiant::Mutex s_glVersionMutex;
  OpenGLVersion s_glVersion;

  static bool s_luminousInitialized = false;

  void initLuminous()
  {
    // Only run this function once. First from simpleInit then later from
    // RenderThread if the first run fails.
    initDefaultImageCodecs();

    // Enable nvidia graphics card on systems with both Intel and Nvidia graphics cards.
    // See: http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
    // Cleaner way would be to export a symbol NvOptimusEnablement, but that
    // doesn't work if done from a shared or static library. Another way is to
    // link to some of the nvidia driver libraries, but in release mode Visual
    // Studio seems to remove unused libraries. Best option is to dynamically link
    // to nvapi64.dll.
    // This will leak and keep the library open until the application exists.
    // See #12580
    QLibrary nvapi("nvapi64");
    nvapi.load();

    s_luminousInitialized = true;
  }

  bool isLuminousInitialized()
  {
    return s_luminousInitialized;
  }

  OpenGLVersion glVersion()
  {
    Radiant::Guard g(s_glVersionMutex);
    return s_glVersion;
  }

  void initDefaultImageCodecs()
  {
    MULTI_ONCE {

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

#if !defined(RADIANT_IOS)
      // Image::codecs()->registerCodec(new ImageCodecSVG());
      Image::codecs()->registerCodec(std::make_shared<ImageCodecSVG>());
      // Qt5 added support for DDS, but we don't want to use that
      Image::codecs()->registerCodec(std::make_shared<ImageCodecDDS>());
#endif

      QList<QByteArray> formats = QImageWriter::supportedImageFormats ();
      for(QList<QByteArray>::iterator it = formats.begin(); it != formats.end(); ++it) {
        QByteArray & format = (*it);
        Image::codecs()->registerCodec(std::make_shared<ImageCodecQT>(format.data()));
      }

      Image::codecs()->registerCodec(std::make_shared<ImageCodecQT>("jpg"));

      Image::codecs()->registerCodec(std::make_shared<ImageCodecCS>());

    } // MULTI_ONCE
  }

  void initOpenGL(OpenGLAPI& opengl)
  {
    const char * glvendor = (const char *) opengl.glGetString(GL_VENDOR);
    const char * glver = (const char *) opengl.glGetString(GL_VERSION);
    const char * glsl = (char *)opengl.glGetString(GL_SHADING_LANGUAGE_VERSION);
    const char * renderer = (const char *) opengl.glGetString(GL_RENDERER);

    bool printVersion = false;
    // Store the OpenGL information so it can be included in breakpad reports
    {
      Radiant::Guard g(s_glVersionMutex);
      OpenGLVersion oldGlVersion = s_glVersion;
      s_glVersion.vendor = glvendor ? QByteArray(glvendor) : QByteArray();
      s_glVersion.version = glver ? QByteArray(glver) : QByteArray();
      s_glVersion.glsl = glsl ? QByteArray(glsl) : QByteArray();
      s_glVersion.renderer = renderer ? QByteArray(renderer) : QByteArray();
      printVersion = oldGlVersion != s_glVersion;
    }

    if (printVersion)
      Radiant::info("OpenGL vendor: %s, Version: %s, Renderer: %s, GLSL: %s", glvendor, glver, renderer, glsl);
  }
}
