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
    typedef std::vector<Key > Keys;

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
      typename Keys::const_iterator i = std::lower_bound(m_keys.begin(), m_keys.end(), t, funky_compare);

      const Key & a = *(i - 1);
      const Key & b = *(i - 0);

      float tt = (t - a.first) / (b.first - a.first);

      return a.second * (1.f - tt) + b.second * tt;
    }

  private:
    // Compare float to pair with float in the first field
    static bool funky_compare(const Key & a, float b) { return a.first < b; }

    Keys m_keys;
  };

}

#endif
