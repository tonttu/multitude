/* COPYRIGHT
 */

#include "AttributeRect.hpp"
#include "DOMElement.hpp"

#include <Radiant/StringUtils.hpp>

namespace Valuable
{
  template <> VALUABLE_API const char * AttributeRectT<float>::type() const { return "rect"; }
  template <> VALUABLE_API const char * AttributeRectT<double>::type() const { return "rectd"; }
  template <> VALUABLE_API const char * AttributeRectT<int>::type() const { return "recti"; }
}

