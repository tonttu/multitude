#include "DistanceFieldGenerator.hpp"

#include "Image.hpp"

namespace Luminous
{
  void DistanceFieldGenerator::generate(const Luminous::Image & src, Nimble::Vector2i srcSize, Luminous::Image & target, int radius)
  {
    const int sheight = srcSize.y, swidth = srcSize.x;
    const int theight = target.height(), twidth = target.width();
    const Nimble::Vector2f scale(float(swidth)/twidth, float(sheight)/theight);

    assert(src.pixelFormat().bytesPerPixel() == 1);
    assert(target.pixelFormat().bytesPerPixel() == 1);

    // Iterate all pixels in the target image
    for (int ty = 0; ty < theight; ++ty) {
      unsigned char * line = target.line(ty);
      const int sy = std::round(scale.y * ty);

      for (int tx = 0; tx < twidth; ++tx) {
        const int sx = std::round(scale.x * tx);

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
  }
}
