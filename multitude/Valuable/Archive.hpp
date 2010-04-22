#ifndef ARCHIVE_HPP
#define ARCHIVE_HPP

#include <Valuable/DOMDocument.hpp>
#include <Valuable/DOMElement.hpp>

#include <cassert>

namespace Valuable
{

  /**
   * Options that define the behaviour of the (de)serialize() methods.
   */
  class SerializationOptions
  {
  };

  /**
   * Abstract class that defines the serialization backend interface.
   *
   * The (de)serialize() methods should use this interface to actually
   * read/write data.
   *
   * For one object, the element can write the objects contents, any number of
   * named attributes (key/value pairs), and any number of child objects.
   * It can respectively read the contents, attributes by name, and iterate
   * over all child objects.
   * Every backend should implement this kind of data structure.
   */
  class ArchiveElement
  {
  protected:
    /// ArchiveElements should not be freed when they are handled as plain
    /// ArchiveElement pointers. Memory management is handle by Archive.
    virtual ~ArchiveElement();

  public:
    /// Adds a new child element
    virtual void add(ArchiveElement & element) = 0;

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

    /// @todo do we need this?
    virtual std::string name() const = 0;
    /// @todo document
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
  class Archive : public SerializationOptions
  {
  public:
    /// Destructor should also delete all ArchiveElements this Archive owns
    virtual ~Archive();

    /// Create a new element that is owned by the Archive
    virtual ArchiveElement & createElement(const char * name) = 0;

    /// Create an empty ArchiveElement
    /// @todo rename to something that includes "null"
    /// @see ArchiveElement::isNull
    virtual ArchiveElement & emptyElement() = 0;
    /// Returns the root element
    virtual ArchiveElement & root() = 0;

    /// Sets the root element
    /// @todo rename to setRoot
    /// @todo Is this surely called only once?
    virtual void add(ArchiveElement & element) = 0;

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
