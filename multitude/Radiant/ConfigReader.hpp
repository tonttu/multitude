/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_CONFIG_READER_HPP
#define RADIANT_CONFIG_READER_HPP

#include <Radiant/Export.hpp>

#include <QString>
#include <map>
#include <cstdint>

#include <iostream>

#include <QString>

/// @todo add chunk free config file support

namespace Radiant {

  /** A single variant. All variants are stored as text strings. This
      class provides methods to convert the string to the most simple
      variable types including integers, doubles and float vectors.

      In addition to the actual variable information, a variant may
      contain a documentation string.

      @author Tommi Ilmonen (may contain original code by Janne
      Kontkanen)
  */

    /// @todo add global chunk support, document, examples, remove this and
    /// use Valuable::ConfigReader
  // @deprecated This class will be removed. Use Valuable::ConfigDocument  - or maybe not.
  class RADIANT_API Variant
  {
  public:
    /// Constructor
    Variant();
    /// Constructs a new variable and sets it value
    /// @param a Value as a string
    /// @param doc Documentation of the variable
    Variant(const QString & a, const char * doc = 0);

    /// Constructs a new variable and sets it value
    /// @param str Value as a string
    /// @param doc Documentation of the variable
    Variant(const char *str, const char * doc = 0);

    /// Constructs a new int variable
    /// @param v Value to represent
    /// @param doc Documentation of the variable
    Variant(int v, const char * doc = 0);

    /// Constructs a new unsigned int variable
    /// @param v Value to represent
    /// @param doc Documentation of the variable
    Variant(unsigned v, const char * doc = 0);

    /// Constructs a double variable
    /// @param v Value to represent
    /// @param doc Documentation of the variable
    Variant(double v, const char * doc = 0);

    /// Constructs an array of floating point numbers
    /// @param arr Array of floating points
    /// @param n Length of an array
    /// @param doc Documentation of the variable
    Variant(const float *arr, int n, const char * doc = 0);

    /// Constructs an array of integers
    /// @param arr Array of integers
    /// @param n Length of an array
    /// @param doc Documentation of the variable
    Variant(const int *arr, int n, const char * doc = 0);

    /// Destructor
    ~Variant();

    /// Returns the value as integer.
    /// @return Value as integer. Zero if value could not represented as integer
    operator int () const;

    /// Returns the value as double
    /// @return Value as double. Zero if value could not represented as integer
    operator double () const;

    /// Returns the value as string
    /// @return Value as a string
    operator const QString & () const;

    /// Returns the value as a double
    /// @param def Default value. Is returned if the value can't be represented as double.
    /// @return Value as double
    double              getDouble(double def = 0.0f) const;

    /// Returns the value as a float
    /// @param def Default value. Is returned if the value can't be represented as float.
    /// @return Value as float
    float               getFloat(float def = 0.f) const;

    /// Returns the value as int
    /// @param def Default value. Is returned if the value can't be represented as integer.
    /// @return Value as integer
    int                 getInt(int def = 0) const;

    /// Returns the value as unsigned 64 bit integer (interpreted as hexadecimal)
    /// @param def Default value. Is returned if the value can't be represented as 64bit unsigned.
    /// @return Value as 64bit unsigned
    uint64_t            getFromHex64(uint64_t def = 0) const;

    /// Returns the value as string or the given default value if the value has not been set
    /// @param def Default value. Is returned if the value is not yet set.
    /// @return Value as string
    const QString & getString(const QString & def) const;

    /// Returns the value as string
    /// @return Value as a string. Empty string if not set
    const QString & getString() const;

    /// Reads an array of integers from the value.
    /// @param[out] target Array where the integers are stored
    /// @param n Maximum number of integers to be read
    /// @return Number of integers read
    int                 getInts(int *target, int n);

    /// Reads an array of floats from the value.
    /// @param[out] target Array where the floats are stored
    /// @param n Maximum number of floats to be read
    /// @return Number of floats read
    int                 getFloats(float *target, int n);

    /// Reads an array of doubles from the value.
    /// @param[out] target Array where the doubles are stored
    /// @param n Maximum number of doubles to be read
    /// @return Number of doubles read
    int                 getDoubles(double *target, int n);

