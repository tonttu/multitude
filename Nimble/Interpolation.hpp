/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_INTERPOLATION_HPP
#define NIMBLE_INTERPOLATION_HPP

#include <vector>
#include <algorithm>

namespace Nimble
{

  /// Performs linear interpolation of non-uniform samples
  template<class T>
  class LinearInterpolator
  {
  public:
    /// Type of key
    typedef std::pair<float, T> Key;
    /// Array of keys
    typedef std::vector<Key> Keys;

    /// Constructs an empty interpolator
    LinearInterpolator() {}

    /// Add a key to the interpolation
    /// @param t position to add the value
    /// @param value value
    void addKey(float t, T value) {

      bool sort = false;
      if(!m_keys.empty() && m_keys.back().first > t)
        sort = true;

      m_keys.push_back(Key(t, value));
      if(sort)
        std::sort(m_keys.begin(), m_keys.end());
    }

    /// Return the interpolated value at given position
    /// @param t position to interpolate at
    /// @return interpolated value
    T interpolate(float t) const
    {
      assert(!m_keys.empty());

      typename Keys::const_iterator i = std::lower_bound(m_keys.begin(), m_keys.end(), std::make_pair(t, T()), funky_compare);

      if(i == m_keys.end())
        return m_keys.back().second;
      else if(i == m_keys.begin())
        return m_keys.front().second;

      const Key & a = *(i - 1);
      const Key & b = *(i - 0);

      float tt = (t - a.first) / (b.first - a.first);

      return a.second * (1.f - tt) + b.second * tt;
    }

    /// Gets a reference to the key-point list
    const Keys & keys() const { return m_keys; }
    /// Remove all key-points
    void clear() { m_keys.clear(); }

  private:
    // Compare float to pair with float in the first field
    static bool funky_compare(const Key & a, const Key & b) { return a.first < b.first; }

    Keys m_keys;
  };

}

#endif
