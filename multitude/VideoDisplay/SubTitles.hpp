/* COPYRIGHT
 *
 * This file is part of VideoDisplay.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "VideoDisplay.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */


#ifndef VIDEODISPLAY_SUBTITLES_HPP
#define VIDEODISPLAY_SUBTITLES_HPP

#include <Radiant/TimeStamp.hpp>

#include "Export.hpp"

#include <string>
#include <vector>

namespace VideoDisplay {

  /// Subtitles for the videos
  class VIDEODISPLAY_API SubTitles
  {
  public:

    /** Subtitle text item. This class contains information about the
    sub-titles that should be displayed at the moment. */
    class Text
    {
    public:

      /// Returns the number of lines in this text item.
      int lineCount() const
      {
        int n = 0;
        if(!m_lines[0].empty()) {
          n = 1;
          if(!m_lines[1].empty())
            n = 2;
        }

        return n;
      }

      /// The subtitle lines
      std::vector<std::string> m_lines;
      /// The earliest time-stamp when one should display this subtitle item
      Radiant::TimeStamp m_begin;
      /// The latest time-stamp when one should display this subtitle item
      Radiant::TimeStamp m_end;
    };

    SubTitles();
    ~SubTitles();

    /// Update the text to be shown to the user
    void update(Radiant::TimeStamp time);
    /// Returns the current text item to be shown
    const Text * current();

    /// Read subtitles from an SRT file
    /// @param filename SRT file name
    /// @return Returns true if the SRT file was successfully laoded, false on
    ///         failure.
    bool readSrt(const char * filename);

    /// Returns the number of subtitle items
    size_t size() const { return m_texts.size(); }

    /// Returns the longest subtitle string
    std::string getLongestSubtitle() const;

  private:
    typedef std::vector<Text> Texts;
    Texts m_texts;
    Text * m_current;
    int m_index;
  };

}

#endif

