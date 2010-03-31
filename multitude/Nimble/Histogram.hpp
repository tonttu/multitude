/* COPYRIGHT
 *
 * This file is part of Nimble.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Nimble.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef NIMBLE_HISTOGRAM_HPP
#define NIMBLE_HISTOGRAM_HPP

#include <Nimble/Export.hpp>

#include <string>

namespace Nimble {

  /// Histogram calculation
  /** This class can be used to calculate histograms of all kinds of
    values.

    Typical use pattern is as follows:

    \code
    Histogram<unsigned int,256> hist;
    hist.clear();
    for(...)
      hist.put(myFunction());

    unsigned median = hist.getLowValueRelative(0.5);
    \endcode

    @author Tommi Ilmonen */

  template <class T, int N> class NIMBLE_API Histogram
  {
  public:
    /** Constructs an empty histogram object.
        The histogram is needs to be cleared before use, by calling #clear(). */
    Histogram() : m_count(0) {}
    ~Histogram() {}

    /// Sets all histogram bins to zero
    void clear() { memset(m_data, 0, sizeof(m_data)); m_count = 0; }

    /// Adds the histogram bin "index" by one
    void put(int index) { m_data[index]++; m_count++; }

    /// Adds the histogram bin "index" by one
    /** It is ok to exceed the array towards top - i.e. index exceeds
          the histogram bin count. */
    void putSafe(int index)
    {
      if(index < N)
        m_data[index]++;
      else
        m_data[N - 1]++;
      m_count++;
    }

    /** It is ok to exceed the array towards any direction. */
    void putSafest(int index)
    {
      if(index >= N)
        m_data[N - 1]++;
      else if(index < 0)
        m_data[0]++;
      else
        m_data[index]++;

      m_count++;
    }

    /// Find the bin below which there are required number of samples
    /** @arg ratio relative ratio for selection (0-1). */
    int getLowValueRelative(float ratio) const
    { return getLowValueFixed((int) (ratio * m_count)); }

    /// Find the bin above which there are required number of samples
    /** @arg ratio relative ratio for selection (0-1). */
    int getHighValueRelative(float ratio) const
    { return getHighValueFixed((int) (ratio * m_count)); }

    /// Find the bin below which there are required number of samples
    /** @arg need required sum of bins (0-count()). */
    int getLowValueFixed(int need) const
    {
      int count = 0;
      for(int i = 0; i < N ; i++) {
        count += m_data[i];
        if(count >= need)
          return i;
      }
      return N - 1;
    }

    /// Find the bin above which there are required number of samples
    /** @arg need required sum of bins (0-count()). */
    int getHighValueFixed(int need) const
    {
      int count = 0;
      for(int i = N - 1; i >= 0 ; i--) {
        count += m_data[i];
        if(count >= need)
          return i;
      }
      return 0;
    }

    /// Add the values from another histogram to this histogram
    void add(const Histogram & that)
    {
      for(int i = 0; i < N; i++)
        m_data[i] += that.m_data[i];
      m_count += that.m_count;
    }

    /// Returns the sum of all bin counts
    int count() const { return m_count; }

    /// Returns the bin with the largest number of hits
    int largestBin() const
    {
      T high = m_data[0];
      int index = 0;

      for(int i = 1; i < N; i++) {
        T tmp = m_data[i];
        if(high < tmp) {
          high = tmp;
          index = i;
        }
      }

      return index;
    }

    /// Returns the lowest bin that is not empty
    /**

        \code
        Histogram<int, 256> hist;
        hist.put(134);
        hist.put(254);
        hist.put(3);
        int low = hist.lowestNonEmpty(); // returns 3.
        \endcode

      */
    int lowestNonEmpty() const
    {
      for(int i = 1; i < N; i++) {
        if(m_data[i])
          return i;
      }
      return N-1;
    }

    /// Returns the highest lowest bin that is not empty
    /// @see #lowestNonEmpty()
    int highestNonEmpty() const
    {
      for(int i = N - 1; i > 0; i--) {
        if(m_data[i])
          return i;
      }
      return 0;
    }

    /// Returns a reference to a given bin
    T & operator [] (int i) { return m_data[i]; }
    /// Returns a const reference to a given bin
    const T & operator [] (int i) const { return m_data[i]; }

  private:

    T   m_data[N];
    int m_count;
  };

  /// Histogram of unsigned integers
  typedef Histogram<unsigned int,256> Histogramu256;
  /// Histogram of signed integers
  typedef Histogram<int,256> Histogrami256;

} // namespace

#endif
