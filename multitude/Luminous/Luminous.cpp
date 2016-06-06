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

namespace Luminous
{
  Radiant::Mutex s_glVersionMutex;
  OpenGLVersion s_glVersion;

  bool isSampleShadingSupported()
  {
#if defined(RADIANT_OSX_YOSEMITE) || defined(RADIANT_OSX_EL_CAPITAN)
    return true;
#elif defined(RADIANT_OSX_MOUNTAIN_LION)
    return false;
#else
    static bool s_supported = glewIsSupported("GL_ARB_sample_shading");
    return s_supported;
#endif
  }

  static bool s_luminousInitialized = false;
  static Radiant::Mutex s_glbindingMutex;
  static Radiant::Condition s_glbindingBarrier;
  static int s_glbindingWaitingThreadCount = -1;

  void initLuminous()
  {
    // Only run this function once. First from simpleInit then later from
    // RenderThread if the first run fails.
    initDefaultImageCodecs();

    s_luminousInitialized = true;
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
