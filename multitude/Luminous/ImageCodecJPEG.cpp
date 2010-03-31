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

#include "ImageCodecJPEG.hpp"
#include "Image.hpp"

#include <Radiant/Trace.hpp>

extern "C" {
# include <jpeglib.h>
# include <setjmp.h>
}

namespace Luminous
{

  using namespace Radiant;

  // Our own error handler to avoid calling exit() if libjpeg encounters a fatal
  // error.
  struct LuminousErrorMgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmpBuffer;
  };

  // The actual error handling routine
  void luminousErrorExit(j_common_ptr cinfo)
  {
    LuminousErrorMgr * myErr = (LuminousErrorMgr *)cinfo->err;

    // Display the error message
    (*cinfo->err->output_message)(cinfo);

    // Return control to setjmp location instead of calling exit()
    longjmp(myErr->setjmpBuffer, 1);    
  }

  ImageCodecJPEG::~ImageCodecJPEG()
  {}

  bool ImageCodecJPEG::canRead(FILE * file)
  {
    unsigned char header[2];

    long pos = ftell(file);
    if(fread(header, 1, 2, file) != 2)
      return false;

    fseek(file, pos, SEEK_SET);

    return header[0] == 0xFF && header[1] == 0xD8;
  }

  std::string ImageCodecJPEG::extensions() const
  {
    return std::string("jpg jpeg");
  }

  std::string ImageCodecJPEG::name() const
  {
    return std::string("jpeg");
  }

  bool ImageCodecJPEG::ping(ImageInfo & info, FILE * file)
  {
    LuminousErrorMgr jerr;
    struct jpeg_decompress_struct cinfo;

    // Set the standard error handler first, but override the error_exit
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = luminousErrorExit;

    // Setup a return context with setjmp for error_exit to use
    if(setjmp(jerr.setjmpBuffer)) {
      jpeg_destroy_decompress(&cinfo);
      return false;
    }

    // Init decompression object
    jpeg_create_decompress(&cinfo);

    // Set source
    jpeg_stdio_src(&cinfo, file);

    // Read header
    jpeg_read_header(&cinfo, TRUE);

    // Decompress the header
    jpeg_start_decompress(&cinfo);

    info.width = cinfo.output_width;
    info.height = cinfo.output_height;

    if(cinfo.output_components == 1) {
      info.pf = PixelFormat(PixelFormat::LAYOUT_LUMINANCE, PixelFormat::TYPE_UBYTE);
    } else if(cinfo.output_components == 3) {
      info.pf = PixelFormat(PixelFormat::LAYOUT_RGB, PixelFormat::TYPE_UBYTE);
    }

    jpeg_destroy_decompress(&cinfo);

    return true;
  }

  bool ImageCodecJPEG::read(Image & image, FILE * file)
  {
    LuminousErrorMgr jerr;
    struct jpeg_decompress_struct cinfo;

    // Set error handler, override exit_error
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = luminousErrorExit;

    // Setup return context for error_exit
    if(setjmp(jerr.setjmpBuffer)){
      jpeg_destroy_decompress(&cinfo);
      return false;
    }

    // Init decompression object
    jpeg_create_decompress(&cinfo);

    // Set source
    jpeg_stdio_src(&cinfo, file);

    // Read header
    jpeg_read_header(&cinfo, TRUE);

    // Start decompressing
    jpeg_start_decompress(&cinfo);

    PixelFormat::ChannelLayout layout = PixelFormat::LAYOUT_UNKNOWN;

    if(cinfo.output_components == 1) {
      layout = PixelFormat::LAYOUT_LUMINANCE;
    } else if(cinfo.output_components == 3) {
      layout = PixelFormat::LAYOUT_RGB;
    } else if(cinfo.output_components == 4) {
      layout = PixelFormat::LAYOUT_RGBA;
    } else {
      Radiant::error("ImageCodecJPEG::read # Unsupported number of components (%d) found", cinfo.output_components);
      jpeg_destroy_decompress(&cinfo);
      return false;
    }

    int stride = cinfo.output_width * cinfo.output_components;

    // Allocate memory
    image.allocate(cinfo.output_width, cinfo.output_height, PixelFormat(layout, PixelFormat::TYPE_UBYTE));

    while(cinfo.output_scanline < cinfo.output_height) {
      JSAMPROW scanline = &image.bytes()[cinfo.output_scanline * stride];
      jpeg_read_scanlines(&cinfo, &scanline, 1);
    }

    // Finish decompression
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return true;
  }

  bool ImageCodecJPEG::write(const Image & image, FILE * file)
  {
    int quality = 100;

    struct jpeg_error_mgr jerr;
    struct jpeg_compress_struct cinfo;

    // Set error handler first
    cinfo.err = jpeg_std_error(&jerr);

    // Init compression object
    jpeg_create_compress(&cinfo);

    // Set destination
    jpeg_stdio_dest(&cinfo, file);

    // Parameters
    cinfo.image_width = image.width();
    cinfo.image_height = image.height(); 
    cinfo.input_components = image.pixelFormat().numChannels();

    if(cinfo.input_components == 1) {
      cinfo.in_color_space = JCS_GRAYSCALE;
    } else if(cinfo.input_components == 3) {
      cinfo.in_color_space = JCS_RGB;
    } else {
      Radiant::error("ImageCodecJPEG::write # JPEG format only supports GRAYSCALE or RGB images");
      jpeg_destroy_compress(&cinfo);
      return false;
    }

    if(image.pixelFormat().type() != PixelFormat::TYPE_UBYTE) {
      Radiant::error("ImageCodecJPEG::write # JPEG only supports byte-based components");
      jpeg_destroy_compress(&cinfo);
      return false;
    }

    // Default compression parameters
    jpeg_set_defaults(&cinfo);

    // Set quality
    jpeg_set_quality(&cinfo, quality, TRUE);

    // Start compressing
    jpeg_start_compress(&cinfo, TRUE);

    int stride = image.width() * cinfo.input_components;

    // Write scanlines
    while(cinfo.next_scanline < cinfo.image_height) {
      unsigned char * buf = const_cast<unsigned char *> (image.bytes());
      buf = buf + cinfo.next_scanline * stride;
      JSAMPROW row_pointer = buf;
      jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    // Finish compression
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    return true;
  }

}
