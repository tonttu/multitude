/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_VIDEO_IMAGE_HPP
#define RADIANT_VIDEO_IMAGE_HPP

#include <Radiant/Export.hpp>

#include <Nimble/Vector2.hpp>

namespace Radiant {

  /// Enumeration of different video image formats
  enum ImageFormat
    {
      IMAGE_UNKNOWN, ///< Unknown format
      IMAGE_GRAYSCALE, ///< Grayscale

      IMAGE_YUV_411 = 10, ///< yuv411
      IMAGE_YUV_411P, ///< yuv411p
      IMAGE_YUV_420, ///< yuv420
      IMAGE_YUV_420P, ///< yuv420p
      IMAGE_YUV_422, ///< yuv422
      IMAGE_YUV_422P, ///< yuv 422p

      IMAGE_RGB_24 = 128, ///< RGB, 8 bytes for each channel
      IMAGE_RGBA_32, ///< RGBA, 8 bytes for each channel

      IMAGE_RGB = IMAGE_RGB_24, ///< Default RGB-format
      IMAGE_RGBA = IMAGE_RGBA_32, ///< Default RGBA-format

      IMAGE_BGR, ///< BGR
      IMAGE_BGRA, ///< BGRA

      IMAGE_RAWBAYER = 256 ///< Raw Bayer format
    };

  /// Enumeration of different video image plane types
  enum PlaneType
    {
      PLANE_UNKNOWN, ///< Unknown plane type

      PLANE_GRAYSCALE, ///< Grayscale

      PLANE_Y = 10, ///< Y-plane in YUV
      PLANE_U, ///< U-plane in YUV
      PLANE_V, ///< V-plane in YUV
      PLANE_YUV, ///< YUV-plane

      PLANE_RED = 128, ///< Red-plane in RGB(A)
      PLANE_GREEN, ///< Green-plane in RGB(A)
      PLANE_BLUE, ///< Blue-plane in RGB(A)
      PLANE_RGB, ///< RGB-plane
      PLANE_BGR, ///< BGR-plane
      PLANE_RGBA, ///< RGBA-plane
      PLANE_BGRA, ///< BGRA-plane

      PLANE_RAWBAYER = 256

    };

  /// An image class to be used with video IO.
  /** This image class is used to package planar and interleaved RGB,
      YUV and grayscale, with or without alpha channel. A typical use
      case for this class is the transfer of video frames between
      video codec and the application.

      An image is composed of planes that contain the actual image
      data.

      @note By default this class does not do any memory management, which
      is left to the application. There are memory management functions
      that can be used if you want.
  */
  /// @todo Make data members private
  class RADIANT_API VideoImage
  {
  public:
    /// Constructs a new video image
    /// @param fmt Format of the image
    /// @param w Width of the image
    /// @param h Height of the image
    VideoImage(ImageFormat fmt = IMAGE_UNKNOWN, int w = 0, int h = 0)
      : m_format(fmt), m_width(w), m_height(h) {}
    /// Destructor
    ~VideoImage();

    /** Holds the data for one image plane.
    @see VideoImage
     */
    class RADIANT_API Plane {
    public:
      /// Constructor
      Plane() : m_data(0), m_linesize(0), m_type(PLANE_UNKNOWN) {}

      /// Makes a shallow copy of the data
      /// @param data Pointer to copy
      /// @param linesize Linesize of the copied date
      /// @param type Type of the plane
      void set(unsigned char * data, int linesize, PlaneType type)
      { m_data = data; m_linesize = linesize; m_type = type; }
      void allocateMemory(int width, int height, PlaneType type, size_t align);

      /// Frees the memory associated with the plane
      void freeMemory();

      /// Get a line from the image
      /// @param y Index of line
      /// @return Pointer to the beginning of the line
      unsigned char * line(int y) { return m_data + m_linesize * y; }
      /// @copydoc line
      const unsigned char * line(int y) const{return m_data + m_linesize * y; }

      /// Pointer to the data
      unsigned char * m_data;
      /// Line size (interval)
      int             m_linesize;
      /// Plane contents
      PlaneType       m_type;
    };

    /// Resets the image
    void reset()
    {
      VideoImage tmp;
      *this = tmp;
    }

    /// Sets the image format to interleaved RGB.
    void setFormatRGB()
    {
      m_format = IMAGE_RGB;
      m_planes[0].m_type = PLANE_RGB;
    }

