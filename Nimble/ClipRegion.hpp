/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#pragma once

#include "Rect.hpp"

#include <vector>

namespace Nimble
{
  /// Clip region that starts as an axis-aligned rectangle, but can have
  /// arbitrary axis-aligned rectangular parts clipped away using operator-=.
  ///
  /// The clip region is a vector of non-overlapping rectangles.
  ///
  /// This is similar to QRegion, but this uses floats.
  class ClipRegion
  {
  public:
    inline ClipRegion() = default;
    inline ClipRegion(Nimble::Rectf rect);

    inline bool isEmpty() const;
    inline bool contains(const Nimble::Rectf & r) const;
    inline bool intersects(const Nimble::Rectf & r) const;

    inline ClipRegion transformed(Nimble::Matrix3f m) const;
    inline void operator-=(const Nimble::Rectf & eraser);

    inline std::vector<Nimble::Rectf>::const_iterator begin() const;
    inline std::vector<Nimble::Rectf>::const_iterator end() const;

  private:
    std::vector<Nimble::Rectf> m_rects;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ClipRegion::ClipRegion(Nimble::Rectf rect)
  {
    if (!rect.isEmpty())
      m_rects.push_back(rect);
  }

  bool ClipRegion::isEmpty() const
  {
    return m_rects.empty();
  }

  bool ClipRegion::contains(const Rectf & test) const
  {
    ClipRegion tmp(test);
    for (const Rectf & rect: m_rects)
      tmp -= rect;
    return tmp.isEmpty();
  }

  bool ClipRegion::intersects(const Rectf & test) const
  {
    for (const Rectf & rect: m_rects)
      if (rect.intersects(test))
        return true;
    return false;
  }

  ClipRegion ClipRegion::transformed(Nimble::Matrix3f m) const
  {
    ClipRegion ret;
    ret.m_rects = m_rects;
    for (auto & r: ret.m_rects)
      r.transform(m);
    return ret;
  }

  void ClipRegion::operator-=(const Nimble::Rectf & eraser)
  {
    for (size_t s = 0, c = m_rects.size(); s < c;) {
      Nimble::Rectf & subrect = m_rects[s];

      // This subrect fully removed
      if (eraser.contains(subrect)) {
        m_rects.erase(m_rects.begin() + s);
        --c;
        continue;
      }

      Nimble::Rectf intersection = eraser.intersection(subrect);
      if (intersection.isEmpty()) {
        ++s;
        continue;
      }

      bool first = true;

      Nimble::Rectf tmp = subrect;
      if (intersection.low().x > tmp.low().x) {
        if (first) {
          subrect.setHighX(intersection.low().x);
          first = false;
        } else {
          m_rects.push_back(tmp);
          m_rects.back().setHighX(intersection.low().x);
        }
        tmp.setLowX(intersection.low().x);
      }

      if (intersection.low().y > tmp.low().y) {
        if (first) {
          subrect.setHighY(intersection.low().y);
          first = false;
        } else {
          m_rects.push_back(tmp);
          m_rects.back().setHighY(intersection.low().y);
        }
        tmp.setLowY(intersection.low().y);
      }

      if (intersection.high().x < tmp.high().x) {
        if (first) {
          subrect.setLowX(intersection.high().x);
          first = false;
        } else {
          m_rects.push_back(tmp);
          m_rects.back().setLowX(intersection.high().x);
        }
        tmp.setHighX(intersection.high().x);
      }

      if (intersection.high().y < tmp.high().y) {
        if (first) {
          subrect.setLowY(intersection.high().y);
        } else {
          m_rects.push_back(tmp);
          m_rects.back().setLowY(intersection.high().y);
        }
      }
      ++s;
    }
  }

  std::vector<Nimble::Rectf>::const_iterator ClipRegion::begin() const
  {
    return m_rects.begin();
  }

  std::vector<Nimble::Rectf>::const_iterator ClipRegion::end() const
  {
    return m_rects.end();
  }
}
