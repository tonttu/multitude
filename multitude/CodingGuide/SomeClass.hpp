/* COPYRIGHT

    All C/C++ files begin with a copyright notice. Whenever you create
    a new file, you should copy the copyright from another file in the
    same directory.

    The first line of the notice looks like the line above. This is
    because then we can replace them atomatically. Anything written
    into this space may get 

    Basic license crap is present in this secion.

 */

#ifndef CODINGGUIDE_SOMECLASS_HPP
#define CODINGGUIDE_SOMECLASS_HPP

/* First we include our "own" headers in alphabetical order. Different
   libraries are separated by an extra line-break. */

#include <Nimble/Math.hpp>
#include <Nimble/Vector2.hpp>

#include <Radiant/RefPtr.hpp>

/* After that we include system headers, first the more custom stuff
   (Qt, FTGL, libcd1394) and then the bulk stuff (stdio, stdlib,
   vector, list etc.)*/

#include <list>



/* Sometimes we cannot keep this include order, in those cases it is
   good to document, why there is a non-standard order. */

namespace CodingGuide {

  /* Here we introduce an anonymous pointer that helps us limit the
     include-chains (include the relevant declaration only in the cpp
     file.*/
  
  class DreamTime;

  /// Example class
  /** Here we document the class in depth. */
  class SomeClass
  {
  public:
    
    /* Friend class come first. We try to avoid "friends", but
       sometimes there really is no choice. */
    friend class FooBar1;

    /* Internal classes come here. */
    /** Nested class to demonstrate how we keep code in compartments. */
    class Item
    {
    public:
      Item(DreamTime *);
      ~Item();
      
      /* To access members, we write usually one-liners. */
      const Nimble::Vector2f & location() { return m_location; }
      const Nimble::Vector2f & velocity() { return m_velocity; }

      /** If you expose a typedef in the API, use class-like naming convention */
      typedef std::list<Nimble::Vector2> VectorList;
      
    private:
      /* Member variables begin with "m_". */
      
      /** Location is named "location", not "position", "loc" or
	  anything else.
	 
	  Also note that we resist the temptation to put "using
	  Nimble" inside the header. This way another programmer (and
	  Doxygen!!) can more easily deduce where the data types come
	  from. In the source (i.e. cpp) files we can be more relaxed.
      */
      Nimble::Vector2f m_location;
      /* Velocity is "velocity" and nothing else. */
      Nimble::Vector2f m_velocity;

      DreamTime * m_dreamTime;
    };
    
    /** Class-specific constants are easily presented as enums (where
	possible). */
    enum Style {
      /// The enum names are fully capitalized
      STYLE_SIMPLE,
      STYLE_ADVANCED,
    };

    /** Typedefs for short-hand notation come here.

	Below is a handy way to avoid writing the long typenames (for
	iterators etc) in the member functions. The use of
	Radiant::RefPtr etc reduce the need for manual resource
	deallocation - which is the most dangerous part of C++. 
	
	BTW: We avoid the GNU-style typedef mania (where every single
	type is typedeffed). Instead, we use types as they come unless
	there is some reason not to. Remember that "stdint.h" has a
	number of useful mappings when you need to specify some
	bit-depth for a number etc.
    */
    typedef std::list<Radiant::RefPtr<Item> > container;
    typedef container::iterator iterator;

    /* Then the constructors and destructor. */

    SomeClass();
    ~SomeClass();

    /* Then come all kinds of member functions. */
    
    /** Here is a typical function. By returning a pointer we imply
	that the return value may be zero (aka NULL, but we never write
        "NULL").

	BTW: We could return also Radiant::RefPtr<Item> which would
	better support reference counting (if the caller would use
	that information).

        Q: Why is the argument type not "const Nimble::Vector2 &",
        that would be faster or not ?

        A: Tha would be slower. We live in (mostly) 64-bit world. The
        Vector2 takes 8 bytes as would the reference (which is really
        a pointer in hardware level). So the effort to just copy the
        object equals the effort to make reference to it. To access
        that variable in the member function a reference would need
        another step of memory access, while here can just access the
        variable directly. There are also pointer aliasing issues that
        are better handled with this aproach.
     */
    Item * findNearest(Nimble::Vector2f location);
    
    /// Ad an item to the internal storage.
    /* Here we use the Radiant::RefPtr so that we can share the object
       counters between this object, and other objects.  */
    void addItem(Radiant::RefPtr<Item> &);
    /* We use function overloading. This is may be not the most
       relevant example, btu still... */
    void addItem(Radiant::RefPtr<Item> &, bool tofront);
    
    /* After the public stuff, comes the section of protected
       functions and after that the private members. The words
       "public", "protected" and "private" should be present only once
       per class. */
  protected:

    /* Usually all member variables are protected (or private). For a
       good reason we can break the rule (as with any rule). */

    container m_items;
    Style     m_style;
    /* Static variables begin with "s_". */
    static bool s_debug;
  };

}

#endif
