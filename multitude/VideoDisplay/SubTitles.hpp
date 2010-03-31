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

#include <VideoDisplay/Export.hpp>

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
      
      int lineCount() const
      {
	int n = 0;
	if(m_lines[0].size()) {
	  n = 1;
	  if(m_lines[1].size())
	    n = 2;
	}

	return n;
      }

      std::vector<std::string> m_lines;
      Radiant::TimeStamp m_begin;
      Radiant::TimeStamp m_end;
    };
    
    SubTitles();
    ~SubTitles();

    void update(Radiant::TimeStamp time);
    const Text * current();

    bool readSrt(const char * filename); 

    unsigned size() const { return m_texts.size(); }
 
    // Temporary for a project (esa) 
    std::string getLongestSubtitle() const;
 
  private:
    typedef std::vector<Text> Texts; 
    Texts m_texts;
    Text * m_current;
    int m_index;
  };
  
}

#endif

