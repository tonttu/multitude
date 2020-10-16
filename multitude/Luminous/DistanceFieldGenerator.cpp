/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "DistanceFieldGenerator.hpp"

#include "Image.hpp"

#include <Radiant/Grid.hpp>

namespace Luminous
{
  namespace
  {
    template<typename T, typename U>
    void generalDistanceTransform1d(U * function,
                                           T * output,
                                           size_t output_stride,
                                           int n,
                                           int * locs,
                                           float * ranges)
    {
      float inf = std::numeric_limits<float>::max();

      int k = 0;
      locs[0] = 0;
      ranges[0] = -inf;
      ranges[1] = inf;

      for(int q=1; q < n; ++q) {
        // s is the point of intersection for parabolas
        //   x |-> (q-x)^2 + function[q])
        // and
        //   x |-> (locs[k]-x)^2 + function[locs[k]]

        float q2 = function[q] + q*q;
retry:
        float s = (q2 - (function[locs[k]] + locs[k]*locs[k])) / (2*(q - locs[k]));

        if(s <= ranges[k]) {
          k--;
          goto retry;
        }
        k++;
        locs[k] = q;
        ranges[k] = s;
        ranges[k+1] = inf;
      }

      k = 0;
      size_t index = 0;
      for(int q=0; q < n; ++q) {
        while(ranges[k+1] < q)
          k++;


        if(function[q] == 0) {
          output[index] = 0;
        } else { // height of lower envelope at locs[k]
          int t = q-locs[k];
          output[index] = t*t + function[locs[k]];
        }

        index += output_stride;
      }
    }

    template<typename T, typename U>
    void generalDistanceTransform2d(U * f, size_t stride, T * output, size_t w, size_t h,
                                           std::vector<float> rowTransformed,
                                           std::vector<int> locs,
                                           std::vector<float> ranges
                                           )
    {
      if(rowTransformed.size() < w*h)
        rowTransformed.resize(w*h);

      if(locs.size() < std::max(w,h))
        locs.resize(std::max(w, h));

      if(ranges.size() < std::max(w,h)+1)
        ranges.resize(std::max(w,h)+1);


      for(size_t y=0; y < h; ++y) {
        // transform row y,
        // write result to column y of rowTransformed
        generalDistanceTransform1d(&f[y * stride],
                                   &rowTransformed[y], h, static_cast<int>(w),
                                   locs.data(), ranges.data());
      }

      for(size_t x=0; x < w; ++x) {
        // transform row x of rowTransformed,
        // write result to column x of output
        generalDistanceTransform1d(&rowTransformed[x * h],
                                   &output[x],
                                   w, static_cast<int>(h),
                                   locs.data(), ranges.data());
      }
    }

    template <typename T>
    void writeImage(const float maxim, Luminous::Image & target,
                    const Radiant::PtrGrid32u & distances_inv, const Radiant::PtrGrid32u & distances,
                    const Nimble::Vector2f & scale)
    {
      const int theight = target.height(), twidth = target.width();
      const float toInt = static_cast<float>((uint64_t(1) << (sizeof(T) * 8)) - 1);

      for (int ty = 0; ty < theight; ++ty) {
        T * line = reinterpret_cast<T*>(target.line(ty));
        const float sy = scale.y*ty;

        for (int tx = 0; tx < twidth; ++tx) {
          const float sx = scale.x * tx;

          float distance = distances.getInterpolatedSafe<float>(Nimble::Vector2(sx, sy));
          float distance_inv = distances_inv.getInterpolatedSafe<float>(Nimble::Vector2(sx, sy));
          float v = distance-distance_inv;
          float sgn = v < 0 ? -1 : 1;
          float q = sgn*std::sqrt(sgn*v) / maxim;
          line[tx] = (0.5f + q) * toInt;
        }
      }
    }
  }

