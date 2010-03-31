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

#include <Radiant/ConfigReaderTmpl.hpp>

#include <Radiant/Mutex.hpp>

#include <math.h>
#include <stdlib.h>


#include <fstream>
#include <iostream>
#include <sstream>

#include <set>
#include <vector>

namespace Radiant {

  Variant::Variant()
  {}

  Variant::Variant(const std::string& a, const char * doc)
    : m_var(a)
  {
    if(doc) m_doc = doc;
  }

  Variant::Variant(const char *a, const char * doc)
    : m_var(a ? a : "")
  {
    if(doc) m_doc = doc;
  }
  
  Variant::Variant(int v, const char * doc)
  {
    char buf[32];
    sprintf(buf, "%d", v);
    m_var = buf;
    if(doc) m_doc = doc;
  }

  Variant::Variant(unsigned v, const char * doc)
  {
    char buf[32];
    sprintf(buf, "%u", v);
    m_var = buf;
    if(doc) m_doc = doc;
  }

  Variant::Variant(double v, const char * doc)
  {
    char buf[128];
    sprintf(buf, "%lf", v);
    m_var = buf;
    if(doc) m_doc = doc;
  }
  
  Variant::Variant(const float * ptr, int n, const char * doc)
  {
    char buf[128];
    
    for(int i = 0; i < n; i++) {
      sprintf(buf, "%f ", ptr[i]);
      m_var += buf;
    }
    if(doc) m_doc = doc;
  }

  Variant::Variant(const int * ptr, int n, const char * doc)
  {
    char buf[128];
    
    for(int i = 0; i < n; i++) {
      sprintf(buf, "%d ", ptr[i]);
      m_var += buf;
    }
    if(doc) m_doc = doc;
  }

  Variant::~Variant()
  {}

  double Variant::getDouble(double def) const
  { 
    const char * str = m_var.c_str();
    char * strEnd = (char *) str;

    double v = strtod(str, &strEnd);
    
    if(strEnd > str)
      return v;

    return def;
  }

  float Variant::getFloat(float def) const
  {
    return (float)getDouble(def);
  } 

  int Variant::getInt(int def) const
  {
    const char * str = m_var.c_str();
    char * strEnd = (char *) str;

    int v = strtol(str, &strEnd, 0);
  
    if(strEnd > str)
      return v;

    return def;
  }

  const std::string & Variant::getString(const std::string & def) const
  {
    if(m_var.size())
      return m_var;

    return def;
  }

  const std::string & Variant::getString() const
  {
    return m_var;
  }

  /** Reads a number of floats from the string. Returns the number of
      floats successfully read. */

  int Variant::getInts(int *p, int n)
  {
    char *str = (char *) m_var.c_str();
    int i = 0;

    while(str < m_var.c_str() + m_var.size() && i < n) {
      char * endStr = str;

      long tmp = strtol(str, &endStr, 10);
      
      if(endStr <= str)
	return i;
      
      str = endStr;

      // printf("Val %d = %lf ", i, tmp);
      
      *p++ = int(tmp);
      i++;
    }
    
    return i;
  }

  /** Reads a number of floats from the string. Returns the number of
      floats successfully read. */

  int Variant::getFloats(float *p, int n)
  {
    char *str = (char *) m_var.c_str();
    int i = 0;

    while(str < m_var.c_str() + m_var.size() && i < n) {
      char * endStr = str;

      double tmp = strtod(str, &endStr);
      
      if(endStr <= str)
	return i;
      
      str = endStr;

      // printf("Val %d = %lf ", i, tmp);
      
      *p++ = float(tmp);
      i++;
    }
    
    return i;
  }

  /** Reads a number of doubles from the string. Returns the number of
      doubles successfully read. */

  int Variant::getDoubles(double *p, int n)
  {
    char *str = (char *) m_var.c_str();
    int i=0;

    while(str < m_var.c_str() + m_var.size() && i < n) {
      char * endStr = str;

      double tmp = strtod(str, &endStr);
      
      if(endStr <= str)
	return i;
      
      str = endStr;

      // printf("Val %d = %lf ", i, tmp);
      
      *p++ = tmp;
      i++;
    }
    
    return i;
  }

