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

#include "ImageConversion.hpp"

#include "Trace.hpp"
#include "VideoImage.hpp"
#include "Types.hpp"

#include <stdio.h>
#include <string.h>

#include <assert.h>

namespace Radiant {

  bool ImageConversion::convert(const VideoImage * source, VideoImage * target)
  {
    bool ok = true;

    // trace("ImageConversion::convert");

    assert(source != 0 && target != 0);
  
    ImageFormat sourceFmt = source->m_format;
    ImageFormat targetFmt = target->m_format;

    if(sourceFmt == targetFmt) {
      ok = target->copyData( * source);
    }
    else if(sourceFmt == IMAGE_GRAYSCALE) {
      if(targetFmt == IMAGE_RGB)
	grayscaleToRGB(source, target);
      else 
	ok = false;
    }
    else if(sourceFmt == IMAGE_YUV_411) {
      if(targetFmt == IMAGE_GRAYSCALE)
	YUV411ToGrayscale(source, target);
      else if(targetFmt == IMAGE_RGB)
	YUV411ToRGB(source, target);
      else 
	ok = false;
    }
    else if(sourceFmt == IMAGE_YUV_411P) {
      if(targetFmt == IMAGE_RGB)
	YUV411PToRGB(source, target);
      else if(targetFmt == IMAGE_RGBA)
	YUV411PToRGBA(source, target);
      else 
	ok = false;
    }
    else if(sourceFmt == IMAGE_YUV_420P) {
      if(targetFmt == IMAGE_GRAYSCALE)
	YUV420PToGrayscale(source, target);
      else if(targetFmt == IMAGE_RGBA)
	YUV420PToRGBA(source, target);
      else if(targetFmt == IMAGE_RGB)
	YUV420PToRGB(source, target);
      else 
	ok = false;    
    }
    else if(sourceFmt == IMAGE_YUV_420) {
      if(targetFmt == IMAGE_GRAYSCALE)
	YUV420ToGrayscale(source, target);
      else if(targetFmt == IMAGE_RGBA)
	YUV420ToRGBA(source, target);
      else 
	ok = false;    
    }
    else if(sourceFmt == IMAGE_YUV_422P) {
      if(targetFmt == IMAGE_RGBA)
	YUV422PToRGBA(source, target);
      else if(targetFmt == IMAGE_GRAYSCALE)
	YUV422PToGrayscale(source, target);
      else 
	ok = false;
    }
    else if(sourceFmt == IMAGE_RGB) {
      if(targetFmt == IMAGE_GRAYSCALE)
	RGBToGrayscale(source, target);
      else 
	ok = false;    
    }
    else if(sourceFmt == IMAGE_RAWBAYER) {
      if(targetFmt == IMAGE_RGB)
	bayerToRGB(source, target);
      else if(targetFmt == IMAGE_GRAYSCALE)
	bayerToGrayscale(source, target);
      else 
	ok = false;    
    
    }
    else 
      ok = false;

    if(!ok)
      Radiant::error("ImageConversion::convert # %s %s",
	    VideoImage::formatName(sourceFmt),
	    VideoImage::formatName(targetFmt));

    return ok;
  }

  inline int clamp(int x, int low, int high)
  {
    if(x <= low) return low;
    if(x >= high) return high;
    return x;
  }

  inline void YUV2RGB(int y, int u, int v, int & r, int & g, int & b)
  {
    /* u -= 128;
       v -= 128; */

    /* y -= 16;
       float yf = 1.164f * y;
     
       b = clamp((int) (yf + 2.018f * u), 0, 255);
       g = clamp((int) (yf - 0.813f * v - 0.391f * u), 0, 255);
       r = clamp((int) (yf + 1.596f * v), 0, 255); */


    // www.answers.com/topic/yuv-rgb-conversion-formulas

    // u /=2 ;
    // v /=2 ;

    r = clamp(y + ((1167 * v) >> 10), 0, 255);
    g = clamp(y - ((595 * v + 404 * u) >> 10), 0, 255);
    b = clamp(y + ((2080 * u) >> 10), 0, 255);
  }

  // color conversion functions from Bart Nabbe.
  // corrected by Damien: bad coeficients in YUV2RGB
  /** These macros were copied from the Coriander. */

