/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef VALUABLE_VALUEFLAGS_HPP
#define VALUABLE_VALUEFLAGS_HPP

namespace Valuable {

  struct Flags
  {
    const char * name;
    int value;
  };

  /**
   * Valuable Flags, bitmask of enum values.
   *
   * @example
   * @code
   * /// FunnyWidget.hpp
   *
   * struct FunnyWidget {
   *   FunnyWidget();
   *
   *   enum Mode {
   *     ON = 1,
   *     OFF = 0
   *   };
   *
   *   enum InputFlags {
   *     INPUT_MOTION_X = 1 << 1,
   *     INPUT_MOTION_Y = 1 << 2,
   *     INPUT_MOTION_XY = INPUT_MOTION_X | INPUT_MOTION_Y
   *   };
   *
   *   /// m_mode is either ON or OFF
   *   ValueEnum<Mode> m_mode;
   *   /// m_flags is bitwise or of InputFlags values
   *   ValueFlags<InputFlags> m_flags;
   * };
   *
   * //////////////////////////////////////////////////////////////////////////
   *
   * /// FunnyWidget.cpp
   *
   * /// In CSS/Script you can use keywords "on" or "enabled / "off" or "disabled"
   * Valuable::Flags s_modes[] = {"on", Widget::ON, "enabled", Widget::ON,
   *                              "off", Widget::OFF, "disabled", Widget::OFF,
   *                              0, 0};
   *
   * /// In CSS/Script you can write "flags: motion-x | motion-y;"
   * Valuable::Flags s_flags[] = {"motion-x", Widget::INPUT_MOTION_X,
   *                              "motion-y", Widget::INPUT_MOTION_Y,
   *                              "motion-xy", Widget::INPUT_MOTION_XY,
   *                              "INPUT_MOTION_X", Widget::INPUT_MOTION_X,
   *                              "INPUT_MOTION_Y", Widget::INPUT_MOTION_Y,
   *                              "INPUT_MOTION_XY", Widget::INPUT_MOTION_XY,
   *                              0, 0};
   *
   * FunnyWidget::FunnyWidget()
   *  : m_mode(this, "mode", s_modes, ON),
   *    m_flags(this, "flags", s_flags, INPUT_MOTION_XY)
   * {}
   * @endcode
   */
  /*template <typename Enum>
  class ValueFlagsT : ValueInt<int32_t>
  {
  public:
    ValueFlags();
  };*/

}

#endif // VALUABLE_VALUEFLAGS_HPP
