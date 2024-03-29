/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include <Valuable/Node.hpp>
#include <Valuable/Valuable.hpp>
#include <Valuable/AttributeFloat.hpp>
#include <Valuable/AttributeRect.hpp>
#include <Valuable/AttributeString.hpp>

#include <string.h>


using namespace Valuable;

class MyValues : public Valuable::Node
{
public:
  MyValues()
      : m_v(this, "kissa", 1.f),
      m_r(this, "nelio", Nimble::Rect(0.f, 0.f, 1.f, 1.f)),
      m_ws(this, "unicode", "widestuff"),
      m_s(this, "str", "abcdefg")
  {
    setName("apina");
    // setType("MyValues");
  }

  virtual const char * type() const { return "MyValues"; }

  AttributeFloat   m_v;
  AttributeRect    m_r;
  AttributeString  m_ws;
  AttributeString  m_s;
};

int main(int, char **)
{
  MyValues values1, values2;

  bool res = values1.saveToFileXML("test.xml");

  printf("save %s.\n", res ? "ok" : "fail");

  res = values2.loadFromFileXML("test.xml");

  printf("load %s.\n", res ? "ok" : "fail");

  return 0;
}
