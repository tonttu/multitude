#ifndef IMAGECODECDDS_HPP
#define IMAGECODECDDS_HPP

#include "ImageCodec.hpp"
#include "PixelFormat.hpp"

#include <Nimble/Vector2.hpp>

#include <vector>

namespace Luminous {

/// Image codec to handle DDS files
class ImageCodecDDS : public ImageCodec
{
public:
  ImageCodecDDS();
  bool canRead(FILE * file);
  std::string extensions() const;
  std::string name() const;
  bool ping(ImageInfo & info, FILE * file);
  bool read(Image & image, FILE * file);
  bool write(const Image & image, FILE * file);
  bool read(CompressedImage & image, FILE * file, int level = 0);

  /// Save the DXT compressed image data to a DDS file.
  /// @param filename file to save to
  /// @param format compression format
  /// @param size size of the image in pixels
  /// @param mipmaps number of mipmap levels in the data
  /// @param dxt compressed image data
  /// @return true if the writing succeeded
  bool writeMipmaps(const std::string & filename, PixelFormat::Compression format,
                    Nimble::Vector2i size, int mipmaps,
                    const std::vector<unsigned char> & dxt);

  /// Gets the required buffer size in pixels for the DDS image. DXT
  /// compression is done in 4x4 blocks. This function gets the required image
  /// size in pixels by rounding the size up to nearest one divisible by four.
  /// @param size size of the original image
  /// @return size required for the DXT compression
  static Nimble::Vector2i bufferSize(Nimble::Vector2i size);

  /// Gets the size of a single line in bytes.
  /// @param size image dimensions
  /// @param format compression format used
  /// @return size of a line in bytes
  static int linearSize(Nimble::Vector2i size, PixelFormat::Compression format);
};

}

#endif // IMAGECODECDDS_HPP
