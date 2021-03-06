/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */


#include "VideoImage.hpp"
#include "MemCheck.hpp"
#include "Memory.hpp"

#include "Trace.hpp"
#include "Types.hpp"

#include <stdlib.h>
#include <string.h>

namespace Radiant {

  void VideoImage::Plane::allocateMemory(int width, int height, PlaneType type,
                                         size_t align)
  {
    m_type = type;

    while((width % static_cast<int>(align)) != 0)
      width++;

    m_linesize = width;
    m_data = static_cast<uint8_t*>(alignedMalloc(static_cast<size_t>(m_linesize * height),
                                                 static_cast<unsigned int>(align)));
  }

  void VideoImage::Plane::freeMemory()
  {
    alignedFree(m_data);
    m_data = nullptr;
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
    else if(fmt == IMAGE_RGB || fmt == IMAGE_BGR) {
      area.x *= 3;
    }
    else if(fmt == IMAGE_RGBA || fmt == IMAGE_BGRA) {
      area.x *= 4;
    }

    return area;
  }


  bool VideoImage::allocateMemory(ImageFormat fmt, int w, int h, size_t alignment)
  {
    if(w == m_width && h == m_height && fmt == m_format)
      return true;

    freeMemory();
    reset();

    m_width = w;
    m_height = h;

    if(fmt == IMAGE_RGB ||
       fmt == IMAGE_BGR ||
       fmt == IMAGE_RGBA ||
       fmt == IMAGE_BGRA ||
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
      else if(fmt == IMAGE_BGR) {
        pt = PLANE_BGR;
        ls = w * 3;
      }
      else if(fmt == IMAGE_RGBA) {
        pt = PLANE_RGBA;
        ls = w * 4;
      }
      else if(fmt == IMAGE_BGRA) {
        pt = PLANE_BGRA;
        ls = w * 4;
      }
      else
        fatal("VideoImage::allocateMemory");

      m_planes[0].allocateMemory(ls, h, pt, alignment);
    }
    else if(fmt == IMAGE_YUV_420P) {

      m_format = IMAGE_YUV_420P;

      m_planes[0].allocateMemory(w, h, PLANE_Y, alignment);
      m_planes[1].allocateMemory(w / 2, h / 2, PLANE_Y, alignment);
      m_planes[2].allocateMemory(w / 2, h / 2, PLANE_V, alignment);
    }
    else if(fmt == IMAGE_YUV_422P) {

      m_format = IMAGE_YUV_422P;

      m_planes[0].allocateMemory(w, h, PLANE_Y, alignment);
      m_planes[1].allocateMemory(w / 2, h, PLANE_Y, alignment);
      m_planes[2].allocateMemory(w / 2, h, PLANE_V, alignment);
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
      uint(m_height), uint(m_height), uint(m_height), 0
    };

    uint rowbytes[4] = {
      uint(m_width), uint(m_width), uint(m_width), 0
    };

    if(m_format == IMAGE_GRAYSCALE) {
      linecount[1] = linecount[2] = 0;
      rowbytes[1]  = rowbytes[2]  = 0;
    }
    else if(m_format == IMAGE_YUV_411) {
      linecount[1] = linecount[2] = 0;
      rowbytes[1]  = rowbytes[2]  = 0;
      rowbytes[0]  = static_cast<uint>(m_width + m_width / 2);
    }
    else if(m_format == IMAGE_YUV_420) {
      linecount[1] = linecount[2] = 0;
      rowbytes[1]  = rowbytes[2]  = 0;
      rowbytes[0]  = static_cast<uint>(m_width + m_width / 2); // ?????
    }
    else if(m_format == IMAGE_YUV_420P) {
      linecount[1] = linecount[2] = static_cast<uint>(m_height / 2);
      rowbytes[1]  = rowbytes[2]  = static_cast<uint>(m_width / 2);
    }
    else if(m_format == IMAGE_YUV_422) {
      linecount[1] = linecount[2] = 0;
      rowbytes[1]  = rowbytes[2]  = 0;
      rowbytes[0]  = static_cast<uint>(m_width + m_width);
    }
    else if(m_format == IMAGE_YUV_422P) {
      linecount[1] = linecount[2] = static_cast<uint>(m_height);
      rowbytes[1]  = rowbytes[2]  = static_cast<uint>(m_width / 2);
    }
    else if(m_format == IMAGE_RGB || m_format == IMAGE_BGR) {
      linecount[1] = linecount[2] = 0;
      rowbytes[1]  = rowbytes[2]  = 0;
      rowbytes[0]  = static_cast<uint>(m_width * 3);
    }
    else if(m_format == IMAGE_RGBA || m_format == IMAGE_BGRA) {
      linecount[1] = linecount[2] = 0;
      rowbytes[1]  = rowbytes[2]  = 0;
      rowbytes[0]  = static_cast<uint>(m_width * 4);
    }

    for(uint i = 0; i < 4; i++) {
      const Plane & src = that.m_planes[i];
      Plane & dest = m_planes[i];

      int lines = static_cast<int>(linecount[i]);
      uint bytes = rowbytes[i];

      for(int y = 0; y < lines; y++) {
        /* Take separate pointers, so that if something goes wrong we can check out the
        situation with debugger. */
        uint8_t * destptr = dest.line(y);
        const uint8_t * srcptr = src.line(y);
        memcpy(destptr, srcptr, bytes);
      }
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
      case IMAGE_BGR:
      case IMAGE_RGBA:
      case IMAGE_BGRA:
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
