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
    LUMINOUS_API RichTextLayout(const Nimble::SizeF & size = Nimble::SizeF(100, 100));
    LUMINOUS_API virtual ~RichTextLayout();

    LUMINOUS_API RichTextLayout(RichTextLayout && t);
    LUMINOUS_API RichTextLayout & operator=(RichTextLayout && t);

    LUMINOUS_API QTextDocument & document();
    LUMINOUS_API const QTextDocument & document() const;

    /// Sets list bullet that is used in front of list elements
    /// @param bullet Bullet string for the list elements
    LUMINOUS_API void setListBullet(const QString& bullet);
    /// Returns the string used as bullet for list elements
    /// @return String used as bullet for list elements
    LUMINOUS_API const QString& listBullet() const;

  private:
    LUMINOUS_API virtual void generateInternal() const OVERRIDE;

    class D;
    D * m_d;
  };
} // namespace Luminous

#endif
