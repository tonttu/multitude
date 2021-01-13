/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Color.hpp"
#include "Trace.hpp"
#include "ColorUtils.hpp"
#include "Thread.hpp"

#include <QMap>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const QMap<QByteArray, Radiant::Color> & colors()
{
  /* CSS Color Module Level 3 - Extended color keywords */
  /* Also the same as SVG 1.0 color keywords */
  static QMap<QByteArray, Radiant::Color> s_colors;
  MULTI_ONCE {
    s_colors["aliceblue"] = Radiant::Color::fromRGBA(240, 248, 255);
    s_colors["antiquewhite"] = Radiant::Color::fromRGBA(250, 235, 215);
    s_colors["aqua"] = Radiant::Color::fromRGBA(0, 255, 255);
    s_colors["aquamarine"] = Radiant::Color::fromRGBA(127, 255, 212);
    s_colors["azure"] = Radiant::Color::fromRGBA(240, 255, 255);
    s_colors["beige"] = Radiant::Color::fromRGBA(245, 245, 220);
    s_colors["bisque"] = Radiant::Color::fromRGBA(255, 228, 196);
    s_colors["black"] = Radiant::Color::fromRGBA(0, 0, 0);
    s_colors["blanchedalmond"] = Radiant::Color::fromRGBA(255, 235, 205);
    s_colors["blue"] = Radiant::Color::fromRGBA(0, 0, 255);
    s_colors["blueviolet"] = Radiant::Color::fromRGBA(138, 43, 226);
    s_colors["brown"] = Radiant::Color::fromRGBA(165, 42, 42);
    s_colors["burlywood"] = Radiant::Color::fromRGBA(222, 184, 135);
    s_colors["cadetblue"] = Radiant::Color::fromRGBA(95, 158, 160);
    s_colors["chartreuse"] = Radiant::Color::fromRGBA(127, 255, 0);
    s_colors["chocolate"] = Radiant::Color::fromRGBA(210, 105, 30);
    s_colors["coral"] = Radiant::Color::fromRGBA(255, 127, 80);
    s_colors["cornflowerblue"] = Radiant::Color::fromRGBA(100, 149, 237);
    s_colors["cornsilk"] = Radiant::Color::fromRGBA(255, 248, 220);
    s_colors["crimson"] = Radiant::Color::fromRGBA(220, 20, 60);
    s_colors["cyan"] = Radiant::Color::fromRGBA(0, 255, 255);
    s_colors["darkblue"] = Radiant::Color::fromRGBA(0, 0, 139);
    s_colors["darkcyan"] = Radiant::Color::fromRGBA(0, 139, 139);
    s_colors["darkgoldenrod"] = Radiant::Color::fromRGBA(184, 134, 11);
    s_colors["darkgray"] = Radiant::Color::fromRGBA(169, 169, 169);
    s_colors["darkgreen"] = Radiant::Color::fromRGBA(0, 100, 0);
    s_colors["darkgrey"] = Radiant::Color::fromRGBA(169, 169, 169);
    s_colors["darkkhaki"] = Radiant::Color::fromRGBA(189, 183, 107);
    s_colors["darkmagenta"] = Radiant::Color::fromRGBA(139, 0, 139);
    s_colors["darkolivegreen"] = Radiant::Color::fromRGBA(85, 107, 47);
    s_colors["darkorange"] = Radiant::Color::fromRGBA(255, 140, 0);
    s_colors["darkorchid"] = Radiant::Color::fromRGBA(153, 50, 204);
    s_colors["darkred"] = Radiant::Color::fromRGBA(139, 0, 0);
    s_colors["darksalmon"] = Radiant::Color::fromRGBA(233, 150, 122);
    s_colors["darkseagreen"] = Radiant::Color::fromRGBA(143, 188, 143);
    s_colors["darkslateblue"] = Radiant::Color::fromRGBA(72, 61, 139);
    s_colors["darkslategray"] = Radiant::Color::fromRGBA(47, 79, 79);
    s_colors["darkslategrey"] = Radiant::Color::fromRGBA(47, 79, 79);
    s_colors["darkturquoise"] = Radiant::Color::fromRGBA(0, 206, 209);
    s_colors["darkviolet"] = Radiant::Color::fromRGBA(148, 0, 211);
    s_colors["deeppink"] = Radiant::Color::fromRGBA(255, 20, 147);
    s_colors["deepskyblue"] = Radiant::Color::fromRGBA(0, 191, 255);
    s_colors["dimgray"] = Radiant::Color::fromRGBA(105, 105, 105);
    s_colors["dimgrey"] = Radiant::Color::fromRGBA(105, 105, 105);
    s_colors["dodgerblue"] = Radiant::Color::fromRGBA(30, 144, 255);
    s_colors["firebrick"] = Radiant::Color::fromRGBA(178, 34, 34);
    s_colors["floralwhite"] = Radiant::Color::fromRGBA(255, 250, 240);
    s_colors["forestgreen"] = Radiant::Color::fromRGBA(34, 139, 34);
    s_colors["fuchsia"] = Radiant::Color::fromRGBA(255, 0, 255);
    s_colors["gainsboro"] = Radiant::Color::fromRGBA(220, 220, 220);
    s_colors["ghostwhite"] = Radiant::Color::fromRGBA(248, 248, 255);
    s_colors["gold"] = Radiant::Color::fromRGBA(255, 215, 0);
    s_colors["goldenrod"] = Radiant::Color::fromRGBA(218, 165, 32);
    s_colors["gray"] = Radiant::Color::fromRGBA(128, 128, 128);
    s_colors["green"] = Radiant::Color::fromRGBA(0, 128, 0);
    s_colors["greenyellow"] = Radiant::Color::fromRGBA(173, 255, 47);
    s_colors["grey"] = Radiant::Color::fromRGBA(128, 128, 128);
    s_colors["honeydew"] = Radiant::Color::fromRGBA(240, 255, 240);
    s_colors["hotpink"] = Radiant::Color::fromRGBA(255, 105, 180);
    s_colors["indianred"] = Radiant::Color::fromRGBA(205, 92, 92);
    s_colors["indigo"] = Radiant::Color::fromRGBA(75, 0, 130);
    s_colors["ivory"] = Radiant::Color::fromRGBA(255, 255, 240);
    s_colors["khaki"] = Radiant::Color::fromRGBA(240, 230, 140);
    s_colors["lavender"] = Radiant::Color::fromRGBA(230, 230, 250);
    s_colors["lavenderblush"] = Radiant::Color::fromRGBA(255, 240, 245);
    s_colors["lawngreen"] = Radiant::Color::fromRGBA(124, 252, 0);
    s_colors["lemonchiffon"] = Radiant::Color::fromRGBA(255, 250, 205);
    s_colors["lightblue"] = Radiant::Color::fromRGBA(173, 216, 230);
    s_colors["lightcoral"] = Radiant::Color::fromRGBA(240, 128, 128);
    s_colors["lightcyan"] = Radiant::Color::fromRGBA(224, 255, 255);
    s_colors["lightgoldenrodyellow"] = Radiant::Color::fromRGBA(250, 250, 210);
    s_colors["lightgray"] = Radiant::Color::fromRGBA(211, 211, 211);
    s_colors["lightgreen"] = Radiant::Color::fromRGBA(144, 238, 144);
    s_colors["lightgrey"] = Radiant::Color::fromRGBA(211, 211, 211);
    s_colors["lightpink"] = Radiant::Color::fromRGBA(255, 182, 193);
    s_colors["lightsalmon"] = Radiant::Color::fromRGBA(255, 160, 122);
    s_colors["lightseagreen"] = Radiant::Color::fromRGBA(32, 178, 170);
    s_colors["lightskyblue"] = Radiant::Color::fromRGBA(135, 206, 250);
    s_colors["lightslategray"] = Radiant::Color::fromRGBA(119, 136, 153);
    s_colors["lightslategrey"] = Radiant::Color::fromRGBA(119, 136, 153);
    s_colors["lightsteelblue"] = Radiant::Color::fromRGBA(176, 196, 222);
    s_colors["lightyellow"] = Radiant::Color::fromRGBA(255, 255, 224);
    s_colors["lime"] = Radiant::Color::fromRGBA(0, 255, 0);
    s_colors["limegreen"] = Radiant::Color::fromRGBA(50, 205, 50);
    s_colors["linen"] = Radiant::Color::fromRGBA(250, 240, 230);
    s_colors["magenta"] = Radiant::Color::fromRGBA(255, 0, 255);
    s_colors["maroon"] = Radiant::Color::fromRGBA(128, 0, 0);
    s_colors["mediumaquamarine"] = Radiant::Color::fromRGBA(102, 205, 170);
    s_colors["mediumblue"] = Radiant::Color::fromRGBA(0, 0, 205);
    s_colors["mediumorchid"] = Radiant::Color::fromRGBA(186, 85, 211);
    s_colors["mediumpurple"] = Radiant::Color::fromRGBA(147, 112, 219);
    s_colors["mediumseagreen"] = Radiant::Color::fromRGBA(60, 179, 113);
    s_colors["mediumslateblue"] = Radiant::Color::fromRGBA(123, 104, 238);
    s_colors["mediumspringgreen"] = Radiant::Color::fromRGBA(0, 250, 154);
    s_colors["mediumturquoise"] = Radiant::Color::fromRGBA(72, 209, 204);
    s_colors["mediumvioletred"] = Radiant::Color::fromRGBA(199, 21, 133);
    s_colors["midnightblue"] = Radiant::Color::fromRGBA(25, 25, 112);
    s_colors["mintcream"] = Radiant::Color::fromRGBA(245, 255, 250);
    s_colors["mistyrose"] = Radiant::Color::fromRGBA(255, 228, 225);
    s_colors["moccasin"] = Radiant::Color::fromRGBA(255, 228, 181);
    s_colors["navajowhite"] = Radiant::Color::fromRGBA(255, 222, 173);
    s_colors["navy"] = Radiant::Color::fromRGBA(0, 0, 128);
    s_colors["oldlace"] = Radiant::Color::fromRGBA(253, 245, 230);
    s_colors["olive"] = Radiant::Color::fromRGBA(128, 128, 0);
    s_colors["olivedrab"] = Radiant::Color::fromRGBA(107, 142, 35);
    s_colors["orange"] = Radiant::Color::fromRGBA(255, 165, 0);
    s_colors["orangered"] = Radiant::Color::fromRGBA(255, 69, 0);
    s_colors["orchid"] = Radiant::Color::fromRGBA(218, 112, 214);
    s_colors["palegoldenrod"] = Radiant::Color::fromRGBA(238, 232, 170);
    s_colors["palegreen"] = Radiant::Color::fromRGBA(152, 251, 152);
    s_colors["paleturquoise"] = Radiant::Color::fromRGBA(175, 238, 238);
    s_colors["palevioletred"] = Radiant::Color::fromRGBA(219, 112, 147);
    s_colors["papayawhip"] = Radiant::Color::fromRGBA(255, 239, 213);
    s_colors["peachpuff"] = Radiant::Color::fromRGBA(255, 218, 185);
    s_colors["peru"] = Radiant::Color::fromRGBA(205, 133, 63);
    s_colors["pink"] = Radiant::Color::fromRGBA(255, 192, 203);
    s_colors["plum"] = Radiant::Color::fromRGBA(221, 160, 221);
    s_colors["powderblue"] = Radiant::Color::fromRGBA(176, 224, 230);
    s_colors["purple"] = Radiant::Color::fromRGBA(128, 0, 128);
    s_colors["red"] = Radiant::Color::fromRGBA(255, 0, 0);
    s_colors["rosybrown"] = Radiant::Color::fromRGBA(188, 143, 143);
    s_colors["royalblue"] = Radiant::Color::fromRGBA(65, 105, 225);
    s_colors["saddlebrown"] = Radiant::Color::fromRGBA(139, 69, 19);
    s_colors["salmon"] = Radiant::Color::fromRGBA(250, 128, 114);
    s_colors["sandybrown"] = Radiant::Color::fromRGBA(244, 164, 96);
    s_colors["seagreen"] = Radiant::Color::fromRGBA(46, 139, 87);
    s_colors["seashell"] = Radiant::Color::fromRGBA(255, 245, 238);
    s_colors["sienna"] = Radiant::Color::fromRGBA(160, 82, 45);
    s_colors["silver"] = Radiant::Color::fromRGBA(192, 192, 192);
    s_colors["skyblue"] = Radiant::Color::fromRGBA(135, 206, 235);
    s_colors["slateblue"] = Radiant::Color::fromRGBA(106, 90, 205);
    s_colors["slategray"] = Radiant::Color::fromRGBA(112, 128, 144);
    s_colors["slategrey"] = Radiant::Color::fromRGBA(112, 128, 144);
    s_colors["snow"] = Radiant::Color::fromRGBA(255, 250, 250);
    s_colors["springgreen"] = Radiant::Color::fromRGBA(0, 255, 127);
    s_colors["steelblue"] = Radiant::Color::fromRGBA(70, 130, 180);
    s_colors["tan"] = Radiant::Color::fromRGBA(210, 180, 140);
    s_colors["teal"] = Radiant::Color::fromRGBA(0, 128, 128);
    s_colors["thistle"] = Radiant::Color::fromRGBA(216, 191, 216);
    s_colors["tomato"] = Radiant::Color::fromRGBA(255, 99, 71);
    s_colors["turquoise"] = Radiant::Color::fromRGBA(64, 224, 208);
    s_colors["violet"] = Radiant::Color::fromRGBA(238, 130, 238);
    s_colors["wheat"] = Radiant::Color::fromRGBA(245, 222, 179);
    s_colors["white"] = Radiant::Color::fromRGBA(255, 255, 255);
    s_colors["whitesmoke"] = Radiant::Color::fromRGBA(245, 245, 245);
    s_colors["yellow"] = Radiant::Color::fromRGBA(255, 255, 0);
    s_colors["yellowgreen"] = Radiant::Color::fromRGBA(154, 205, 50);
    s_colors["transparent"] = Radiant::Color::fromRGBA(255, 255, 255, 0);
  }
  return s_colors;
}

