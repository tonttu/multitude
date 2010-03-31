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

#include "ImageCodecPNG.hpp"
#include "Image.hpp"

#include <png.h>

#include <Radiant/Trace.hpp>

namespace Luminous
{
  ImageCodecPNG::~ImageCodecPNG()
  {}

  bool ImageCodecPNG::canRead(FILE * file)
  {
    long pos = ftell(file);

    char header[8];
    if(fread(header, 1, 8, file) != 8) 
      return false;

    fseek(file, pos, SEEK_SET);

    return png_sig_cmp((png_bytep)header, 0, 8) == 0;  
  }

  std::string ImageCodecPNG::extensions() const
  {
    return std::string("png");
  }

  std::string ImageCodecPNG::name() const
  {
    return std::string("png");
  }

  bool ImageCodecPNG::ping(ImageInfo & info, FILE * file)
  {
    // Skip the header
    fseek(file, 8, SEEK_CUR);

    // Initialize IO stuff
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, png_voidp_NULL, png_error_ptr_NULL, png_error_ptr_NULL);
    if(png_ptr == NULL) {
      Radiant::error("ImageCodecPNG::ping # couldn't create PNG read struct");
      return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == NULL) {
      png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
      Radiant::error("ImageCodecPNG::ping # couldn't create PNG info struct");
      return false;
    }

    if(setjmp(png_jmpbuf(png_ptr))) {
      png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
      Radiant::error("ImageCodecPNG::ping # couldn't set png_jumpbuf");
      return false;
    }

    png_init_io(png_ptr, file);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    int bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    int color_type = png_get_color_type(png_ptr, info_ptr);

    // Convert palette color to RGB
    if(color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(png_ptr);

    // Convert grayscale with less than 8 bpp to 8 bpp
    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) 
      png_set_gray_1_2_4_to_8(png_ptr);

    // Add full alpha channel if there's transparency
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
      png_set_tRNS_to_alpha(png_ptr);

    // If there's more than one pixel per byte, expand to 1 pixel / byte
    if(bit_depth < 8)
      png_set_packing(png_ptr);

    png_read_update_info(png_ptr, info_ptr);

    info.width = png_get_image_width(png_ptr, info_ptr);
    info.height = png_get_image_height(png_ptr, info_ptr);
    int channels = png_get_channels(png_ptr, info_ptr);

    switch(channels) {
      case 4:
        info.pf = PixelFormat(PixelFormat::LAYOUT_RGBA, PixelFormat::TYPE_UBYTE);
        break;
      case 3:
        info.pf = PixelFormat(PixelFormat::LAYOUT_RGB, PixelFormat::TYPE_UBYTE);
        break;
      case 2:
        info.pf = PixelFormat(PixelFormat::LAYOUT_LUMINANCE_ALPHA, PixelFormat::TYPE_UBYTE);
        break;
      case 1:
        info.pf = PixelFormat(PixelFormat::LAYOUT_LUMINANCE, PixelFormat::TYPE_UBYTE);  
        break;
      default:
        Radiant::error("ImageCodecPNG::ping # unsupported number of channels (%d) found", channels);
        return false; 
    };

    png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

