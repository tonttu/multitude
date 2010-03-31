#include <Valuable/HasValues.hpp>
#include <Valuable/Valuable.hpp>
#include <Valuable/ValueFloat.hpp>
#include <Valuable/ValueRect.hpp>
#include <Valuable/ValueString.hpp>

#include <string.h>


using namespace Valuable;

class MyValues : public Valuable::HasValues
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

  ValueFloat   m_v;
  ValueRect    m_r;
  ValueWString m_ws;
  ValueString  m_s;
};

int main(int, char **)
{
  Valuable::initialize();

  MyValues values1, values2;

  bool res = values1.saveToFileXML("test.xml");

  printf("save %s.\n", res ? "ok" : "fail");

  res = values2.loadFromFileXML("test.xml");

  printf("load %s.\n", res ? "ok" : "fail");

  Valuable::terminate();

  return 0;
}
