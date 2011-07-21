#ifndef IMAGECODECDDS_HPP
#define IMAGECODECDDS_HPP

#include "ImageCodec.hpp"
#include "PixelFormat.hpp"

#include <Nimble/Vector2.hpp>

#include <vector>

namespace Luminous {

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

  bool writeMipmaps(const std::string & filename, PixelFormat::Compression format,
                    Nimble::Vector2i size, int mipmaps,
                    const std::vector<unsigned char> & dxt);

  static Nimble::Vector2i bufferSize(Nimble::Vector2i size);
  static int linearSize(Nimble::Vector2i size, PixelFormat::Compression format);
};

}

#endif // IMAGECODECDDS_HPP
