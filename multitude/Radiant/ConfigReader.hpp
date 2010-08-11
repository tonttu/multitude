/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RADIANT_CONFIG_READER_HPP
#define RADIANT_CONFIG_READER_HPP

#include <Radiant/Export.hpp>

#include <string>
#include <map>

#include <iostream>

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
  /// @deprecated This class will be removed. Use Valuable::ConfigDocument
  class RADIANT_API Variant
  {
  public:
    Variant();
    /// Constructs a new variable and sets it value
    Variant(const std::string & a, const char * doc = 0);
    /// Constructs a new variable and sets it value
    Variant(const char *, const char * doc = 0);
    /// Constructs a new variable and sets it value
    Variant(int, const char * doc = 0);
    /// Constructs a new variable and sets it value
    Variant(unsigned, const char * doc = 0);
    /// Constructs a new variable and sets it value
    Variant(double, const char * doc = 0);
    /// Constructs a new variable and sets it value
    Variant(const float *, int, const char * doc = 0);
    /// Constructs a new variable and sets it value
    Variant(const int *, int, const char * doc = 0);

    ~Variant();

    /// Returns the value as integer
    operator int () const;
    /// Returns the value as double
    operator double () const;
    /// Returns the value as string
    operator const std::string & () const;

    /// Returns the value as double
    double              getDouble(double def = 0.0f) const;
    /// Returns the value as float
    float               getFloat(float def = 0.f) const;
    /// Returns the value as int
    int                 getInt(int def = 0) const;
    /// Returns the value as string or the given default value if the value has not been set
    const std::string & getString(const std::string & def) const;
    /// Returns the value as string
    const std::string & getString() const;

    /// Returns the value as several integers
    int                 getInts(int *, int);
    /// Returns the value as several floats
    int                 getFloats(float *, int);
    /// Returns the value as several doubles
    int                 getDoubles(double *, int);

    /// Sets the value
    void                set(const std::string &s);

    /// Prints the value to given stream
    void                dump(std::ostream& os) const;

    /// Returns true if the value has not been set
    bool                isEmpty() const;

    /// Returns true if the variable has been documented
    bool                hasDocumentation() const;
    
    /// Returns the documentation
    const std::string & documentation() const;

  private:
    std::string m_var;
    std::string m_doc;
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

      std::string fileName = chunk.get("filename").getString("default-file");
      float scale = chunk.get("scale").getFloat(1.0);

      \endcode

      @author Tommi Ilmonen (may contain some original code by Janne
      Kontkanen)
  */
  /// @deprecated This class will be removed. Use Valuable::ConfigDocument
  template <class T>
  class RADIANT_API ChunkT {
  public:
    /// Iterator for traversing all elements
    typedef typename std::multimap<std::string, T>::iterator iterator;
    typedef typename std::multimap<std::string, T>::const_iterator const_iterator;

    ChunkT() {clearFirst=false;}
    ~ChunkT() {}
    
    /// Returns the number of elements with given id/tag
    int numberOf(const std::string & id) const;

    /// Gets an element from the chunk
    /** @return The first element of type T. If there is no element
	with the given id, then an element will be created withthe
	default constructor. */
    T                  get(const std::string &id) const;
    /** Gets an element from the chunk.
      @param id the primary id to search for
      @param alternateId if the primary id does not match, alternate is used
      */
    T                  get(const std::string &id,
                           const std::string &alternateId) const;

    /// Check if this chunk contains an element with given id
    bool               contains(const std::string &id) const;

    /// Adds an element to the chunk
    /** If there are other elements with the same id before, then
	this element is added among those. */
    void               set(const std::string & id, const T &v);
  /// Sets the flag to inform whether an old value should be removed before defining a new
	void setClearFlag(bool clearF);
    /// Adds an element to the chunk, erasing any elements with identical id
    /** After calling this method, the chunk will contain only one
	element this this id. */
    void               override(const std::string & id, const T &v);

    /// Dumps this chunk into the stream
    /** May be specialized at each level. */
    void               dump(std::ostream& os);

    /// Empties this chunk
    void               clear() { m_variants.clear(); }

    /// Number of elements
    size_t size() const { return m_variants.size(); }

    /// Check if the chunk is empty
    bool               isEmpty() const { return m_variants.size() == 0; }

    /// Iterator to the first element
    iterator begin() { return m_variants.begin(); }
    /// Iterator to the after-the-end element
    iterator end()   { return m_variants.end(); }

    /// Iterator to the first element
    const_iterator begin() const { return m_variants.begin(); }
    /// Iterator to the after-the-end element
    const_iterator end()   const { return m_variants.end(); }

    /// Gets the element from an iterator
    static T & getType(iterator & it) { return (*it).second; }
    static const T & getType(const_iterator & it) { return (*it).second; }
        /// Gets the name (id) from an iterator
    static const std::string & getName(iterator & it) { return (*it).first; }
    static const std::string & getName(const_iterator & it) { return (*it).first; }
    
  private:

bool clearFirst;
    std::multimap<std::string, T> m_variants;
  };

  /// A chunk of configuration variables
  typedef ChunkT<Variant> Chunk;
  /// A chunk of chunks
  typedef ChunkT<Chunk>   Config;

  /// Read a configuration from a file
  bool RADIANT_API readConfig(Config *c, const char *filename);
  /// Write the given configuration into a file
  bool RADIANT_API writeConfig(Config *c, const char *filename);
  
} // namespace

/**
 * Output operator for Radiant::Variant
 */

inline std::ostream & operator << 
(std::ostream & os, const Radiant::Variant & v)
{
  v.dump(os);
  return os;
}


#endif