    return true;
  }

  bool ImageCodecPNG::read(Image & image, FILE * file)
  {
    // Skip the header
    fseek(file, 8, SEEK_CUR);

    // Initialize IO stuff
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, png_voidp_NULL, png_error_ptr_NULL, png_error_ptr_NULL);
    if(png_ptr == NULL) {
      Radiant::error("ImageCodecPNG::read # couldn't create PNG read struct");
      return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == NULL) {
      png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
      Radiant::error("ImageCodecPNG::read # couldn't create PNG info struct");
      return false;
    }

    if(setjmp(png_jmpbuf(png_ptr))) {
      png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
      Radiant::error("ImageCodecPNG::read # couldn't set png_jumpbuf");
      return false;
    }

    png_init_io(png_ptr, file);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    int bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    int color_type = png_get_color_type(png_ptr, info_ptr);

    // Convert palette color to RGB
    if(color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(png_ptr);

    // Convert grayscale with less than 8 bpp to 8 bpp
    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) 
      png_set_gray_1_2_4_to_8(png_ptr);

    // Add full alpha channel if there's transparency
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
      png_set_tRNS_to_alpha(png_ptr);

    // PNGs support 16 bpp, we don't
    if(bit_depth == 16) {
      Radiant::trace(Radiant::WARNING, "ImageCodecPNG::read # warning, converting 16-bit channels to 8-bit");
      png_set_strip_16(png_ptr);
    }

    // If there's more than one pixel per byte, expand to 1 pixel / byte
    if(bit_depth < 8)
      png_set_packing(png_ptr);

    png_read_update_info(png_ptr, info_ptr);

    int width      = png_get_image_width(png_ptr, info_ptr);
    int height     = png_get_image_height(png_ptr, info_ptr);
    int channels = png_get_channels(png_ptr, info_ptr);
    int rowsize  = width * channels;

    PixelFormat pf;
    switch(channels) {
      case 4:
        pf = PixelFormat(PixelFormat::LAYOUT_RGBA, PixelFormat::TYPE_UBYTE);
        break;
      case 3:
        pf = PixelFormat(PixelFormat::LAYOUT_RGB, PixelFormat::TYPE_UBYTE);
        break;
      case 2:
        pf = PixelFormat(PixelFormat::LAYOUT_LUMINANCE_ALPHA, PixelFormat::TYPE_UBYTE);
        break;
      case 1:
        pf = PixelFormat(PixelFormat::LAYOUT_LUMINANCE, PixelFormat::TYPE_UBYTE);  
        break;
      default:
        Radiant::error("ImageCodecPNG::read # unsupported number of channels (%d) found");
        return false; 
    };

    // Allocate memory
    image.allocate(width, height, pf);

    png_bytep * row_pointers = new png_bytep [height];

    for(int i = 0; i < height; i++)
      row_pointers[i] = image.bytes() + i * rowsize;

    png_read_image(png_ptr, row_pointers);

    delete[] row_pointers;

    png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

    return true;

  }

  bool ImageCodecPNG::write(const Image & image, FILE * file)
  {
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, png_voidp_NULL, png_error_ptr_NULL, png_error_ptr_NULL);
    if(png_ptr == NULL) {
      Radiant::error("ImageCodecPNG::write # couldn't create a PNG write struct");
      return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if(info_ptr == NULL) {
      Radiant::error("ImageCodecPNG::write # couldn't create a PNG info struct");
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return false;
    }

    if(setjmp(png_jmpbuf(png_ptr))) {
      Radiant::error("ImageCodecPNG::write # png_jmpbuf failed");
      png_destroy_write_struct(&png_ptr, &info_ptr);
      return false;
    }

    png_init_io(png_ptr, file);

    int color_type;
    int channels = image.pixelFormat().numChannels();

    switch(channels) {
      case 1:
        color_type = PNG_COLOR_TYPE_GRAY;
        break;
      case 2:
        color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
        break;
      case 3:
        color_type = PNG_COLOR_TYPE_RGB;
        break;
      case 4:
        color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        break;
      default:
        Radiant::error("ImageCodecPNG::write # cannot write a PNG file with (%d).", channels);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return false;
    };

    png_set_IHDR(png_ptr, info_ptr,
        image.width(), image.height(),
        8, // bit depth
        color_type,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT);

    int rowsize = image.width() * channels;
    unsigned char** row_pointers = new unsigned char* [image.height()];

    for(int i = 0; i < image.height(); i++)
      row_pointers[i] = const_cast<unsigned char *> (image.bytes()) + rowsize * i;

    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    delete[] row_pointers;

    png_destroy_write_struct(&png_ptr, &info_ptr);

    return true;
  }

}
