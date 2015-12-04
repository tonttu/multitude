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
#include "GPUAssociation.hpp"

#include <QImageWriter>
#include <QImageReader>
#include <QCoreApplication>

#include <Radiant/Trace.hpp>

#include <QString>

namespace Luminous
{
  bool isSampleShadingSupported()
  {
#ifdef RADIANT_OSX
    return true;
#else
    static bool s_supported = glewIsSupported("GL_ARB_sample_shading");
    return s_supported;
#endif
  }

  static bool s_luminousInitialized = false;

  bool initLuminous(bool initOpenGL)
  {
    // Only run this function once. First from simpleInit then later from
    // RenderThread if the first run fails.
    initDefaultImageCodecs();

    if (initOpenGL) {
      static bool s_ok = true;
      MULTI_ONCE {

#ifndef RADIANT_OSX
        GLenum err = glewInit();

        if(err != GLEW_OK) {
          Radiant::error("Failed to initialize GLEW: %s", glewGetErrorString(err));
          s_ok = false;
          return false;
        }


        if(Luminous::GPUAssociation::isSupported()) {
          unsigned int gpuCount = Luminous::GPUAssociation::numGPUs();
          Radiant::info("Available GPUs: %u", gpuCount);
        }

        // Check for DXT support
        bool dxtSupport = glewIsSupported("GL_EXT_texture_compression_s3tc");
        Radiant::info("Hardware DXT texture compression support: %s", dxtSupport ? "yes" : "no");


        if (!glewIsSupported("GL_ARB_sample_shading")) {
          Radiant::warning("OpenGL 4.0 or GL_ARB_sample_shading not supported by this computer, "
                           "some multi-sampling features will be disabled.");
          // This is only a warning, no need to set s_ok to false
        }

        if (!GLEW_VERSION_3_1 && !glewIsSupported("GL_ARB_uniform_buffer_object")) {
          Radiant::error("OpenGL 3.1 or GL_ARB_uniform_buffer_object not supported by this computer");
          /// @todo If we have the extension with older OpenGL, can we call
          ///       BindBufferRange etc or should we call ARB/EXT -versions of those functions?
          s_ok = false;
        }
#endif
        const char * glvendor = (const char *) glGetString(GL_VENDOR);
        const char * glver = (const char *) glGetString(GL_VERSION);
        const char * glsl = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);

        Radiant::info("OpenGL vendor: %s (OpenGL version: %s)", glvendor, glver);

        if (glsl) {
          Radiant::info("GLSL: %s", glsl);
        } else {
          Radiant::error("GLSL not supported");
          s_ok = false;
        }

      } // MULTI_ONCE
      if(s_ok)
        s_luminousInitialized = true;
      return s_ok;
    }

    s_luminousInitialized = true;

    return true;
  }

  bool isLuminousInitialized()
  {
    return s_luminousInitialized;
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

}