namespace Radiant
{

  Color::Color(const QByteArray & color)
  {
    if (!set(color)) {
      setRGBA(0, 0, 0, 1);
      Radiant::warning("Color::Color # Failed to parse color '%s'", color.data());
    }
  }

  Color::Color(const char * color)
  {
    if (!set(color)) {
      setRGBA(0, 0, 0, 1);
      Radiant::warning("Color::Color # Failed to parse color '%s'", color);
    }
  }

  Color::Color(const QColor & c)
    : ColorBase(static_cast<float>(c.redF()), static_cast<float>(c.greenF()),
                static_cast<float>(c.blueF()), static_cast<float>(c.alphaF()))
  {}

  void Color::setHSVA(float h, float s, float v, float a)
  {
    float r, g, b;
    ColorUtils::hsvTorgb(h, s, v, r, g, b);
    setRGBA(r, g, b, a);
  }

  bool Color::set(const QByteArray & color)
  {
    if (color.startsWith('#')) {
      bool ok = false;
      Color c;
      if (color.size() == 4 || color.size() == 5) {
        // #RGB or #RGBA
        for (int i = 0; i < color.size()-1; ++i) {
          int t = color.mid(i+1, 1).toInt(&ok, 16);
          if (!ok)
            return false;
          c.data()[i] = ((t << 4) | t) / 255.0f;
        }
      } else if (color.size() == 7 || color.size() == 9) {
        // #RRGGBB or #RRGGBBAA
        for (int i = 0; i < color.size()-1; i += 2) {
          int t = color.mid(i+1, 2).toInt(&ok, 16);
          if (!ok || t < 0)
            return false;
          c.data()[i/2] = t / 255.0f;
        }
      } else {
        return false;
      }
      *this = c;
      return true;
    }

    const QMap<QByteArray, Radiant::Color> & c = colors();
    auto it = c.find(color.toLower());
    if (it == c.end())
      return false;

    *this = *it;
    return true;
  }

  QColor Color::toQColor() const
  {
    using Nimble::Math::Clamp;
    return QColor::fromRgbF(Clamp(r, 0.f, 1.f), Clamp(g, 0.f, 1.f), Clamp(b, 0.f, 1.f), Clamp(a, 0.f, 1.f));
  }

  const QMap<QByteArray, Color> & Color::namedColors()
  {
    return colors();
  }
}