    /// Sets the image format to interleaved BGR.
    void setFormatBGR()
    {
      m_format = IMAGE_BGR;
      m_planes[0].m_type = PLANE_BGR;
    }

    /// Sets the image format to interleaved RGB.
    void setFormatRGBA()
    {
      m_format = IMAGE_RGBA;
      m_planes[0].m_type = PLANE_RGBA;
    }

    /// Sets the image format to interleaved BGRA.
    void setFormatBGRA()
    {
      m_format = IMAGE_BGRA;
      m_planes[0].m_type = PLANE_BGRA;
    }

    /// Sets the image format to interleaved YUV 420.
    void setFormatYUV420()
    {
      m_format = IMAGE_YUV_420;
      m_planes[0].m_type = PLANE_YUV;
    }

    /// Sets the image format to planar YUV 420.
    void setFormatYUV420P()
    {
      m_format = IMAGE_YUV_420P;
      m_planes[0].m_type = PLANE_Y;
      m_planes[1].m_type = PLANE_U;
      m_planes[2].m_type = PLANE_V;
      m_planes[3].m_type = PLANE_UNKNOWN;
    }

    /// Sets the image format to planar YUV 422.
    void setFormatYUV422P()
    {
      m_format = IMAGE_YUV_422P;
      m_planes[0].m_type = PLANE_Y;
      m_planes[1].m_type = PLANE_U;
      m_planes[2].m_type = PLANE_V;
      m_planes[3].m_type = PLANE_UNKNOWN;
    }

    /// Sets the image format to planar RGB24.
    void setFormatRGB24()
    {
      m_format = IMAGE_RGB_24;
      m_planes[0].m_type = PLANE_RGB;
      m_planes[1].m_type = PLANE_UNKNOWN;
      m_planes[2].m_type = PLANE_UNKNOWN;
      m_planes[3].m_type = PLANE_UNKNOWN;
    }

    /// Returns the size of an image plane in bytes
    /// @param fmt Format of the image
    /// @param w Width of the plane
    /// @param h Height of the plane
    static Nimble::Vector2i planeSize(ImageFormat fmt, int w, int h, int plane);

    /// Allocates memory and sets the image format.
    /// @param fmt Image format
    /// @param w image width
    /// @param h image height
    /// @param alignment Alignment for memory allocation and line size
    /// @returns true if allocation was successful
    bool allocateMemory(ImageFormat fmt, int w, int h, size_t alignment);
    /// Allocates memory and sets the image format, based on another image
    /** This function can be used when preparing to copy contents from
    from another image.
    @param that VideoImage used as template for the new width, height and format settings
    @param alignment Alignment for memory allocation and line size
    @returns true if allocation was successful */
    bool allocateMemory(const VideoImage & that, size_t alignment)
    { return allocateMemory(that.m_format, that.width(), that.height(), alignment); }

    /// Copies the image data.
    /** The image format, image size and data
    buffers should be set correctly before calling this method. */
    /// @param that VideoImage to copy data from
    /// @returns true if copy was successful
    bool copyData(const VideoImage & that);

    /// Free memory on all the planes
    void freeMemory();

    /// Returns the size of the image data in bytes
    /// @return Number of bytes
    unsigned size() const
    {
      const unsigned pixels = m_width * m_height;

      switch (m_format)
      {
      case IMAGE_GRAYSCALE:
        return pixels;
      case IMAGE_BGR:
      case IMAGE_RGB:
        return pixels * 3;
      case IMAGE_BGRA:
      case IMAGE_RGBA:
        return pixels * 3;
      default:
        return 0;
      }
    }

    /// Returns the pixel dimensions of the image
    /// @return Number of pixels in the image
    Nimble::Vector2i geometry() const { return Nimble::Vector2i(m_width, m_height); }

    /// Returns a readable name of a given image format
    /// @param fmt Format of the image
    /// @return Name of the format
    static const char * formatName(ImageFormat fmt);

    /// The width of the image
    /// @return Width of the image
    int width()  const { return m_width; }
    /// The height of the image
    /// @return Height of the image
    int height() const { return m_height; }
    /// The number of pixels in the image
    /// @return Number of pixels in the image
    int pixels() const { return m_width * m_height; }

    /// Fill the image with 0
    void zero();

    /// Plane information
    Plane m_planes[4];
    /// Image format
    ImageFormat m_format;
    /// Width of the image
    int m_width;
    /// Height of the image
    int m_height;
  };

}

#endif