    /// Sets the value
    /// @param s Value to set
    void                set(const QString &s);

    /// Prints the value to given stream
    /// @param os Stream where the value is printed
    void                dump(std::ostream& os) const;

    /// Returns true if the value has not been set
    /// @return True if value is not set ie. it is empty.
    bool                isEmpty() const;

    /// Returns true if the variable has been documented
    /// @return True if the variable has documentation ie. is not empty
    bool                hasDocumentation() const;

    /// Returns the documentation
    /// @return Documentation of the varuable
    const QString & documentation() const;

  private:
    QString m_var;
    QString m_doc;
  };

  /** A template chunk class. A chunk contains elements that are named
      with (text string) identifiers. A chunk may contain multiple
      elements with the same identifier. These classes can be nested
      easily. Usually the two default levels are enough. A practical
      example:

      \code

      Radiant::Config config;

      // Read in two configuration files:
      bool ok =  Radiant::readConfig(&config, "first-config");
      ok = ok && Radiant::readConfig(&config, "second-config");

      // Now retrieve values from the configuration:

      Radiant::Chunk chunk = config.get("Scene");

      // Now read two variants from the config
      // We can provide default values for variables in case the variable
      // has not been defined in the configuration.

      QString fileName = chunk.get("filename").getString("default-file");
      float scale = chunk.get("scale").getFloat(1.0);

      \endcode

      @author Tommi Ilmonen (may contain some original code by Janne
      Kontkanen)
  */
  /// @deprecated This class will be removed. Use Valuable::ConfigDocument
  /// @tparam T Type of things contained in Chunk
  template <class T>
  class RADIANT_API ChunkT {
  public:

    /// Iterator for traversing all elements
    typedef typename std::multimap<QString, T>::iterator iterator;
    /// Constant iterator for traversing all elements
    typedef typename std::multimap<QString, T>::const_iterator const_iterator;

    /// Creates an empty configuration chunk
    ChunkT() {clearFirst=false; m_chunks = new std::multimap<QString, ChunkT<T> >(); }
    /// Destructor
    ~ChunkT() { delete m_chunks; }

    /// Copy constructor
    /// @param copy Chunk to copy
    ChunkT(const ChunkT & copy)
      : clearFirst(copy.clearFirst)
      , m_variants(copy.m_variants)
      , m_chunks(new std::multimap<QString, ChunkT<T> >(*copy.m_chunks))
    {
    }

    /// Copy a chunk with assignment
    /// @param copy Chunk to assign to this
    /// @return Reference to this
    ChunkT & operator= (const ChunkT & copy)
    {
      clearFirst = copy.clearFirst;
      *m_chunks = *copy.m_chunks;
      m_variants = copy.m_variants;

      return *this;
    }

    /// Returns the number of elements with given id/tag
    /// @param id Id to query
    /// @return Number of elements with the given id
    int numberOf(const QString & id) const;

    /// Gets an element from the chunk
    /// @param id element id
    /// @return The first element of type T. If there is no element
    /// with the given id, then an element will be created with the
    /// default constructor.
    T                  get(const QString &id) const;

    /** Gets an element from the chunk.
      @param id the primary id to search for
      @param alternateId if the primary id does not match, alternate is used
      @return matching element
      */
    T                  get(const QString &id,
                           const QString &alternateId) const;

    /// Check if this chunk contains an element with given id
    /// @param id Id to check
    /// @return True if the chunk contains an element with given id
    bool               contains(const QString &id) const;

    /// Adds an element to the chunk
    /** If there are other elements with the same id before, then
    this element is added among those.
    @param id element id
    @param v chunk to add */
    void               set(const QString & id, const T &v);

    /// Adds a new child node to this chunk
    /// @param id Id of the added chunk
    /// @param v Chunk to add
    void               addChunk(const QString & id, const ChunkT<T> &v);

    /// Gets a child chunk. If child with given id does not exist this returns proxy chunk.
    /// @param id Id of the queried chunk
    /// @return Reference to chunk queried, or proxy chunk.
    const ChunkT<T> &        getChunk(const QString & id) const;