  void DistanceFieldGenerator::generate(const Luminous::Image & src, Nimble::Vector2i srcSize, Luminous::Image & target, int radius)
  {
    const int sheight = srcSize.y, swidth = srcSize.x;
    const int theight = target.height(), twidth = target.width();
    const Nimble::Vector2f scale(float(swidth)/twidth, float(sheight)/theight);

    assert(src.pixelFormat().bytesPerPixel() == 1);

#if 1
    const int spixels = sheight*swidth;
    std::vector<uint32_t> orig(spixels), res(spixels), res_inverted(spixels);

    // Squared distances are clamped to this
    float inf = radius * radius;

    for(int y=0; y < sheight; ++y) {
      const unsigned char * p = src.line(y);
      for(int x=0; x < swidth; ++x) {
        orig[y*swidth + x] = *p == 0 ? 0 : inf;
        p++;
      }
    }

    // temporary buffers
    std::vector<float> rowTransformed;
    std::vector<int> locs;
    std::vector<float> ranges;

    generalDistanceTransform2d(
          orig.data(),
          swidth,
          res.data(),
          static_cast<size_t>(swidth), static_cast<size_t>(sheight),
          rowTransformed, locs, ranges
          );

    for(int i=0; i < spixels; ++i)
      orig[i] = orig[i] == 0 ? inf : 0;

    generalDistanceTransform2d(
          orig.data(),
          swidth,
          res_inverted.data(),
          swidth, sheight,
          rowTransformed, locs, ranges
          );


    float maxim = 2*radius;

    // squared distances
    Radiant::PtrGrid32u distances(res.data(), swidth, sheight);
    Radiant::PtrGrid32u distances_inv(res_inverted.data(), swidth, sheight);

    if (target.pixelFormat().bytesPerPixel() == 1)
      writeImage<uint8_t>(maxim, target, distances_inv, distances, scale);
    else if (target.pixelFormat().bytesPerPixel() == 2)
      writeImage<uint16_t>(maxim, target, distances_inv, distances, scale);
    else if (target.pixelFormat().bytesPerPixel() == 4)
      writeImage<uint32_t>(maxim, target, distances_inv, distances, scale);
    else
      Radiant::error("DistanceFieldGenerator::generate # Unsupported pixel format");

#else
    assert(target.pixelFormat().bytesPerPixel() == 1);

    // Iterate all pixels in the target image
    for (int ty = 0; ty < theight; ++ty) {
      unsigned char * line = target.line(ty);
      const int sy = Nimble::Math::Round(scale.y * ty);

      for (int tx = 0; tx < twidth; ++tx) {
        const int sx = Nimble::Math::Round(scale.x * tx);

        // is this pixel "inside"
        const bool pixelIn = src.line(sy)[sx] > 0x7f;

        int best2 = radius*radius;

        // manhattan distance optimization
        int best1 = radius;

        // this could actually test pixels from (sx, 0) -> (sx, sheight) and
        // (0, sy) -> (swidth, sy) first, since after testing those, best1 with
        // vm/um would be much smaller in most cases

        // Iterate the neighbourhood in the source image
        for (int v = std::max(0, sy-radius), vm = std::min(sheight, sy+radius); v < vm; ++v) {
          const unsigned char * testLine = src.line(v);
          for (int u = std::max(0, sx-radius), um = std::min(swidth, sx+radius); u < um; ++u) {

            // Test if we found border
            bool testIn = testLine[u] > 0x7f;
            if (pixelIn == testIn)
              continue;

            int currentLength2 = (v-sy)*(v-sy) + (u-sx)*(u-sx);
            if (currentLength2 > best2) continue;

            best2 = currentLength2;

            // manhattan length optimization to narrow down the search area
            int currentLength1 = std::abs(v-sy) + std::abs(u-sx);
            if (currentLength1 < best1) {
              best1 = currentLength1;
              vm = std::min(sheight, sy+currentLength1);
              um = std::min(swidth, sx+currentLength1);
              v = std::max(v, sy-currentLength1);
              u = std::max(u, sx-currentLength1);
            }
          }
        }

        // distance from 0 to 1 to the image edge
        const float unsignedDistance = std::sqrt(float(best2)) / radius;

        // signed distance normalized from 0 to 1
        const float q = (pixelIn ? unsignedDistance : -unsignedDistance) * 0.5f + 0.5f;

        line[tx] = q * 255;
      }
    }
#endif
  }
}
