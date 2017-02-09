/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ImageCodecCS.hpp"
#include "Image.hpp"

#include <Radiant/Directory.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/BinaryData.hpp>

#include <QFile>

namespace Luminous {

bool ImageCodecCS::canRead(QFile & file)
{  
  ImageInfo info;
  return ping(info, file);
}

QString ImageCodecCS::extensions() const
{
  return "csimg";
}

QString ImageCodecCS::name() const
{
  return "CS";
}

bool ImageCodecCS::ping(ImageInfo & info, QFile & file)
{  
  Radiant::BinaryData bd;
  auto original_pos = file.pos();

  int offset = 0;
  file.read((char*)&offset, sizeof(offset));

  QByteArray magic = file.peek(4);

  // Hide warning messages from QByteArray constructor and bd.readString by
  // checking for string marker manually
  if (magic.size() != 4 || *reinterpret_cast<uint32_t*>(magic.data()) !=
      Radiant::BinaryData::STRING_MARKER) {
    return false;
  }

  QByteArray buffer = file.read(offset);
  bd.linkTo(buffer.data(), buffer.size());
  bd.setTotal(buffer.size());

  file.seek(original_pos);

  QString hdr;
  if (bd.readString(hdr) && hdr != "cornerstone img") {
    Radiant::warning("loadImage # header error: '%s'", hdr.toUtf8().data());
    return false;
  }

  bool ok = true;

  int version = bd.readInt32();
  (void)version;
  int compression = bd.readInt32();
  (void)compression;
  int width = bd.readInt32();
  int height = bd.readInt32();
  int layout = bd.readInt32();
  int type = bd.readInt32();
  int dataSize = bd.readInt32(&ok);
  (void)dataSize;

  if(!ok) return false;

  info.height = height;
  info.width = width;
  info.pf = Luminous::PixelFormat(Luminous::PixelFormat::ChannelLayout(layout),
                                                        Luminous::PixelFormat::ChannelType(type));

  return true;
}

bool ImageCodecCS::read(Image & image, QFile & file)
{
  Radiant::BinaryData bd;
  int offset = 0;
  file.read((char*)&offset, sizeof(offset));

  QByteArray buffer = file.read(offset);
  bd.linkTo(buffer.data(), buffer.size());
  bd.setTotal(buffer.size());

  QString hdr;
  if (bd.readString(hdr) && hdr != "cornerstone img") {
    Radiant::warning("loadImage # header error: '%s'", hdr.toUtf8().data());
    return false;
  }

  int version = bd.readInt32();
  (void)version;
  int compression = bd.readInt32();
  int width = bd.readInt32();
  int height = bd.readInt32();
  int layout = bd.readInt32();
  int type = bd.readInt32();
  int dataSize = bd.readInt32();

  image.allocate(width, height, Luminous::PixelFormat(Luminous::PixelFormat::ChannelLayout(layout),
                                                      Luminous::PixelFormat::ChannelType(type)));
  const int size = image.lineSize() * image.height();

  if (compression == 0) {
    return size == dataSize && size == file.read((char*)image.data(), size);
  } else {
    QByteArray data = qUncompress(file.read(dataSize));
    if (data.size() != size) {
      Radiant::warning("loadImage # uncompressed data size: %d (should be %d)", data.size(), size);
      return false;
    }
    memcpy(image.data(), data.data(), size);
    return true;
  }
}

bool ImageCodecCS::write(const Image & image, QFile & file)
{
  Radiant::BinaryData bd;
  bd.write("cornerstone img");
  bd.writeInt32(0); // version
  bd.writeInt32(1); // compression
  bd.writeInt32(image.width());
  bd.writeInt32(image.height());
  bd.writeInt32(image.pixelFormat().layout());
  bd.writeInt32(image.pixelFormat().type());

  QByteArray data = qCompress(image.data(), image.lineSize() * image.height());
  bd.writeInt32(data.size());

  const int offset = bd.pos();
  file.write((const char*)&offset, sizeof(offset));
  qint64 total = file.write(bd.data(), offset);
  total += file.write(data);
  return total == offset + data.size();
}

}