  /*
    #define YUV2RGB(y, u, v, r, g, b)\
    r = y + ((v*1436) >> 10);\
    g = y - ((u*352 + v*731) >> 10);\
    b = y + ((u*1814) >> 10);\
    r = r < 0 ? 0 : r;\
    g = g < 0 ? 0 : g;\
    b = b < 0 ? 0 : b;\
    r = r > 255 ? 255 : r;\
    g = g > 255 ? 255 : g;\
    b = b > 255 ? 255 : b
  */
  /*#define RGB2YUV(r, g, b, y, u, v)		\
    y = (306*r + 601*g + 117*b)  >> 10;\
    u = ((-172*r - 340*g + 512*b) >> 10)  + 128;\
    v = ((512*r - 429*g - 83*b) >> 10) + 128;\
    y = y < 0 ? 0 : y;\
    u = u < 0 ? 0 : u;\
    v = v < 0 ? 0 : v;\
    y = y > 255 ? 255 : y;\
    u = u > 255 ? 255 : u;\
    v = v > 255 ? 255 : v
  */

  /* This function contains code copied from the Coriander. */
  void ImageConversion::YUV411ToRGB
  (const VideoImage *, VideoImage *)
  {
    Radiant::trace(FATAL, "ImageConversion::YUV411ToRGB");
    /*  long numpixels = (long) width * (long) height;
  
    long i = numpixels + ( numpixels >> 1 ) -1;
    long j = numpixels * 3 - 1;
    int y0, y1, y2, y3, u, v, r, g, b;

    while (i > 0)
    {
    y3 = (unsigned char) src[i--];
    y2 = (unsigned char) src[i--];
    v  = (unsigned char) src[i--] - 128;
    y1 = (unsigned char) src[i--];
    y0 = (unsigned char) src[i--];
    u  = (unsigned char) src[i--] - 128;

    YUV2RGB (y3, u, v, r, g, b);
    dest[j--] = b;
    dest[j--] = g;
    dest[j--] = r;
    YUV2RGB (y2, u, v, r, g, b);
    dest[j--] = b;
    dest[j--] = g;
    dest[j--] = r;
    YUV2RGB (y1, u, v, r, g, b);
    dest[j--] = b;
    dest[j--] = g;
    dest[j--] = r;
    YUV2RGB (y0, u, v, r, g, b);
    dest[j--] = b;
    dest[j--] = g;
    dest[j--] = r;
    }*/
  }

  void ImageConversion::YUV411PToRGB
  (const VideoImage * source, VideoImage * target)
  {
    long w = source->m_width;
    long h = source->m_height;

    target->m_width  = w;
    target->m_height = h;
    target->m_format = IMAGE_RGB;
    target->m_planes[0].m_linesize = w;
    target->m_planes[0].m_type = PLANE_RGB;
  
    assert(target->m_planes[0].m_data);

    uchar * dest = target->m_planes[0].m_data;

    for(long l = 0; l < h; l++) {

      const uchar * iy = source->m_planes[0].line(l);
      const uchar * iu = source->m_planes[1].line(l);
      const uchar * iv = source->m_planes[2].line(l);

      const uchar * sentinel = iy + w;

      while(iy < sentinel) {

	int r, g, b;
      
	int u = *iu++;
	int v = *iv++;

	int y = *iy++;

	u -= 127;
	v -= 127;

	YUV2RGB(y, u, v, r, g, b);
	dest[0] = r;
	dest[1] = g;
	dest[2] = b;
	dest += 3;

	y = *iy++;

	YUV2RGB(y, u, v, r, g, b);
	dest[0] = r;
	dest[1] = g;
	dest[2] = b;
	dest += 3;

	y = *iy++;

	YUV2RGB(y, u, v, r, g, b);
	dest[0] = r;
	dest[1] = g;
	dest[2] = b;
	dest += 3;

	y = *iy++;

	YUV2RGB(y, u, v, r, g, b);
	dest[0] = r;
	dest[1] = g;
	dest[2] = b;
	dest += 3;
      }
    }
  
  }

  void ImageConversion::YUV411PToRGBA
  (const VideoImage * source, VideoImage * target)
  {
    long w = source->m_width;
    long h = source->m_height;

    target->m_width  = w;
    target->m_height = h;
    target->m_format = IMAGE_RGBA;
    target->m_planes[0].m_linesize = w;
    target->m_planes[0].m_type = PLANE_RGBA;
  
    assert(target->m_planes[0].m_data);

    uchar * dest = target->m_planes[0].m_data;

    for(long l = 0; l < h; l++) {

      const uchar * iy = source->m_planes[0].line(l);
      const uchar * iu = source->m_planes[1].line(l);
      const uchar * iv = source->m_planes[2].line(l);

      const uchar * sentinel = iy + w;

      while(iy < sentinel) {

	int r, g, b;
      
	int u = *iu++;
	int v = *iv++;

	int y = *iy++;

	YUV2RGB(y, u, v, r, g, b);
	dest[0] = r;
	dest[1] = g;
	dest[2] = b;
	dest[3] = 0xFF;
	dest += 4;

	y = *iy++;

	YUV2RGB(y, u, v, r, g, b);
	dest[0] = r;
	dest[1] = g;
	dest[2] = b;
	dest[3] = 0xFF;
	dest += 4;

	y = *iy++;

	YUV2RGB(y, u, v, r, g, b);
	dest[0] = r;
	dest[1] = g;
	dest[2] = b;
	dest[3] = 0xFF;
	dest += 4;

	y = *iy++;

	YUV2RGB(y, u, v, r, g, b);
	dest[0] = r;
	dest[1] = g;
	dest[2] = b;
	dest[3] = 0xFF;
	dest += 4;
      }
    }
  
  }

