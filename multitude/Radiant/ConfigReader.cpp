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
#include <Radiant/FileUtils.hpp>

#include <math.h>
#include <stdlib.h>


#include <fstream>
#include <iostream>
#include <sstream>

#include <set>
#include <vector>
#include <stack>

namespace Radiant {

  Variant::Variant()
  {}

  Variant::Variant(const QString& a, const char * doc)
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
    QByteArray ba = m_var.toUtf8();
    const char * str = ba.data();
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
    QByteArray ba = m_var.toUtf8();
    const char * str = ba.data();
    char * strEnd = (char *) str;

    int v = strtol(str, &strEnd, 0);
  
    if(strEnd > str)
      return v;

    return def;
  }
  uint64_t Variant::getFromHex64(uint64_t def) const
  {
    if (m_var.isEmpty())
      return def;

    long long lltmp = 0;
    lltmp = m_var.toLongLong(0, 16);
    return lltmp;
  }


  const QString & Variant::getString(const QString & def) const
  {
    if(!m_var.isEmpty())
      return m_var;

    return def;
  }

  const QString & Variant::getString() const
  {
    return m_var;
  }

  /** Reads a number of floats from the string. Returns the number of
      floats successfully read. */

  int Variant::getInts(int *p, int n)
  {
    QByteArray ba = m_var.toUtf8();
    char *str = (char *) ba.data();
    int i = 0;

    while(str < ba.data() + ba.size() && i < n) {
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
    QByteArray ba = m_var.toUtf8();
    char *str = (char *) ba.data();

    int i = 0;

    while(str < ba.data() + ba.size() && i < n) {
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
    QByteArray ba = m_var.toUtf8();
    const char * str = ba.data();

    int i=0;

    while(str < ba.data() + ba.size() && i < n) {
      const char * endStr = str;

      char * end = 0;
      double tmp = strtod(str, &end);
      endStr = end;
      
      if(endStr <= str)
	return i;
      
      str = endStr;

      // printf("Val %d = %lf ", i, tmp);
      
      *p++ = tmp;
      i++;
    }
    
    return i;
  }

  void Variant::set(const QString &s)
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

  Variant::operator const QString & () const
  {
    return m_var;
  }

  void Variant::dump(std::ostream & os) const
  {
    os << m_var.toUtf8().data();
  }

  bool Variant::isEmpty() const 
  {
    return m_var.size() ? m_var[0] == '\0' : true;
  }

  bool Variant::hasDocumentation() const 
  {
    return m_doc.size() ? m_doc[0] != '\0' : false;
  }
    
  const QString & Variant::documentation() const
  {
    return m_doc;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  static std::set<QString> __writtenDocs;

  static Radiant::Mutex __mutex;

  static void cleardocs()
  {
    Radiant::Guard g( __mutex);

    __writtenDocs.clear();
  }

  static bool writedocs(const Variant & var)
  {
    if(!var.hasDocumentation())
      return false;

    Radiant::Guard g( __mutex);

    std::set<QString>::iterator it = 
      __writtenDocs.find(var.documentation());

    if(it != __writtenDocs.end())
      return false;

    __writtenDocs.insert(var.documentation());

    return true;
  }

  /** Specialized version that mimics the file format. */
  template <>
  void ChunkT<Variant>::dump(std::ostream& os, int indent) const
  {
    QString ws(indent, ' ');

    for(auto it = m_chunks->begin(); it != m_chunks->end(); ++it) {
      os << ws << it->first << " {\n";
      it->second.dump(os, indent+2);
      os << ws << "}\n";
    }

    for(const_iterator it = m_variants.begin();it != m_variants.end(); it++) {
      if((*it).second.hasDocumentation() && writedocs((*it).second))
        os << ws << "/* " << (*it).second.documentation() << " */\n";
      os << ws << (*it).first << " = \"" << (*it).second << "\"\n";
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

  bool readConfig(Config * c, const char * buf, int size)
  {
    if(!size)
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

    typedef std::pair<QString, Chunk> StackItem;
    std::stack<StackItem> stack;
    stack.push(std::make_pair("global", Chunk()));

    QString chunkName;
    QString variantName;
    QString variantVal;

    Chunk chunk;

    bool longVariant = false;

    int i;
    for(i=0; i < size; i++) {
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

          stack.push(std::make_pair(chunkName, chunk));
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

          stack.push(std::make_pair(chunkName, chunk));
          chunkName.clear();
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
          StackItem si = stack.top();
          stack.pop();

          Chunk tmp = chunk;
          chunk = si.second;
          chunk.addChunk(si.first, tmp);

          chunkName.clear();

          state = SCAN_VARIANT_NAME;
          DEBUG_PRINT(buf, i, "SCAN_CHUNK_NAME");
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
        else if(c1 == '{') {
          // expected to get a value for current variant
          // but a new block started instead
          stack.push(std::make_pair(variantName, chunk));
          chunk.clear();

          state = SCAN_VARIANT_NAME;
        }
        else if(c1 == '/' && c2 == '*') {
          state = SCAN_COMMENT;
          stackState = SCAN_VARIANT_BEGIN;
          DEBUG_PRINT(buf, i, "SCAN_COMMENT");
        }
        else if(c1 == '}') {
          StackItem si = stack.top();

          Chunk tmp = chunk;
          chunk = si.second;
          chunk.addChunk(si.first, tmp);

          stack.pop();
          //c->set(chunkName, chunk);
          chunkName.clear();
          //state = SCAN_CHUNK_NAME;
          state = SCAN_VARIANT_BEGIN;
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

    // @todo collect global variables from stack.top()
    Chunk & ch = chunk;
    for (auto it = ch.chunks()->begin(); it != ch.chunks()->end(); ++it) {
      c->set(it->first, it->second);
    }

    return c->size() > 0;
  }

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
    // only way to suppress warn_unused_result warnings with gcc?
    (void)n;
    fclose(in);

    return n <= 0 ? false : readConfig(c, &buf[0], size);
  }

  bool writeConfig(Config *config, const char *filename)
  {
    cleardocs();

    QString tmpfile = QString(filename) + ".tmp";

    std::ofstream out;
    out.open(tmpfile.toUtf8().data());

    if(!out.good())
      return false;

    config->dump(out);
    out.close();

    // remove original and replace with temporary
    // there doesn't seem to be a portable way to do this atomically
    if (FileUtils::removeFile(filename)) {
      return FileUtils::renameFile(tmpfile.toUtf8().data(), filename);
    }
    return false;
  }

} // namespace


