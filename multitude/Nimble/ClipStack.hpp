#ifndef NIMBLE_CLIPSTACK_HPP
#define NIMBLE_CLIPSTACK_HPP

#include "Export.hpp"
#include "Rectangle.hpp"

namespace Nimble
{
  class NIMBLE_API ClipStack
  {
  public:
    ClipStack();
    ClipStack(const ClipStack & other);
    ClipStack & operator=(const ClipStack & other);
    ~ClipStack();

    ClipStack & push(const Rectangle & r);
    ClipStack & pop();

    bool isVisible(const Nimble::Rectangle & r) const;

  private:
    class D;
    D * m_d;
  };

}

#endif // NIMBLE_CLIPSTACK_HPP