  /**  */

  void ImageConversion::YUV411ToGrayscale
  (const VideoImage *source, VideoImage *target)
  {
    long w = source->m_width;
    long h = source->m_height;

    uchar * dest = target->m_planes[0].m_data;

    target->m_width  = w;
    target->m_height = h;
    target->m_format = IMAGE_GRAYSCALE;
    target->m_planes[0].m_linesize = w;
    target->m_planes[0].m_type = PLANE_GRAYSCALE;

    for(long y = 0; y < h; y++) {
      const uchar * src = source->m_planes[0].line(y) + 1;
      uchar * sentinel = dest + w;

      while(dest < sentinel) {
	*dest++ = src[0];
	*dest++ = src[1];
	src += 3;
	*dest++ = src[0];
	*dest++ = src[1];
	src += 3;
      }
    }
  }

  void ImageConversion::YUV420PToGrayscale
  (const VideoImage * source, VideoImage * target)
  {
    long lw = source->m_planes[0].m_linesize;
    long w = source->m_width;
    long h = source->m_height;

    target->m_width  = w;
    target->m_height = h;
    target->m_format = IMAGE_GRAYSCALE;
    target->m_planes[0].m_linesize = w;

    const uchar * src = source->m_planes[0].m_data;
    uchar * dest = target->m_planes[0].m_data;
  
    for(long y = 0; y < h; y++) {
      memcpy(dest, src, w);
      dest += w;
      src += lw;
    }
  }

  void ImageConversion::YUV420PToRGBA
  (const VideoImage * source, VideoImage * target)
  {
    long w = source->m_width;
    long h = source->m_height;

    target->m_width  = w;
    target->m_height = h;
    target->m_format = IMAGE_RGBA;
    target->m_planes[0].m_linesize = w * 4;
    target->m_planes[0].m_type = PLANE_RGBA;

    assert(target->m_planes[0].m_data);


    /* Convert  the image two lines at a time (2x2 macroblocks). */

    for(long l = 0; l < h; l += 2) {

      uchar * dest1 = target->m_planes[0].line(l);
      uchar * dest2 = target->m_planes[0].line(l + 1);

      long l2 = l / 2;

      const uchar * iy1 = source->m_planes[0].line(l);
      const uchar * iy2 = source->m_planes[0].line(l + 1);
      const uchar * iu  = source->m_planes[1].line(l2);
      const uchar * iv  = source->m_planes[2].line(l2);

      const uchar * sentinel = iy1 + w;

      while(iy1 < sentinel) {

	int r, g, b;
      
	int u = *iu++;
	int v = *iv++;

	u -= 128;
	v -= 128;

	int y = *iy1++;
	YUV2RGB(y, u, v, r, g, b);
	dest1[0] = r;
	dest1[1] = g;
	dest1[2] = b;
	dest1[3] = 0xFF;
	dest1 += 4;

	y = *iy1++;
	YUV2RGB(y, u, v, r, g, b);
	dest1[0] = r;
	dest1[1] = g;
	dest1[2] = b;
	dest1[3] = 0xFF;
	dest1 += 4;

	y = *iy2++;
	YUV2RGB(y, u, v, r, g, b);
	dest2[0] = r;
	dest2[1] = g;
	dest2[2] = b;
	dest2[3] = 0xFF;
	dest2 += 4;

	y = *iy2++;
	YUV2RGB(y, u, v, r, g, b);
	dest2[0] = r;
	dest2[1] = g;
	dest2[2] = b;
	dest2[3] = 0xFF;
	dest2 += 4;

	//printf("yuv = %d %d %d -> rgb = %d %d %d XXXXXX ", y, u, v, r, g, b);
      }
    }
  }

