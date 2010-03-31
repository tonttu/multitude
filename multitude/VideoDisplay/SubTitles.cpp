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

#include "SubTitles.hpp"

#include <Radiant/Trace.hpp>
#include <Radiant/StringUtils.hpp>

#include <fstream>

#include <string.h>

namespace VideoDisplay {

  using namespace Radiant;

  SubTitles::SubTitles()
    : m_current(0),
    m_index(0)
  {}

  SubTitles::~SubTitles()
  {}

  void SubTitles::update(Radiant::TimeStamp time)
  {
    if(!m_texts.size()) {
      m_current = 0;
      return;
    }

    if(m_index >= (int) m_texts.size()) {
      m_index = m_texts.size() - 1;
    }
    else if(m_index < 0)
      m_index = 0;

    while(m_index >= 0 && m_texts[m_index].m_begin > time)
      m_index--;
    while(m_index >= 0 && m_index < (int) m_texts.size() && 
        m_texts[m_index].m_end < time)
      m_index++;


    if(m_index < 0 || m_index >= (int) m_texts.size()) {
      m_current = 0;
      return;
    }

    Text & text = m_texts[m_index];

    if(text.m_begin <= time && time <= text.m_end) {
      m_current = & text;
    }
    else
      m_current = 0;
  }

  static bool nextLine(std::ifstream & in, char * buf, int maxn)
  {
    while(in.good()) {
      buf[0] = 0;
      buf[1] = 0;

      in.getline(buf, maxn);

      if(strlen(buf) != 0 && buf[0] > 23)
        return true;
    }

    return false;
  }

  static bool readTime(const char * text, Radiant::TimeStamp & time)
  {
    int h = -1;
    int m = 0;
    int s = 0;
    int ms = 0;

    if(sscanf(text, "%d:%d:%d,%d", & h, & m, & s, & ms) != 4) {
      // Radiant::trace("Got non-time: %d %d %d %d (%s)", h, m, s, ms, text);
      return false;
    }

    // Radiant::trace("Got time: %d %d %d %d", h, m, s, ms);

    time = Radiant::TimeStamp::createDHMS(0, h, m, s) + 
      Radiant::TimeStamp::createSecondsD(ms * 0.001);

    return h >= 0;
  }

  bool SubTitles::readSrt(const char * filename)
  {
    const int LEN = 1024;
    int index = 1;

    std::ifstream in;

    m_texts.clear();

    in.open(filename);

    char buf[LEN];

    m_texts.clear();
    m_index = 0;
    m_current = 0;

    int errors = 0;

    while (in.good()) {
      // First we get the text chunk index:
      if(!nextLine(in, buf, LEN))
        break;

      int readIndex = atoi(buf);

      if(readIndex != index) {
        Radiant::error(
"SubTitles::readSrt # Wrong chunk index (%d) %d != %d",
            (int) buf[0], readIndex, index);
        errors++;
        continue;
      }
      index++;

      // Now we get the timing information
      if(!nextLine(in, buf, LEN))
        break;

      Radiant::StringUtils::StringList list;

      Radiant::StringUtils::split(buf, " ", list);

      if(list.size() != 3) {
        Radiant::error(
"SubTitles::readSrt # Wrong time format \"%s\"",
            buf);
        errors++;
        continue;
      }

      std::string t1 = list.front();
      std::string t2 = list.back();

      Text tmp;

      if(!readTime(t1.c_str(), tmp.m_begin))
        errors++;

      if(!readTime(t2.c_str(), tmp.m_end))
        errors++;

      int foo = 0;
      bool r = true;
      while(r && foo++ < 20) {
        buf[0] = 0;

        in.getline(buf, LEN);

        bool r = (buf[0] && buf[0] != '\n');
        //Radiant::debug("SUB READ %s (buf[0] = %d, r = %d)", buf, buf[0], r);
        if(r) tmp.m_lines.push_back(buf);
        else break;
      } 

      // These will remove unicode characters, so don't do this
      //Radiant::StringUtils::eraseNonVisibles(tmp.m_lines[0]);
      //Radiant::StringUtils::eraseNonVisibles(tmp.m_lines[1]);

      Radiant::debug("Subtitle chunk %lf -> %lf %d lines",
        tmp.m_begin.secondsD(), tmp.m_end.secondsD(),
        tmp.m_lines.size());
        
      m_texts.push_back(tmp);

      if(errors > 10)
        return false;
    }

    if(!m_texts.empty())
      info("Loaded subtitles with %d items", (int) m_texts.size());

    return m_texts.size() != 0 && errors < 10;
  }

  const SubTitles::Text * SubTitles::current()
  {
    return m_current;
  }

  std::string SubTitles::getLongestSubtitle() const
  {
    size_t longest = 0;
    size_t index = 0;
    std::string full;

    for(size_t i = 0; i < m_texts.size(); i++) { 
      const Text & text = m_texts[i];

      full = text.m_lines[0];

      for(size_t j = 1; j < text.m_lines.size(); j++)
        full += '\n' + text.m_lines[j];
      size_t len = full.size();

      if(len > longest) {
        longest = len;
        index = i;
      }
    }

    full = m_texts[index].m_lines[0];
    for(size_t j = 1; j < m_texts[index].m_lines.size(); j++) {
      full += '\n' + m_texts[index].m_lines[j];
    }

    Radiant::debug("LONGEST SUB %s", full.c_str());
    return full;
  }

}

