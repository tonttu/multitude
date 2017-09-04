/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ImageCodecSVG.hpp"

#include <Luminous/Image.hpp>

#include <cstdint>

#include <QFile>
#include <QImageReader>
#include <QXmlStreamReader>
#include <QPainter>

#include <QSvgRenderer>

namespace Luminous
{
  std::unique_ptr<QSvgRenderer> createRenderer(QFile & file)
  {
    qint64 old = file.pos();
    QXmlStreamReader reader(&file);
    std::unique_ptr<QSvgRenderer> renderer(new QSvgRenderer(&reader));
    file.seek(old);
    return renderer;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ImageCodecSVG::ImageCodecSVG()
  {

  }

  ImageCodecSVG::~ImageCodecSVG()
  {}

  bool ImageCodecSVG::canRead(QFile & file)
  {
    auto old = file.pos();
    QXmlStreamReader reader(&file);
    QXmlStreamReader::TokenType token;

    // sniff the start element
    bool seems_valid = false;
    do {
      token = reader.readNext();
      if (token == QXmlStreamReader::StartElement) {
        if (reader.name().compare(QByteArray("svg"), Qt::CaseInsensitive) == 0) {
          seems_valid = true;
          break;
        }
      }
    } while (token != QXmlStreamReader::Invalid);

    file.seek(old);
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

  bool ImageCodecSVG::ping(ImageInfo & info, QFile & file)
  {
    std::unique_ptr<QSvgRenderer> r = createRenderer(file);

    if (!r || !r->isValid() || r->defaultSize().isEmpty()) {
      return false;
    }

    // always this
    info.pf = PixelFormat::rgbaUByte();

    info.width = r->defaultSize().width();
    info.height = r->defaultSize().height();

    return true;
  }

  bool ImageCodecSVG::read(Image & image, QFile & file)
  {
    std::unique_ptr<QSvgRenderer> r = createRenderer(file);

    if (!r || !r->isValid() || r->defaultSize().isEmpty()) {
      return false;
    }

    int width = r->defaultSize().width();
    int height = r->defaultSize().height();
    QImage img(width, height, QImage::Format_ARGB32);

    // This might happen if the image size is too big
    if (img.isNull()) {
      return false;
    }

    image.allocate(width, height, PixelFormat::rgbaUByte());
    img.fill(0x00000000);
    QPainter painter(&img);
    r->render(&painter);
    painter.end();

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
  bool ImageCodecSVG::write(const Image & /*image*/, QFile & /*file*/)
  {
    return false;
  }

} // namespace Luminous
