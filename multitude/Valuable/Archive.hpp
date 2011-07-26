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

#ifndef VALUABLE_ARCHIVE_HPP
#define VALUABLE_ARCHIVE_HPP

#include "DOMDocument.hpp"
#include "DOMElement.hpp"
#include "Export.hpp"

#include <cassert>

namespace Valuable
{
  class Archive;

  /**
   * Options that define the behaviour of the (de)serialize() methods.
   */
  class VALUABLE_API SerializationOptions
  {
  public:
    /// Serialization bitflags
    enum Options { DEFAULTS = 0,    /// Normal behaviour, serialize everything
                   ONLY_CHANGED = 1 /// Serialize only values that are different from the original values
                 };

    /// Construct an options object with given flags.
    SerializationOptions(unsigned int options = DEFAULTS);

    /// Returns true if given flag is enabled with in the options.
    inline bool checkFlag(unsigned int flag) { return (m_options & flag) == flag; }
  protected:
    /// Actual bitmask of flags
    unsigned int m_options;
  };

  /**
   * Abstract class that defines the serialization backend interface.
   *
   * The (de)serialize() methods should use this interface to actually
   * read/write data.
   *
   * For one object, the element can write the object name, it's contents,
   * any number of named attributes (key/value pairs), and any number of
   * child objects.
   * It can respectively read the name, contents, attributes by name,
   * and iterate over all child objects.
   * Every backend should implement this kind of data structure.
   *
   * There is also special "NULL" or "empty" element, that is usually used
   * in case of errors or similar. It can be created / returned by using
   * Archive::emptyElement().
   */
  class VALUABLE_API ArchiveElement
  {
  protected:
    /// ArchiveElements should not be freed when they are handled as plain
    /// ArchiveElement pointers. Memory management is handle by Archive.
    virtual ~ArchiveElement();

  public:
    /// Child iterator for ArchiveElement children
    class VALUABLE_API Iterator
    {
    public:
      virtual ~Iterator();

      /// Returns NULL if the iterator is not valid anymore. Can be used like
      /// for(it = foo.children(); it; ++it) {}
      virtual operator const void * () = 0;

      /// Returns the current child, or empty element in case of invalid iterator
      virtual ArchiveElement & operator * () = 0;
      /// Returns the current child, or empty element in case of invalid iterator
      virtual ArchiveElement * operator -> () = 0;

      /// Prefix increment operator
      virtual Iterator & operator ++ () = 0;
      /// Postfix increment operator, slower than prefix version, since the new
      /// iterator will have to be copied and stored to the old one.
      virtual Iterator & operator ++ (int) = 0;

      /// Compares if the iterators point to the same element
      virtual bool operator == (const Iterator & other) = 0;
      /// Compares if the iterators point to different elements
      virtual bool operator != (const Iterator & other) = 0;
    };

    /// Adds a new child element
    virtual void add(ArchiveElement & element) = 0;
    /// Returns the child iterator that is owned by this element
    virtual Iterator & children() = 0;

    /// Writes a new named attribute, name should be unique along this object
    virtual void add(const char * name, const char * value) = 0;
    /// Reads a named attribute
    virtual std::string get(const char * name) const = 0;

    /// Writes the element contents
    virtual void set(const std::string & s) = 0;
    /// Writes the element contents as a wide character string
    virtual void set(const std::wstring & s) = 0;
    /// Reads the element contents
    virtual std::string get() const = 0;
    /// Reads the element contents as a wide character string
    virtual std::wstring getW() const = 0;

    /// Reads the element name
    virtual std::string name() const = 0;
    /// Is this a NULL element, created by Archive::emptyElement()
    virtual bool isNull() const = 0;

    /// If this is actually a XMLArchiveElement, return the wrapped DOMElement
    /// Default implementation returns NULL
    virtual DOMElement * xml();
  };

  /**
   * Abstract class that owns all ArchiveElements and defines an interface
   * that allows working with them.
   *
   * Different kinds of backends (XML, binary, etc..) should inherit from
   * this class. There always should be an Archive-implementation and
   * an ArchiveElement-implementation, that are designed to work together.
   *
   * Every time Serializable objects are being serialized, the recursive
   * serialize() calls will have one Archive object as a parameter. It
   * keeps the serialization state and options with the help of
   * SerializationOptions.
   *
   * Every ArhiveElement is owned by Archive, because otherwise the memory
   * management would be quite complex or inefficient.
   *
   * Archive has one "root" ArchiveElement, that may have more child elements.
   */
  class VALUABLE_API Archive : public SerializationOptions
  {
  public:
    /// Creates a new Archive and initializes the SerializationOptions with given options.
    /// @param options Bitmask of SerializationOptions::Options
    Archive(unsigned int options = DEFAULTS);
    /// Destructor should also delete all ArchiveElements this Archive owns
    virtual ~Archive();

    /// Create a new element that is owned by the Archive
    virtual ArchiveElement & createElement(const char * name) = 0;

    /// Create an empty ArchiveElement
    /// @see ArchiveElement::isNull
    virtual ArchiveElement & emptyElement() = 0;
    /// Returns the root element
    virtual ArchiveElement & root() = 0;

    /// Sets the root element
    virtual void setRoot(ArchiveElement & element) = 0;

    /// Writes the archive to file
    virtual bool writeToFile(const char * file) = 0;
    /// Write the archive to memory buffer
    /// @todo why not std::string - basically the same thing as vector<char>
    /// @todo why not some binary buffer object from Qt
    virtual bool writeToMem(std::vector<char> & buffer) = 0;
    /// Reads the archive from a file
    virtual bool readFromFile(const char * filename) = 0;

    /// If this is actually a XMLArchive, return the wrapped DOMDocument
    /// Default implementation returns NULL
    virtual DOMDocument * xml();
  };
}
#endif // ARCHIVE_HPP
