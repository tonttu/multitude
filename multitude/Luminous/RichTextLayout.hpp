/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_RICH_TEXT_LAYOUT_HPP
#define LUMINOUS_RICH_TEXT_LAYOUT_HPP

#include "TextLayout.hpp"

#include <Patterns/NotCopyable.hpp>

class QTextDocument;

namespace Luminous
{
  /// Rich text document layout
  class RichTextLayout : public TextLayout
  {
  public:
    LUMINOUS_API RichTextLayout(const Nimble::Vector2f & size = Nimble::Vector2f(100, 100));
    LUMINOUS_API virtual ~RichTextLayout();

    LUMINOUS_API RichTextLayout(RichTextLayout && t);
    LUMINOUS_API RichTextLayout & operator=(RichTextLayout && t);

    LUMINOUS_API virtual void generateInternal() const OVERRIDE;

    LUMINOUS_API QTextDocument & document();
    LUMINOUS_API const QTextDocument & document() const;

  private:
    class D;
    D * m_d;
  };
} // namespace Luminous

#endif
