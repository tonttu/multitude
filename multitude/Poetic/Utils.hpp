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

#ifndef POETIC_UTILS_HPP
#define POETIC_UTILS_HPP

#include <Poetic/CPUBitmapFont.hpp>
#include <Poetic/Export.hpp>

#include <Radiant/StringUtils.hpp>

namespace Poetic
{

  /** Utility functions for the Poetic font rendering engine. */

  namespace Utils
  {

    /// Zero-width space character is used as new line character.
    //enum { W_NEWLINE = 0x200B };
    enum { W_NEWLINE = 10 };
    /**
      * @brief Break wstring into lines.
      * The lines will be less than or equal to the specified width when displayed
      * in the given font.
      * @param ws The string to be broken.
      * @param width Maximum width of line.
      * @param bitmapFont Font used for rendering.
      * @param lines Reference to list to receive the lines.
      * @note Newline characters are retained in the output.
      * @param afterSpace true to break lines after inter-word spaces.
      */
    /// @todo move inside CPUFont
    void POETIC_API breakToLines(const std::wstring & ws, const float width,
      CPUFont & bitmapFont, Radiant::StringUtils::WStringList & lines,
      const bool afterSpace = true);

    /**
      * @brief Tokenize wstring.
      * @param ws The string to be tokenized.
      * @param delim One or more delimiter characters.
      * @param out Reference to list to receive the tokens.
      * @param afterDelim true to split string after delimiter.
      */
    /// @todo Remove, the same is in Radiant::StringUtils
    void POETIC_API split(const std::wstring & ws, const std::wstring & delim,
      Radiant::StringUtils::WStringList & out, const bool afterDelim = true);

  }

}

#endif
