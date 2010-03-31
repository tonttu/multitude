/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */


#include "VideoImage.hpp"

#include "Trace.hpp"
#include "Types.hpp"

#include <stdlib.h>
#include <string.h>

namespace Radiant {

  void VideoImage::Plane::freeMemory()
  {
    free(m_data);
    m_data = 0;
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  VideoImage::~VideoImage()
  {
  }

  Nimble::Vector2i VideoImage::planeSize(ImageFormat fmt, int w, int h, int plane)
  {

    Nimble::Vector2i area(w, h);

    if(plane == 3) {
      area.clear();
    }
    else if(plane) {
      if(fmt == Radiant::IMAGE_YUV_411P) {
        area.x /= 4;
      }
      else if(fmt == Radiant::IMAGE_YUV_420P) {
        area /= 2;
      }
      else if(fmt == Radiant::IMAGE_YUV_422P) {
        area.x /= 2;
      }
      else
        area.clear();
    }
    else if(fmt == IMAGE_RGB) {
      area.x *= 3;
    }
    else if(fmt == IMAGE_RGBA) {
      area.x *= 4;
    }

    return area;
  }


  bool VideoImage::allocateMemory(ImageFormat fmt, int w, int h)
  {
    freeMemory();
    reset();

    m_width = w;
    m_height = h;

    unsigned pixels = w * h;

    if(fmt == IMAGE_RGB ||
       fmt == IMAGE_RGBA ||
       fmt == IMAGE_GRAYSCALE) {

      m_format = fmt;

      PlaneType pt = PLANE_UNKNOWN;
      int ls = 0;

      if(fmt == IMAGE_GRAYSCALE) {
        pt = PLANE_GRAYSCALE;
        ls = w;
      }
      else if(fmt == IMAGE_RGB) {
        pt = PLANE_RGB;
        ls = w * 3;
      }
      else if(fmt == IMAGE_RGBA) {
        pt = PLANE_RGBA;
        ls = w * 4;
      }
      else
        trace(FATAL, "VideoImage::allocateMemory");

      unsigned char * buf = (unsigned char*) malloc(ls * h);

      m_planes[0].set(buf, ls, pt);
    }
    else if(fmt == IMAGE_YUV_420P) {

      m_format = IMAGE_YUV_420P;

      int pixels4 = pixels >> 2;

      m_planes[0].set((unsigned char *) malloc(pixels),  w, PLANE_Y);
      m_planes[1].set((unsigned char *) malloc(pixels4), w / 2, PLANE_U);
      m_planes[2].set((unsigned char *) malloc(pixels4), w / 2, PLANE_V);
    }
    else if(fmt == IMAGE_YUV_422P) {

      m_format = IMAGE_YUV_422P;

      int pixels2 = pixels >> 1;

      m_planes[0].set((unsigned char *) malloc(pixels), w, PLANE_Y);
      m_planes[1].set((unsigned char *) malloc(pixels2), w / 2, PLANE_U);
      m_planes[2].set((unsigned char *) malloc(pixels2), w / 2, PLANE_V);
    }
    else
      return false;

    return true;
  }

  bool VideoImage::copyData(const VideoImage & that)
  {
    if(m_format != that.m_format ||
        m_width  != that.m_width ||
        m_height != that.m_height)
      return false;

    // The line counts of different planes
    uint linecount[4] = {
      m_height, m_height, m_height, 0
    };

    uint rowbytes[4] = {
      m_width, m_width, m_width, 0
    };

    if(m_format == IMAGE_GRAYSCALE) {
      linecount[1] = linecount[2] = 0;
      rowbytes[1]  = rowbytes[2]  = 0;
    }
    else if(m_format == IMAGE_YUV_411) {
      linecount[1] = linecount[2] = 0;
      rowbytes[1]  = rowbytes[2]  = 0;
      rowbytes[0]  = m_width + m_width / 2;
    }
    else if(m_format == IMAGE_YUV_420) {
      linecount[1] = linecount[2] = 0;
      rowbytes[1]  = rowbytes[2]  = 0;
      rowbytes[0]  = m_width + m_width / 2; // ?????
    }
    else if(m_format == IMAGE_YUV_420P) {
      linecount[1] = linecount[2] = m_height / 2;
      rowbytes[1]  = rowbytes[2]  = m_width / 2;
    }
    else if(m_format == IMAGE_YUV_422) {
      linecount[1] = linecount[2] = 0;
      rowbytes[1]  = rowbytes[2]  = 0;
      rowbytes[0]  = m_width + m_width;
    }
    else if(m_format == IMAGE_YUV_422P) {
      linecount[1] = linecount[2] = m_height;
      rowbytes[1]  = rowbytes[2]  = m_width / 2;
    }
    else if(m_format == IMAGE_RGB) {
      linecount[1] = linecount[2] = 0;
      rowbytes[1]  = rowbytes[2]  = 0;
      rowbytes[0]  = m_width * 3;
    }
    else if(m_format == IMAGE_RGBA) {
      linecount[1] = linecount[2] = 0;
      rowbytes[1]  = rowbytes[2]  = 0;
      rowbytes[0]  = m_width * 4;
    }

    for(uint i = 0; i < 4; i++) {
      const Plane & src = that.m_planes[i];
      Plane & dest = m_planes[i];

      uint lines = linecount[i];
      uint bytes = rowbytes[i];

      for(uint y = 0; y < lines; y++)
        memcpy(dest.line(y), src.line(y), bytes);
    }

    return true;
  }

  const char * VideoImage::formatName(ImageFormat fmt)
  {
    if(fmt == IMAGE_UNKNOWN)
      return "UNKNOWN";
    else if(fmt == IMAGE_GRAYSCALE)
      return "GRAYSCALE";
    else if(fmt == IMAGE_YUV_411)
      return "YUV_411";
    else if(fmt == IMAGE_YUV_411P)
      return "YUV_411P";
    else if(fmt == IMAGE_YUV_420)
      return "YUV_420";
    else if(fmt == IMAGE_YUV_420P)
      return "YUV_420P";
    else if(fmt == IMAGE_YUV_422)
      return "YUV_422";
    else if(fmt == IMAGE_YUV_422P)
      return "YUV_422P";
    else if(fmt == IMAGE_RGB)
      return "RGB";
    else if(fmt == IMAGE_RGBA)
      return "RGBA";
    else if(fmt == IMAGE_RAWBAYER)
      return "BAYER";

    return "ILLEGAL IMAGE FORMAT";
  }

  void VideoImage::zero()
  {
    switch(m_format) {
      case IMAGE_GRAYSCALE:
      case IMAGE_RGB:
      case IMAGE_RGBA:
          memset(m_planes[0].m_data, 0, m_planes[0].m_linesize * m_height);
        break;
        case IMAGE_YUV_420P:
        case IMAGE_YUV_422P:
          memset(m_planes[0].m_data, 0, m_planes[0].m_linesize * m_height);
          memset(m_planes[1].m_data, 0, m_planes[1].m_linesize * m_height);
          memset(m_planes[2].m_data, 0, m_planes[2].m_linesize * m_height);
          break;
      default:
        Radiant::error("VideoImage::zero # unsupported format");
        break;
    };
  }


  void VideoImage::freeMemory()
  {
    for(int i = 0; i < 4; i++)
      m_planes[i].freeMemory();
  }
}
