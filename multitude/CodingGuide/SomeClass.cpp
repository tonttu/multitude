/* COPYRIGHT

    All C/C++ files begin with a copyright notice. Whenever you create
    a new file, you should copy the copyright from another file in the
    same directory.

    The first line of the notice looks like the line above. This is
    because then we can replace them atomatically. Anything written
    into this space may get 

    Basic license crap is present in this secion.

 */

/* We use quotes to tell the compiler that these files can fond from
   this directory. */

#include "SomeClass.hpp"

/* Now it is time to include the stuff we avoided in the header. */

#include <ProjectX/DreamTime.hpp>

/* Trace and error functions come from this header. */
#include <Radiant/Trace.hpp>

#include <assert.h>

namespace CodingGuide {

  SomeClass::Item::Item(ProjectX::DreamTime * dreamTime)
    : m_location(0, 0),
      m_velocity(0, 0),
      m_dreamTime(dreamTime)
  {}

  SomeClass::Item::~Item()
  {
  }

  /* Below is a marker that - if a file has more than one class -
     separates classes. */

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /* Before anything else, one puts the static variables: */

  bool SomeClass::s_debug = false;

  /* Member functions are iplemented in the declaration order (copy
     order from the header file). */

  SomeClass::SomeClass()
    : m_style(STYLE_SIMPLE)
  {
    if(s_debug)
      Radiant::trace("SomeClass::SomeClass # %p", this);
  }

  /* In principle we do not need this destructor as C++ could generate
     it for us. How-ever, to make sure that is instantiated only once
     we have this almost-dummy function here. We instantiate it only
     once to prevent binary bloat (which you easily get when you use
     C++ templates, like this class does). */
  SomeClass::~SomeClass()
  {}

  Item * SomeClass::findNearest(Nimble::Vector2f location)
  {
    /* For debug output we sometimes put the function name into a
       string constant called "fname". If the debug calls are moved to
       another function, then the function name will be printed
       correctly. */
    static const char * fname = "SomeClass::findNearest";

    /* As you can see we handle error cases by return value - not by
       exceptions. */
    if(!m_items.size()) {
      /* Error reporting can be done with Radiant::error.

	 The output functions (trace, error, fatal), are in principle
	 thin wrappers around printf, with the extra feature that they
	 prevent garbled output when many threads produce
	 output. Likewise they add the newline at the end of the text
	 so you do not need to type it manually. Finally we turn off
	 all console output if we want at some later stage. */
      if(s_debug)
	Radiant::error("%s # No items", fname);
      return 0;
    }

    iterator it = m_items.begin();
    Item * res = (*it).ptr(); // "res" = result
    float bestDist = (location - res->location()).length();

    for( ; it != m_items.end(); it++) {
      Item * tmp = (*it).ptr();

      float dist = (location - tmp->location()).length();
      if(bestDist > dist) {
	bestDist = dist;
	res = tmp;
      }
    }

    /* If you want to do debug output this is the usual way: */
    if(s_debug)
      Radiant::trace("%s # Got %p at [%f %f]", fname, res,
		     res->location().x, res->location().y);

    return res;
  }

  void SomeClass::addItem(Radiant::RefPtr<Item> & item)
  {
    /* One can also use "assert" for detecting fatal errors (if it
       feels like it). Usually assert is a bit too aggressive -
       especially in libraries you do not the application to die, just
       because there was bad argument.  */
    assert(item.ptr() != 0);

    m_items.push_back(item);
  }

  void SomeClass::addItem(Radiant::RefPtr<Item> &, bool tofront)
  {
    assert(item.ptr() != 0);

    if(tofront)
      m_items.push_front(item);
    else
      m_items.push_back(item);
  }
}
