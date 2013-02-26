/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "ImageCodecSVG.hpp"

#include <Luminous/Image.hpp>

#include <cstdint>

#include <QFile>
#include <QImageReader>
#include <QXmlStreamReader>
#include <QPainter>

/* On Mac OS X, with Qt 4.7.0, it seems that QT_NO_SVGRENDERER gets defined by
   someone. Lets do our best to undefine it.   */
#ifdef QT_NO_SVGRENDERER
# undef QT_NO_SVGRENDERER
#endif

#include <QSvgRenderer>

namespace Luminous {

  ImageCodecSVG::ImageCodecSVG()
  {

  }

  ImageCodecSVG::~ImageCodecSVG()
  {}

  bool ImageCodecSVG::canRead(FILE * file)
  {
    QFile f;
    qint64 old = f.pos();
    f.open(file, QIODevice::ReadOnly);
    QXmlStreamReader reader(&f);
    QXmlStreamReader::TokenType token;

    // sniff the start element
    bool seems_valid = false;
    do {
      token = reader.readNext();
      if (token == QXmlStreamReader::StartElement) {
        if (reader.name().compare("svg", Qt::CaseInsensitive) == 0) {
          seems_valid = true;
          break;
        }
      }
    } while (token != QXmlStreamReader::Invalid);

    f.seek(old);
    return seems_valid;
  }

  QString ImageCodecSVG::extensions() const
  {
    return QString("svg");
  }

  QString ImageCodecSVG::name() const
  {
    return QString("svg");
  }

  bool ImageCodecSVG::ping(ImageInfo & info, FILE * file)
  {

    QSvgRenderer * r = updateSVG(file);
    // always this
    info.pf = PixelFormat::rgbaUByte();

    info.width = r->defaultSize().width();
    info.height = r->defaultSize().height();
    delete r;
    return true;
  }

  bool ImageCodecSVG::read(Image & image, FILE * file)
  {
    QSvgRenderer * r = updateSVG(file);

    int width = r->defaultSize().width();
    int height = r->defaultSize().height();
    image.allocate(width, height, PixelFormat::rgbaUByte());

    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(0x00000000);
    QPainter painter(&img);
    r->render(&painter);
    painter.end();

    delete r;

    const uint8_t * src = img.bits();
    const uint8_t * last = img.bits() + 4*img.width() * img.height();
    uint8_t * dest = image.data();
    // ARGB -> RGBA
    while (src < last) {
      dest[0] = src[2];
      dest[1] = src[1];
      dest[2] = src[0];
      dest[3] = src[3];
      src += 4;
      dest += 4;
    }
    return true;
  }

  /// Not supported (could use QSvgGenerator to render bitmap as SVG, but why?)
  bool ImageCodecSVG::write(const Image & /*image*/, FILE * /*file*/)
  {
    return false;
  }

  QSvgRenderer * ImageCodecSVG::updateSVG(FILE * file)
  {
    QFile qFile;
    qFile.open(file, QIODevice::ReadOnly);
    qint64 old = qFile.pos();
    QXmlStreamReader reader(&qFile);
    QSvgRenderer * renderer = new QSvgRenderer(&reader);
    qFile.seek(old);
    return renderer;
  }

} // namespace Luminous
