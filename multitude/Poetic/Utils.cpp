/* COPYRIGHT
 */

#include "Utils.hpp"

namespace Poetic
{

  using namespace Radiant;
  using namespace StringUtils;

#if 0

  void Utils::breakToLines(const std::wstring & ws, const float width,
                           CPUFont & fnt, WStringList & lines, const bool afterSpace)
  {
    // Ensure line list empty
    /*
      info("breakToLines Comes line with size %d", (int) ws.size());
      for(unsigned i = 0; i < ws.size(); i++) {
    printf("%d ", (int) ws[i]);
      }

      printf("\n");
      */
    lines.clear();

    if(ws.empty() || width <= 0.0f) {
      return;
    }

    // First break the wstring at newlines

    std::wstring  delim;
    delim = W_NEWLINE;
    WStringList   wSub;

    split(ws, delim, wSub);

    // Now add the newlines we just removed:

    /*for(WStringList::iterator itSub = wSub.begin(); itSub != wSub.end(); ) {

    std::wstring & str = *itSub;

    itSub++;

    if(itSub == wSub.end()) {
      if(ws[ws.size() - 1] == W_NEWLINE)
        str += W_NEWLINE;
    }
    else {
      str += W_NEWLINE;
    }
      }
      */
    // Break the resulting sub-wstrings to fit width

    delim = std::wstring(L" ");

    for(WStringList::iterator itSub = wSub.begin(); itSub != wSub.end(); itSub++)
    {
      // Split the sub-string into words

      WStringList words;
      split(* itSub, delim, words, afterSpace);

      // Make the lines
      int tries = 0;
      // 10 tries per character ought to be enough
      ///@todo fix for real, this probably didn't terminate if width < one character
      int maxTries = ws.size() * 10;
      while(words.size() && tries < maxTries)
      {
        ++tries;

        // First try to fit it in as separate words

        const int   numWords = words.size();
        bool  got = false;
        BBox  bBox;
        for(int i = numWords; i >= 1 && !got; i--)
        {
          std::wstring  ln;
          WStringList::iterator   itWord = words.begin();
          for(int j = 0; j < i; j++, itWord++)
          {
            ln += * itWord;
          }
          fnt.bbox((wchar_t *)(ln.data()), bBox);
          if(bBox.width() <= width)
          {
            lines.push_back(ln);
            for(int j = 0; j < i; j++)
            {
              words.pop_front();
            }
            got = true;
          }
        }

        if(got)
        {
          continue;
        }

        // Truncate the overlong word to fit width

        const std::wstring  word = words.front();
        const int   numChars = word.length();
        for(int i = numChars - 1; i >= 1 && !got; i--)
        {
          const std::wstring  ln = word.substr(0, i + 1);
          fnt.bbox((wchar_t *)(ln.data()), bBox);
          if(bBox.width() <= width)
          {
            lines.push_back(ln);
            words.pop_front();
            words.push_front(word.substr(i + 1));
            got = true;
          }
        }
      }
    }

    // If last character is newline append empty line

    if(ws[ws.length() - 1] == W_NEWLINE) {
      lines.push_back(std::wstring(L""));
    }
    /*
      for(WStringList::iterator itSub = lines.begin(); itSub != lines.end(); itSub++) {

    info("breakToLines LINELEN = %d", (int) (*itSub).length());

    for(unsigned i = 0; i < (*itSub).length(); i++) {
      printf("%d ", (int) (*itSub)[i]);
    }

    printf("\n");

      }
      */
  }

#else

  void Utils::breakToLines(const std::wstring & ws, float width,
                           CPUFont & fnt, WStringList & lines, bool afterSpace)
  {
    lines.clear();

    if(ws.empty())
      return;

    int n = ws.size();

    std::vector<float> advances;
    advances.resize(n, 0);

    fnt.advanceList(ws.c_str(), & advances[0], n);

    int lineStart = 0;
    int okEnd = 0;
    float sum = 0.0f;
    float okEndSum = 0.0f;

    bool onspace = false;

    for(int i = 0; i < n; i++) {

      int c = ws[i];
      float a = advances[i];

      sum += a;

      // info("brea to lines: %d:%c %f vs %f", i, (char) c, sum, width);

      if(sum > width) {

        if(okEnd > 0) {
          lines.push_back(std::wstring( & ws[lineStart], okEnd - lineStart));
          sum = sum - okEndSum;
          lineStart = okEnd;
        }
        else if(i > (lineStart + 1)) {
          // The word does not fit line, split the word
          lines.push_back(std::wstring( & ws[lineStart], i - lineStart - 1));
          sum = a;
          lineStart = i - 1;
        }
        else {
          // The first letter does not fit the line, take just the first letter
          lines.push_back(std::wstring( & ws[lineStart], 1));
          sum = 0;
          lineStart++;
        }
        onspace = false;

        okEnd = 0;
      }
      else {
        if(c == ' ' || c == '.' || c == ',' || c == '!') {
          onspace = true;
        }
        else if(c == W_NEWLINE) {
          lines.push_back(std::wstring( & ws[lineStart], i - lineStart));
          sum = 0.0f;
          okEnd = 0;
          lineStart = i+1;
          onspace = false;
        }
        else if(onspace) {
          // End of word, now we have a good place for splitting the word, lets mark this down.
          okEnd = i;
          okEndSum = sum;
          onspace = false;
        }
      }
    }

    if(n != lineStart) {
      lines.push_back(std::wstring( & ws[lineStart], n - lineStart));
    }
  }


#endif


void Utils::split(const std::wstring & ws, const std::wstring & delim,
                  WStringList & out, const bool afterDelim)
{
  out.clear();

  if(ws.empty())
  {
    return;
  }

  // Find first a delimiter

  std::wstring  wsCopy(ws);
  size_t  pos = wsCopy.find_first_of(delim);

  // Loop until no delimiters left

  if(afterDelim)
    // split string after delimiter
  {
    while(pos != wsCopy.npos)
    {
      out.push_back(wsCopy.substr(0, pos + 1));
      wsCopy.erase(0, pos + 1);
      pos = wsCopy.find_first_of(delim);
    }
  }
  else
    // split string before delimiter
  {
    while(pos != wsCopy.npos)
    {
      out.push_back(wsCopy.substr(0, pos));
      wsCopy.erase(0, pos);
      pos = wsCopy.find_first_of(delim, 1);
    }
  }

  // Push remainder of wstring onto list

  if(!wsCopy.empty())
  {
    out.push_back(wsCopy);
  }
}


}
