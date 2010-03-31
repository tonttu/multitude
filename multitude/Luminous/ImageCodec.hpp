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

#ifndef IMAGE_CODEC_HPP
#define IMAGE_CODEC_HPP

#include <cstdio>

#include <string>

namespace Luminous
{
  class Image;
  struct ImageInfo;
  
  /// The base class for different image codecs. Derive your own codec from this
  /// and override the three methods.
  class ImageCodec 
  {
    public:
      virtual ~ImageCodec() {}
      /// Can this codec read the given file? The method should examine the file
      /// contents and return true if this codec can read it. The function must
      /// not change the current position in the file.
      /// @param file file to examine
      /// @return true if the codec can read the image data stored in the file, false if it can't
      virtual bool canRead(FILE * file) = 0;

      /// Get the extensions associated with this codec in a string separated by
      /// spaces
      ///  @return extensions separated by spaces (eg. "jpeg jpg")
      virtual std::string extensions() const = 0;

      /// Return name of the codec
      /// @return name of the codec
      virtual std::string name() const = 0;

      /// Pinging an image just reads the width, height, and pixel format from a file.
      /// @param info ImageInfo struct to store the read info to
      /// @param file file to read from
      /// @return true if the reading succeeded, false otherwise
      virtual bool ping(ImageInfo & info, FILE * file) = 0;

      /// Read the image data from the given file
      /// @param image Image to store the data into
      /// @param file file to read the data from
      /// @return true if the file was decoded successfully, false otherwise
      virtual bool read(Image & image, FILE * file) = 0;

      /// Store the given Image into a file
      /// @param image Image to store
      /// @param file file to write to
      /// @return true if the encoding was successful, false otherwise
      virtual bool write(const Image & image, FILE * file) = 0;

  };

}

#endif