  void ImageConversion::YUV420PToRGB
  (const VideoImage * source, VideoImage * target)
  {
    long w = source->m_width;
    long h = source->m_height;

    target->m_width  = w;
    target->m_height = h;
    target->m_format = IMAGE_RGB;
    target->m_planes[0].m_linesize = w * 3;
    target->m_planes[0].m_type = PLANE_RGB;

    assert(target->m_planes[0].m_data);


    /* Convert  the image two lines at a time (2x2 macroblocks). */

    for(long l = 0; l < h; l += 2) {

      uchar * dest1 = target->m_planes[0].line(l);
      uchar * dest2 = target->m_planes[0].line(l + 1);

      long l2 = l / 2;

      const uchar * iy1 = source->m_planes[0].line(l);
      const uchar * iy2 = source->m_planes[0].line(l + 1);
      const uchar * iu  = source->m_planes[1].line(l2);
      const uchar * iv  = source->m_planes[2].line(l2);

      const uchar * sentinel = iy1 + w;

      while(iy1 < sentinel) {

	int r, g, b;
      
	int u = *iu++;
	int v = *iv++;

	u -= 128;
	v -= 128;

	int y = *iy1++;
	YUV2RGB(y, u, v, r, g, b);
	dest1[0] = r;
	dest1[1] = g;
	dest1[2] = b;
	dest1 += 3;

	y = *iy1++;
	YUV2RGB(y, u, v, r, g, b);
	dest1[0] = r;
	dest1[1] = g;
	dest1[2] = b;
	dest1 += 3;

	y = *iy2++;
	YUV2RGB(y, u, v, r, g, b);
	dest2[0] = r;
	dest2[1] = g;
	dest2[2] = b;
	dest2 += 3;

	y = *iy2++;
	YUV2RGB(y, u, v, r, g, b);
	dest2[0] = r;
	dest2[1] = g;
	dest2[2] = b;
	dest2 += 3;

	//printf("yuv = %d %d %d -> rgb = %d %d %d XXXXXX ", y, u, v, r, g, b);
      }
    }
  }

  void ImageConversion::YUV422PToRGBA
  (const VideoImage * source, VideoImage * target)
  {
    long w = source->m_width;
    long h = source->m_height;

    target->m_width  = w;
    target->m_height = h;
    target->m_format = IMAGE_RGBA;
    target->m_planes[0].m_linesize = w * 4;
    target->m_planes[0].m_type = PLANE_RGBA;

    assert(target->m_planes[0].m_data);

    /* Convert  the image (2x1 macroblocks). */

    for(long l = 0; l < h; l++) {

      uchar * dest = target->m_planes[0].line(l);

      const uchar * iy  = source->m_planes[0].line(l);
      const uchar * iu  = source->m_planes[1].line(l);
      const uchar * iv  = source->m_planes[2].line(l);

      const uchar * sentinel = iy + w;

      while(iy < sentinel) {

	int r, g, b;
      
	int u = *iu++;
	int v = *iv++;

	u -= 128;
	v -= 128;

	int y = *iy++;
	YUV2RGB(y, u, v, r, g, b);
	dest[0] = r;
	dest[1] = g;
	dest[2] = b;
	dest[3] = 0xFF;
	dest += 4;

	y = *iy++;
	YUV2RGB(y, u, v, r, g, b);
	dest[0] = r;
	dest[1] = g;
	dest[2] = b;
	dest[3] = 0xFF;
	dest += 4;

	//printf("yuv = %d %d %d -> rgb = %d %d %d XXXXXX ", y, u, v, r, g, b);
      }
    }
  }

  void ImageConversion::YUV422PToGrayscale
  (const VideoImage * source, VideoImage * target)
  {
    long w = source->m_width;
    long h = source->m_height;

    target->m_width  = w;
    target->m_height = h;
    target->m_format = IMAGE_GRAYSCALE;
    target->m_planes[0].m_linesize = w;
    target->m_planes[0].m_type = PLANE_GRAYSCALE;

    assert(target->m_planes[0].m_data);

    /* Convert  the image (2x1 macroblocks). */

    for(long l = 0; l < h; l++) {
      memcpy(target->m_planes[0].line(l), source->m_planes[0].line(l), w);
    }
  }

  void ImageConversion::YUV420ToGrayscale
  (const VideoImage * source, VideoImage * target)
  {
    long lw = source->m_planes[0].m_linesize;
    long w = source->m_width;
    long h = source->m_height;

    target->m_width  = w;
    target->m_height = h;
    target->m_format = IMAGE_GRAYSCALE;
    target->m_planes[0].m_linesize = w;

    const uchar * src = source->m_planes[0].m_data;
    uchar * dest = target->m_planes[0].m_data;
  
    for(long y = 0; y < h; y++) {
      const uchar * srcplane = & src[y * lw];
    
      for(long x = 0; x < w; x++) {
	*dest++ = *srcplane++;
	*dest++ = *srcplane++;
	*dest++ = *srcplane++;
	*dest++ = *srcplane++;

	srcplane += 2;
      }
    }
  }

