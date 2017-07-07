/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef LUMINOUS_IMAGE_CODEC_CS_HPP
#define LUMINOUS_IMAGE_CODEC_CS_HPP

#include "ImageCodec.hpp"
#include "PixelFormat.hpp"

#include <Nimble/Vector2.hpp>

#include <vector>

namespace Luminous
{
  /**
   * Image codec for lossless Cornerstone image format (*.csimg).
   *
   * This format is significantly faster to compress and decompress than PNG,
   * supports more pixel formats than Qt image codecs (for instance floating
   * point images and more than 8 bit images) and supports images with
   * premultiplied alpha.
   *
   * Files are typically bigger than PNG images. Performance test image set
   * mipmaps are 14 GB in csimg format and 8.3 GB in PNG format.
   *
   * File format description:
   * The file is a binary file that has three parts:
   *  - Header size in bytes: int32
   *  - Header: Radiant::BinaryData object
   *  - Image data
   *
   * The header has the following Radiant::BinaryData fields:
   *  - magic: String - "cornerstone img"
   *  - version: int32 - 0 or 1
   *  - compression: int32 - see ImageCodecCS::Compression
   *  - image width: int32
   *  - image height: int32
   *  - pixel format layout: int32 - see Luminous::PixelFormat::layout
   *  - pixel format type: int32 - see Luminous::PixelFormat::type
   *  - image data size in bytes: int32
   * New fields in version 1:
   *  - pixel format compression: int32 - see Luminous::PixelFormat::compression
   *  - flags: int32 - See ImageCodecCS::Flags
   */
  class ImageCodecCS : public ImageCodec
  {
  public:
    enum Compression
    {
      NO_COMPRESSION   = 0,
      COMPRESSION_ZLIB = 1, ///< Default in version 0
      COMPRESSION_LZ4  = 2, ///< Default in version 1
    };

    /// Added in version 1
    enum Flags
    {
      NO_FLAGS                 = 0,
      FLAG_PREMULTIPLIED_ALPHA = 1 << 0,
    };

  public:
    bool canRead(QFile & file) override;
    QString extensions() const override;
    QString name() const override;
    bool ping(ImageInfo & info, QFile & file) override;
    bool read(Image & image, QFile & file) override;
    bool write(const Image & image, QFile & file) override;
    bool canWritePremultipliedAlpha() const override { return true; }
  };
}

#endif // LUMINOUS_IMAGE_CODEC_CS_HPP