  void Variant::set(const std::string &s)
  {
    m_var = s;
  }

  Variant::operator int () const
  {
    return getInt(0);
  }

  Variant::operator double () const
  {
    return getDouble(0.0);
  }

  Variant::operator const std::string & () const
  {
    return m_var;
  }

  void Variant::dump(std::ostream & os) const
  {
    os << m_var;
  }

  bool Variant::isEmpty() const 
  {
    return m_var.size() ? m_var[0] == '\0' : true;
  }

  bool Variant::hasDocumentation() const 
  {
    return m_doc.size() ? m_doc[0] != '\0' : false;
  }
    
  const std::string & Variant::documentation() const
  {
    return m_doc;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  static std::set<std::string> __writtenDocs;

  static Radiant::MutexStatic __mutex;

  static void cleardocs()
  {
    Radiant::GuardStatic g( & __mutex);

    __writtenDocs.clear();
  }

  static bool writedocs(const Variant & var)
  {
    if(!var.hasDocumentation())
      return false;

    Radiant::Guard g( & __mutex);

    std::set<std::string>::iterator it = 
      __writtenDocs.find(var.documentation());

    if(it != __writtenDocs.end())
      return false;

    __writtenDocs.insert(var.documentation());

    return true;
  }

  /** Specialized version that mimics the file format. */
  template <>
  void ChunkT<Variant>::dump(std::ostream& os)
  {
    for(iterator it = m_variants.begin();it != m_variants.end(); it++) {
      if(writedocs((*it).second))
	os << "  /* " << (*it).second.documentation() << " */\n";
      os << "  " << (*it).first << " = \"" << (*it).second << "\"\n";
    }
  }

  template class ChunkT<Variant>;
  template class ChunkT<Chunk>;

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

#if 0
  // Debugging function
  static void DEBUG_PRINT(const std::vector<char> & buf, 
		       int index, const char * state)
  {
    int line = 1;
    int ind = 0;

    for(int i=0; i < index; i++) {
      if(buf[i] == '\n') {
	line++;
	ind = 0;
      }
      ind++;
    }
    
    printf("Entering %s on line %d (char %d)\n", state, line, ind);
  }
#else

  // Empty macro to disable printouts
#define DEBUG_PRINT(x, y, z)

#endif

  bool readConfig(Config *c, const char *filename)
  {
    const char * fname = "Radiant::readConfig";
    
    FILE * in = fopen(filename, "rb");
    
    if(!in) {
      fprintf(stderr, "%s # Failed to open file \"%s\"\n", fname, filename);
      return false;
    }
    
    fseek(in, 0, SEEK_END);
    int size = ftell(in);
    fseek(in, 0, SEEK_SET);

    std::vector<char> buf;
    buf.resize(size);
    size_t n = fread(&buf[0], 1, size, in);
    fclose(in);

    if(!n)
      return false;

    enum {
      SCAN_CHUNK_NAME,
      READ_CHUNK_NAME,
      SCAN_CHUNK_BEGIN,
      SCAN_VARIANT_NAME,
      READ_VARIANT_NAME,
      SCAN_VARIANT_BEGIN,
      READ_VARIANT,
      SCAN_COMMENT
    };

    int state = SCAN_CHUNK_NAME;
    int stackState = 0;    
    

    std::string chunkName;
    std::string variantName;
    std::string variantVal;

    Chunk chunk;

    bool longVariant = false;

    for(int i=0; i < size; i++) {
      char c1 = buf[i];
      char c2 = i + 1 < size ? buf[i + 1] : '\n';
      
      if(state == SCAN_CHUNK_NAME) {
	if(isspace(c1))
	  ;
	else if(c1 == '/' && c2 == '*') {
	  state = SCAN_COMMENT;
	  stackState = SCAN_CHUNK_NAME;
	  DEBUG_PRINT(buf, i, "SCAN_CHUNK_NAME");
	}
	else {
	  state = READ_CHUNK_NAME;
	  DEBUG_PRINT(buf, i, "READ_CHUNK_NAME");
	  chunkName += c1;
	}
      }
      else if(state == READ_CHUNK_NAME) {
	if(isspace(c1)) {
	  state = SCAN_CHUNK_BEGIN;
	  DEBUG_PRINT(buf, i, "SCAN_CHUNK_BEGIN");
	  chunk.clear();
	}
	else if(c1 == '{') {
	  state = SCAN_VARIANT_NAME;
	  DEBUG_PRINT(buf, i, "SCAN_VARIANT_NAME");
	  chunk.clear();
	}
	else
	  chunkName += c1;
      }
      else if(state == SCAN_CHUNK_BEGIN) {
	if(isspace(c1))
	  ;
	else if(c1 == '/' && c2 == '*') {
	  state = SCAN_COMMENT;
	  stackState = SCAN_CHUNK_NAME;
	  DEBUG_PRINT(buf, i, "SCAN_COMMENT");
	}
	else if(c1 == '{') {
	  state = SCAN_VARIANT_NAME;
	  DEBUG_PRINT(buf, i, "SCAN_VARIANT_NAME");
	}
      }	
      else if(state == SCAN_VARIANT_NAME) {
	
	if(isspace(c1))
	  ;
	else if(c1 == '/' && c2 == '*') {
	  state = SCAN_COMMENT;
	  stackState = SCAN_VARIANT_NAME;
	  DEBUG_PRINT(buf, i, "SCAN_COMMENT");
	}
	else if(c1 == '}') {
	  c->set(chunkName, chunk);
	  state = SCAN_CHUNK_NAME;
	  DEBUG_PRINT(buf, i, "SCAN_CHUNK_NAME");
	  chunkName.clear();
	}
	else {
	  variantName.clear();
	  variantName += c1;
	  state = READ_VARIANT_NAME;
	  DEBUG_PRINT(buf, i, "READ_VARIANT_NAME");
	}
      }
      else if(state == READ_VARIANT_NAME) {
	if(isspace(c1) || (c1 == '=')) {
	  state = SCAN_VARIANT_BEGIN;
	  DEBUG_PRINT(buf, i, "SCAN_VARIANT_BEGIN");
	}
	else
	  variantName += c1;
      }
      else if(state == SCAN_VARIANT_BEGIN) {
	if(isspace(c1) || (c1 == '='))
	  ;
	else if(c1 == '/' && c2 == '*') {
	  state = SCAN_COMMENT;
	  stackState = SCAN_VARIANT_BEGIN;
	  DEBUG_PRINT(buf, i, "SCAN_COMMENT");
	}
	else if(c1 == '}') {
	  c->set(chunkName, chunk);
	  chunkName.clear();
	  state = SCAN_CHUNK_NAME;
	  DEBUG_PRINT(buf, i, "SCAN_CHUNK_NAME");
	}
	else {
	  variantVal.clear();
	  state = READ_VARIANT;

	  if(c1 == '"') {
	    longVariant = true;
	    DEBUG_PRINT(buf, i, "READ_VARIANT (long)");
	  }
	  else {
	    variantVal += c1;
	    longVariant = false;
	    DEBUG_PRINT(buf, i, "READ_VARIANT (short)");
	  }
	}
      }
      else if(state == READ_VARIANT) {
	if(isspace(c1) && !longVariant) {
	  chunk.set(variantName, variantVal);
	  state = SCAN_VARIANT_NAME;
	  DEBUG_PRINT(buf, i, "SCAN_VARIANT_NAME");
	}
	else if(c1 == '"' && longVariant) {
	  chunk.set(variantName, variantVal);
	  state = SCAN_VARIANT_NAME;
	  DEBUG_PRINT(buf, i, "SCAN_VARIANT_NAME");
	}
	else
	  variantVal += c1;
      }
      else if(state == SCAN_COMMENT) {
	if(c1 == '*' && c2 == '/') {
	  state = stackState;
	  i++;
	  DEBUG_PRINT(buf, i, "EXIT COMMENT");
	}
      }
    }

    return c->size() > 0;
  }

  bool writeConfig(Config *config, const char *filename)
  {
    cleardocs();

    std::ofstream out;
    out.open(filename);

    if(!out.good())
      return false;
    
    config->dump(out);

    return true;
  }

} // namespace


