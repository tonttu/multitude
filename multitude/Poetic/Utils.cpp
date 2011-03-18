/* COPYRIGHT
 *
 * This file is part of Poetic.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Poetic.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "Utils.hpp"

#include <QStringList>

namespace Poetic
{

    using namespace Radiant;

    void Utils::breakToLines(const QString & ws, const float width,
      CPUFont & fnt, QStringList & lines, const bool afterSpace)
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

      if(ws.isEmpty() || width <= 0.0f) {
        return;
      }

      // First break the wstring at newlines

      QStringList wSub = ws.split(" ", QString::SkipEmptyParts);

      // Now add the newlines we just removed:

      /*for(WStringList::iterator itSub = wSub.begin(); itSub != wSub.end(); ) {
	
	QString & str = *itSub;
	
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

      foreach(QString itSub, wSub) {
        // Split the sub-string into words

        /// @todo this afterSpace Skip/Keep thingy might be totally wrong
        QStringList words = itSub.split(" ", afterSpace ? QString::SkipEmptyParts
                                                        : QString::KeepEmptyParts);
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
            QString  ln;
            for(int j = 0; j < i; j++)
            {
              ln += words[j];
            }
            std::wstring tmp = ln.toStdWString();
            fnt.bbox(tmp.data(), bBox);
            if(bBox.width() <= width)
            {
              lines << ln;
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

          const QString  word = words.front();
          const int   numChars = word.length();
          for(int i = numChars - 1; i >= 1 && !got; i--)
          {
            const QString  ln = word.left(i+1);
            std::wstring tmp = ln.toStdWString();
            fnt.bbox(tmp.data(), bBox);
            if(bBox.width() <= width)
            {
              lines.push_back(ln);
              words.pop_front();
              words.push_front(word.mid(i + 1));
              got = true;
            }
          }
        }
      }

      // If last character is newline append empty line

      if(ws[ws.length() - 1] == W_NEWLINE) {
        lines.push_back("");
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
}