  void ImageConversion::YUV420ToRGBA
  (const VideoImage *, VideoImage * )
  {
/*
    bool implemented = false;

    assert(implemented);

      long lw = source->m_planes[0].m_linesize;
      long w = source->m_width;
      long h = source->m_height;

      target->m_width  = w;
      target->m_height = h;
      target->m_format = IMAGE_GRAYSCALE;
      target->m_planes[0].m_linesize = w;

      const uchar * src = source->m_planes[0].m_data;
      uchar * dest = target->m_planes[0].m_data;
  
      for(long l = 0; l < h; l++) {

      const uchar * src = & src[l * lw];
      const char * sentinel = src + w;

      while(src < sentinel) {
      
      }
    
      }*/
  }

  void ImageConversion::grayscaleToRGB
  (const VideoImage * source, VideoImage * target)
  {
    long w = source->m_width;
    long h = source->m_height;

    target->m_width  = w;
    target->m_height = h;
    target->m_format = IMAGE_RGB;
    target->m_planes[0].m_linesize = w * 3;

    const uchar * src  = source->m_planes[0].m_data;
    uchar * dest = target->m_planes[0].m_data;

    uchar * sentinel = dest + w * h * 3;

    while(dest < sentinel) {
    
      uchar tmp = * src;

      dest[0] = tmp;
      dest[1] = tmp;
      dest[2] = tmp;

      src ++;
      dest += 3;
    }
  }

  void ImageConversion::RGBToGrayscale
  (const VideoImage * source, VideoImage * target)
  {
    long lw = source->m_planes[0].m_linesize;
    long w = source->m_width;
    long h = source->m_height;

    target->m_width  = w;
    target->m_height = h;
    target->m_planes[0].m_linesize = w;

    // target->m_planes[0].m_type = IMAGE_GRAYSCALE;

    const uchar * src = source->m_planes[0].m_data;
    uchar * dest = target->m_planes[0].m_data;
  
    for(long y = 0; y < h; y++) {

      const uchar * srcplane = & src[y * lw];

      for(long x = 0; x < w; x++) {
	unsigned sum = srcplane[0] + srcplane[1] + srcplane[2];
	*dest++ = sum / 3;
	srcplane += 3;
      }    
    }
  }

  void ImageConversion::bayerToRGB(const VideoImage * source, VideoImage * target)
  {
    long lw = source->m_planes[0].m_linesize;
    long w = source->m_width / 2;
    long h = source->m_height / 2;

    target->m_width  = w;
    target->m_height = h;
    target->m_format = IMAGE_RGB;
    target->m_planes[0].m_linesize = 3 * w;

    const uchar * src = source->m_planes[0].m_data;
    uchar * dest = target->m_planes[0].m_data;
  
    for(long y = 0; y < h; y++) {

      const uchar * src1 = & src[y * 2 * lw];
      const uchar * src2 = src1 + lw;

      for(long x = 0; x < w; x++) {
	uint green = (uint) src1[0] + (uint) src2[1];
	dest[0] = src2[0];
	dest[2] = src1[1];
	dest[1] = green >> 1;

	src1 += 2;
	src2 += 2;
	dest += 3;
      }    
    }

  }

  void ImageConversion::bayerToGrayscale(const VideoImage * source, VideoImage * target)
  {
    long lw = source->m_planes[0].m_linesize;
    long w = source->m_width / 2;
    long h = source->m_height / 2;

    target->m_width  = source->m_width;
    target->m_height = source->m_height;
    target->m_format = IMAGE_GRAYSCALE;
    target->m_planes[0].m_linesize = source->m_width;

    const uchar * src = source->m_planes[0].m_data;
  
    uchar * dest = target->m_planes[0].m_data;

    for(long y = 0; y < h; y++) {

      const uchar * src1 = & src[y * 2 * lw];
      const uchar * src2 = src1 + lw;

      for(long x = 0; x < w; x++) {
	uint green = (uint) src1[0] + (uint) src2[1];
	uint red  = src2[0];
	uint blue = src1[1];
	uint gray = (green + red + blue) >> 2;

	dest[0]  = gray;
	dest[1]  = gray;
	dest[lw] = gray;
	dest[lw+1] = gray;

	src1 += 2;
	src2 += 2;
	dest += 2;
      }

      dest += lw;
    }

  }

}
