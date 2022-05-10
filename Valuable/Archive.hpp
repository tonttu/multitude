/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
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
  class ArchiveElement;
  class ArchiveIterator;

  /**
   * Options that define the behaviour of the (de)serialize() methods.
   */
  class SerializationOptions
  {
  public:
    /// Serialization bitflags. The actual value that is serialized is from the
    /// layer with highest priority that is included in serialization with
    /// LAYER_-flags. The default is LAYER_USER, meaning that only manually set
    /// values will be serialized. If an attribute has a value set only from a
    /// CSS file, that whole attribute is ignored.
    /// @sa Valuable::Attribute::Layer
    enum Options
    {
      LAYER_DEFAULT         = 1 << 0, /// Serialize values from Valuable::Attribute::DEFAULT layer
      LAYER_STYLE           = 1 << 1, /// Serialize values from Valuable::Attribute::STYLE layer
      LAYER_USER            = 1 << 2, /// Serialize values from Valuable::Attribute::USER layer
      LAYER_STYLE_IMPORTANT = 1 << 3, /// Serialize values from Valuable::Attribute::STYLE_IMPORTANT layer

      /// Normal behaviour, serialize manually set values
      DEFAULTS              = LAYER_USER,
      /// Serialize only values that are different from the original values
      ONLY_CHANGED          = LAYER_STYLE | LAYER_USER | LAYER_STYLE_IMPORTANT,
      /// Serialize all values
      EVERYTHING            = LAYER_DEFAULT | LAYER_STYLE | LAYER_USER | LAYER_STYLE_IMPORTANT
    };

    /// Construct an options object with given flags.
    SerializationOptions(unsigned int options = DEFAULTS)
      : m_options(options) {}

    /// Check if given flag is enabled
    /// @param flag Flag to test
    /// @return True if given flag is enabled in the options.
    inline bool checkFlags(Options flag) { return (m_options & unsigned(flag)) == unsigned(flag); }
    /// Set the options for serialization
    inline void setOptions(Options flags) { m_options = flags; }
  protected:
    /// Actual bitmask of flags
    unsigned int m_options;
  };

  /// Interface for classes that want to implement ArchiveElement API
  /// ArchiveElement has a name, list of children, map of (name -> value) -pairs,
  /// and the content string.
  /// Every serialization format must provide a implementation of this class
  /// and use that to make new instances of ArchiveElement.
  /// All functions are called from ArchiveElement.
  class VALUABLE_API ArchiveElementImpl
  {
  public:
    /// Virtual destructor
    virtual ~ArchiveElementImpl();

    /// Adds a new child element to this element
    /// @param element New child element to be added
    virtual void add(ArchiveElementImpl & element) = 0;
    /// Access all child elements.
    /// @return The child iterator for all child elements.
    virtual ArchiveIterator children() const = 0;

    /// Writes a new named attribute
    /// @param name New unique (along this object) name for the attribute
    /// @param value Attribute value
    virtual void add(const QString & name, const QString & value) = 0;
    /// Reads a named attribute from the element.
    /// @param name Name of the attribute to retrieve
    /// @return Attribute contents as a string, or empty string if the attribute
    ///         was not found.
    virtual QString get(const QString & name) const = 0;

    /// Writes the element contents
    /// @param s New contents of the element
    virtual void set(const QString & s) = 0;
    /// Reads the element contents
    /// @return The contents of the element
    virtual QString get() const = 0;

    /// Reads the element name
    /// @return The name of the element
    virtual QString name() const = 0;
    /// Sets the element name
    /// @param name The new name of the element
    virtual void setName(const QString & name) = 0;
  };

  /// Classes that implement this interface provide the functionality for
  /// ArchiveIterator for one specific element type. This is used to iterate
  /// ArchiveElement children. All functions are called from ArchiveIterator.
  class VALUABLE_API ArchiveIteratorImpl
  {
  public:
    /// Virtual destructor
    virtual ~ArchiveIteratorImpl();
    /// Fetch the current element or empty Smart pointer on non-valid iterators.
    /// @return Smart pointer to current child element implementation
    virtual std::shared_ptr<ArchiveElementImpl> get() const = 0;
    /// Moves the iterator to the next element.
    virtual void next() = 0;
    /// Check if the iterator still points to valid element
    /// @return True if get() would return valid element.
    virtual bool isValid() const = 0;
    /// Check if two iterators point to the same object.
    /// @param other The seond iterator implementation to check.
    /// @return True if the iterators point to the same object.
    virtual bool operator == (const ArchiveIteratorImpl & other) const = 0;
  };

  /// Child iterator for ArchiveElement children, uses instance of
  /// ArchiveIteratorImpl to provide implementation for different element types.
  class VALUABLE_API ArchiveIterator
  {
  public:
    /// Construct a new ArchiveIterator
    /// @param impl The ArchiveIterator implementation to use, has to be valid pointer.
    ArchiveIterator(std::shared_ptr<ArchiveIteratorImpl> impl);

    /// Returns NULL if the iterator is not valid anymore. Can be used like
    /// for(it = foo.children(); it; ++it) {}
    /// @return true if the iterator is still valid
    explicit operator bool () const;

    /// Returns the current child, or null element in case of an invalid iterator
    ArchiveElement operator * () const;

    /// Prefix increment operator, changes the iterator to point to the next element.
    /// @return Returns itself after moving the location.
    ArchiveIterator & operator ++ ();

    /// Compares if the iterators point to the same element
    bool operator == (const ArchiveIterator & other) const;
    /// Compares if the iterators point to different elements
    bool operator != (const ArchiveIterator & other) const;

  private:
    std::shared_ptr<ArchiveIteratorImpl> m_impl;
  };

  /**
   * ArchiveElement defines the serialization API.
   *
   * The (de)serialize() methods should use this interface to actually
   * read/write data. The actual implementation is provided by instance of
   * ArchiveElementImpl, so this same class can be used and copied freely
   * without caring about underlying serialization format.
   *
   * For one object, the element can write the object name, it's contents,
   * any number of named attributes (key/value pairs), and any number of
   * child objects.
   * It can respectively read the name, contents, attributes by name,
   * and iterate over all child objects.
   * Every backend should implement this kind of data structure.
   *
   * There is also special "NULL" or "empty" element, that is usually used
   * in case of errors or similar. It can be created by using the default
   * ArchiveElement constructor.
   */
  class VALUABLE_API ArchiveElement
  {
  public:
    /// Child iterator for ArchiveElement
    typedef Valuable::ArchiveIterator Iterator;
    /// Constructs a new element with given implementation
    /// @param impl Serialization implementation to use. If implementation is
    ///        null, the whole ArchiveElement is "NULL" element.
    ArchiveElement(std::shared_ptr<ArchiveElementImpl> impl =
                   std::shared_ptr<ArchiveElementImpl>());

    /// Adds a new child element
    /// @param element New child element
    void add(const ArchiveElement & element);
    /// Returns the child iterator to children of this class
    /// @return New child iterator that points to the first child
    Iterator children() const;

    /// Writes a new named attribute, name should be unique along this object
    /// @param name Name of the attribute
    /// @param value Value of the attribute
    void add(const QString & name, const QString & value);
    /// Reads a named attribute
    /// @param name Name of the attribute to read
    /// @return Attribute value as a string, or empty string if no attribute was found.
    QString get(const QString & name) const;

    /// Writes the element contents
    /// @param s New contents of the element
    void set(const QString & s);
    /// Reads the element contents
    /// @return The contents of the element
    QString get() const;

    /// Reads the element name
    /// @return The name of the element
    QString name() const;
    /// Sets the element name
    /// @param name The new name of the element
    void setName(const QString & name);
    /// Is this a NULL element, created by the default constructor
    /// @return True if this element has no implementation.
    bool isNull() const;

    /// If the implementation is actually a XMLArchiveElement, return the wrapped
    /// DOMElement. This function is provided only for keeping backwards compatibility.
    /// @return Wrapped DOM element
    const DOMElement * xml() const;

  private:
    std::shared_ptr<ArchiveElementImpl> m_impl;
  };

  /**
   * Abstract class that defines an interface that allows working with elements.
   *
   * Different kinds of backends (XML, binary, etc..) should inherit from
   * this class. There always should be an Archive-implementation and
   * an ArchiveElementImpp-implementation, that are designed to work together.
   *
   * Every time Serializable objects are being serialized, the recursive
   * serialize() calls will have one Archive object as a parameter. It
   * keeps the serialization state and options with the help of
   * SerializationOptions.
   *
   * Archive has one "root" ArchiveElement, that may have child elements.
   */
  class VALUABLE_API Archive : public SerializationOptions
  {
  public:
    /// Creates a new Archive and initializes the SerializationOptions with given options.
    /// @param options Bitmask of SerializationOptions::Options
    Archive(unsigned int options = DEFAULTS);
    /// Default destructor does nothing.
    virtual ~Archive();

    /// Create a new element with correct implementation.
    /// @param name Name of the element.
    /// @return The created element
    virtual ArchiveElement createElement(const QString & name) = 0;

    /// Query the root element
    /// @return The root element
    virtual ArchiveElement root() const = 0;

    /// Sets the root element
    /// @param element New element that should be set as a new root element
    virtual void setRoot(const ArchiveElement & element) = 0;

    /// Writes the archive to file
    /// @param filename Absolute or relative filepath
    /// @return True on success
    virtual bool writeToFile(const QString & filename) const = 0;
    /// Write the archive to memory buffer
    /// @param buffer The output buffer where the archive is written
    /// @return True if writing succeeds
    virtual bool writeToMem(QByteArray & buffer) const = 0;
    /// Reads the archive from a file
    /// @param filename Absolute or relative filepath
    /// @return True on success
    virtual bool readFromFile(const QString & filename) = 0;

    virtual bool readFromMem(const QByteArray & buffer) = 0;

    /// If this is actually a XMLArchive, return the wrapped DOMDocument
    /// @return Default implementation returns NULL.
    virtual DOMDocument * xml();
  };
}
#endif // ARCHIVE_HPP