    /// Sets the flag to inform whether an old value should be removed before defining a new
    /// @param clearF Are the old values removed when defining new value with same id
    void setClearFlag(bool clearF);

    /// Adds an element to the chunk, erasing any elements with identical id
    /// After calling this method, the chunk will contain only one
    /// element this this id.
    /// @param id id to override
    /// @param v chunk to override with
    void               override(const QString & id, const T &v);

    /// Dump chunk into stream
    /// May be specialized at each level.
    /// @param os output stream
    /// @param indent text indent for formatting
    void               dump(std::ostream& os, int indent=0) const;

    /// Empties this chunk
    void               clear() {
      m_variants.clear(); m_chunks->clear();
    }

    /// Number of elements
    /// @return Number of elements
    size_t size() const { return m_variants.size(); }

    /// Check if the chunk is empty
    /// @return True if empty
    bool               isEmpty() const { return m_variants.size() == 0; }

    /// Check if at least one variant with given name exists
    /// @param variantName Name to search
    /// @return True if contains variant with the given name
    bool               containsVariant(const QString & variantName) const
    { return m_variants.find(variantName) != m_variants.end(); }

    /// Iterator to the first element
    /// @return STL-like iterator to the beginning of this chunk
    iterator begin() { return m_variants.begin(); }
    /// Iterator to the after-the-end element
    /// @return STL-like iterator to the end of this chunk
    iterator end()   { return m_variants.end(); }

    /// Iterator to the first element
    /// @return STL-like iterator to the beginning of this chunk
    const_iterator begin() const { return m_variants.begin(); }
    /// Iterator to the after-the-end element
    /// @return STL-like iterator to the end of this chunk
    const_iterator end()   const { return m_variants.end(); }

    /// Get the map of chunks
    /// @return Chunks stored in this chunk
    const std::multimap<QString, ChunkT<T> > & chunks() const { return *m_chunks; }

    /// Gets the data element from an iterator
    /// @param it Iterator to inspect
    /// @return Contents of Chunk pointed by iterator
    static T & getType(iterator & it) { return (*it).second; }
    /// Gets the data element from an constant iterator
    /// @param it Iterator to inspect
    /// @return Contents of Chunk pointed by iterator
    static const T & getType(const_iterator & it) { return (*it).second; }

    /// Gets the name (id) from an iterator
    /// @param it Iterator to inspect
    /// @return Name of the Chunk pointed by iterator
    static const QString & getName(iterator & it) { return (*it).first; }
    /// Gets the name (id) from a constant iterator
    /// @param it Iterator to inspect
    /// @return Name of the Chunk pointed by iterator
    static const QString & getName(const_iterator & it) { return (*it).first; }


  private:

    bool clearFirst;
    std::multimap<QString, T> m_variants;
    std::multimap<QString, ChunkT<T> > * m_chunks;
  };

  /// A chunk of configuration variables
  typedef ChunkT<Variant> Chunk;
  /// A chunk of chunks
  typedef ChunkT<Chunk>   Config;

  /// Read a configuration from a file
  /// @param c The configuration object to fill
  /// @param filename Name of the file where the config is read
  /// @return False if nothing was read, otherwise true
  bool RADIANT_API readConfig(Config *c, const char *filename);

  /// Read the configuration from a string
  /** @param c The configuration object to fill.
      @param buf The configuration string.
      @param n The length of the configuration string
      @param sourceName filename or other source identification that is used with error messages
      @return false if nothing was read, otherwise true
  */
  bool  RADIANT_API readConfig(Config *c, const char * buf, int n, const QString & sourceName);

  /// Write the given configuration into a file
  /// @param c Configuration object to write
  /// @param filename Name of the file to write
  /// @return True if the operation was succesful, false otherwise.
  bool RADIANT_API writeConfig(const Config *c, const char *filename);

  /**
   * Output operator for Radiant::Variant
   * @param os Target stream for output
   * @param v Variant to output
   * @return Reference to stream
   */
  inline std::ostream & operator <<
  (std::ostream & os, const Variant & v)
  {
    v.dump(os);
    return os;
  }
} // namespace

#endif
