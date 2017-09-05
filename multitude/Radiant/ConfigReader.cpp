/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include <Radiant/ConfigReaderTmpl.hpp>

#include <Radiant/Mutex.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

#include <QFile>

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

      *p++ = int(tmp);
      i++;
    }
    
    return i;
  }


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

      *p++ = float(tmp);
      i++;
    }
    
    return i;
  }

  int Variant::getDoubles(double *p, int n)
  {
    QByteArray ba = m_var.toUtf8();
    const char * str = ba.data();

    int i=0;

    while(str < ba.data() + ba.size() && i < n) {
      char * end = 0;
      double tmp = strtod(str, &end);
      const char * endStr = end;
      
      if(endStr <= str)
	return i;
      
      str = endStr;

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
    return m_var.isEmpty();
  }

  bool Variant::hasDocumentation() const 
  {
    return !m_doc.isEmpty();
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
    std::string ws(indent, ' ');

    for(auto it = m_chunks->begin(); it != m_chunks->end(); ++it) {
      os << ws << it->first << " {\n";
      it->second.dump(os, indent+2);
      os << ws << "}\n";
    }

    for(const_iterator it = m_variants.begin();it != m_variants.end(); ++it) {
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
  static void DEBUG_PRINT(const char * buf,
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

  bool readConfig(Config * c, const char * buf, int size, const QString & sourceName)
  {
    if(!size)
      return true;

    enum State {
      SCAN_CHUNK_NAME,
      READ_CHUNK_NAME,
      SCAN_CHUNK_BEGIN,
      SCAN_VARIANT_NAME,
      READ_VARIANT_NAME,
      SCAN_VARIANT_BEGIN,
      READ_VARIANT_BEGIN,
      READ_VARIANT,
      SCAN_COMMENT
    };

    State state = SCAN_CHUNK_NAME;
    State stackState = SCAN_CHUNK_NAME;

    typedef std::pair<QString, Chunk> StackItem;
    std::stack<StackItem> stack;
    stack.push(std::make_pair("global", Chunk()));

    QString chunkName;
    QString variantName;
    QString variantVal;

    Chunk chunk;

    bool longVariant = false;

    int i;
    int line = 1;
    for(i=0; i < size; i++) {
      char c1 = buf[i];
      if (c1 == '\n')
        ++line;
      char c2 = i + 1 < size ? buf[i + 1] : '\n';

      // Allow comments everywhere except in quoted strings (long variants)
      if (state != SCAN_COMMENT && (state != READ_VARIANT || !longVariant)) {
        if (c1 == '/' && c2 == '*') {
          stackState = state;
          state = SCAN_COMMENT;
          ++i;
          continue;
        }
      }

      if(state == SCAN_CHUNK_NAME) {
        if(c1 == '{' || c1 == '}') {
          Radiant::error("%s:%d: error: Expected chunk name, got '%c'",
                         sourceName.toUtf8().data(), line, c1);
          return false;
        }
        if(!isspace(c1)) {
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
          chunkName.clear();
          chunk.clear();
        }
        else if(c1 == '}') {
          Radiant::error("%s:%d: error: Expected chunk name, got '}'",
                         sourceName.toUtf8().data(), line);
          return false;
        }
        else
          chunkName += c1;
      }
      else if(state == SCAN_CHUNK_BEGIN) {
        if(isspace(c1))
          ;
        else if(c1 == '{') {
          state = SCAN_VARIANT_NAME;

          stack.push(std::make_pair(chunkName, chunk));
          chunkName.clear();
          DEBUG_PRINT(buf, i, "SCAN_VARIANT_NAME");
        } else {
          Radiant::error("%s:%d: error: Expected chunk begin ('{')",
                         sourceName.toUtf8().data(), line);
          return false;
        }
      }
      else if(state == SCAN_VARIANT_NAME) {

        if(isspace(c1))
          ;
        else if(c1 == '}') {
          if (stack.size() <= 1) {
            Radiant::error("%s:%d: error: Unexpected '}'",
                           sourceName.toUtf8().data(), line);
            return false;
          }
          StackItem si = stack.top();
          stack.pop();

          Chunk tmp = chunk;
          chunk = si.second;
          chunk.addChunk(si.first, tmp);

          chunkName.clear();

          state = SCAN_VARIANT_NAME;
          DEBUG_PRINT(buf, i, "SCAN_VARIANT_NAME");
        }
        else if(c1 == '{') {
          Radiant::error("%s:%d: error: Expected variant name, got '{'",
                         sourceName.toUtf8().data(), line);
          return false;
        }
        else {
          variantName.clear();
          variantName += c1;
          state = READ_VARIANT_NAME;
          DEBUG_PRINT(buf, i, "READ_VARIANT_NAME");
        }
      }
      else if(state == READ_VARIANT_NAME) {
        if(isspace(c1)) {
          state = SCAN_VARIANT_BEGIN;
          DEBUG_PRINT(buf, i, "SCAN_VARIANT_BEGIN");
        } else if ((c1 == '=') || (c1 == '{')) {
          state = SCAN_VARIANT_BEGIN;
          DEBUG_PRINT(buf, i, "SCAN_VARIANT_BEGIN");
          --i;
        }
        else if(c1 == '}') {
          Radiant::error("%s:%d: error: Expected variant name, got '}'",
                         sourceName.toUtf8().data(), line);
          return false;
        }
        else
          variantName += c1;
      }
      else if(state == SCAN_VARIANT_BEGIN) {
        if(isspace(c1))
          ;
        else if(c1 == '=') {
          state = READ_VARIANT_BEGIN;
        }
        else if(c1 == '{') {
          // expected to get a value for current variant
          // but a new block started instead
          stack.push(std::make_pair(variantName, chunk));
          chunk.clear();

          state = SCAN_VARIANT_NAME;
          DEBUG_PRINT(buf, i, "SCAN_VARIANT_NAME");
        }
        else if(c1 == '}') {
          if (stack.size() <= 1) {
            Radiant::error("%s:%d: error: Unexpected '}'",
                           sourceName.toUtf8().data(), line);
            return false;
          }

          StackItem si = stack.top();

          Chunk tmp = chunk;
          chunk = si.second;
          chunk.addChunk(si.first, tmp);

          stack.pop();
          //c->set(chunkName, chunk);
          chunkName.clear();
          //state = SCAN_CHUNK_NAME;
          state = SCAN_VARIANT_BEGIN;
          DEBUG_PRINT(buf, i, "SCAN_VARIANT_BEGIN");
        }
        else {
          Radiant::error("%s:%d: error: Expected new chunk or '=', got '%c'",
                         sourceName.toUtf8().data(), line, c1);
          return false;
        }
      }
      else if(state == READ_VARIANT_BEGIN) {
        if(c1 == '{' || c1 == '}') {
          Radiant::error("%s:%d: error: Expected variant value, got '%c'",
                         sourceName.toUtf8().data(), line, c1);
          return false;
        }
        if(!isspace(c1)) {
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
        if (longVariant) {
          if(c1 == '"') {
            chunk.set(variantName, variantVal);
            state = SCAN_VARIANT_NAME;
            DEBUG_PRINT(buf, i, "SCAN_VARIANT_NAME after READ_VARIANT (long)");
          }
          else if (c1 == '\r' || c1 == '\n') {
            Radiant::error("%s:%d: error: Missing '\"' at the EOL",
                           sourceName.toUtf8().data(), line-1);
            return false;
          }
          else
            variantVal += c1;
        } else {
          if(isspace(c1) || c1 == '}') {
            chunk.set(variantName, variantVal);
            state = SCAN_VARIANT_NAME;
            if (c1 == '}')
              --i;
            DEBUG_PRINT(buf, i, "SCAN_VARIANT_NAME after READ_VARIANT (short)");
          } else if (c1 == '{') {
            Radiant::error("%s:%d: error: Expected variant value, got '}'",
                           sourceName.toUtf8().data(), line);
            return false;
          }
          else
            variantVal += c1;
        }
      }
      else if(state == SCAN_COMMENT) {
        if(c1 == '*' && c2 == '/') {
          state = stackState;
          i++;
          DEBUG_PRINT(buf, i, "EXIT COMMENT");
        }
      }
    }

    if (state == SCAN_COMMENT) {
      Radiant::error("%s:%d: error: Unterminated comment", sourceName.toUtf8().data(), line);
      return false;
    }
    if ((state != SCAN_CHUNK_NAME && state != SCAN_VARIANT_BEGIN && state != SCAN_VARIANT_NAME) ||
        stack.size() != 1) {
      Radiant::error("%s:%d: error: Unexpected end of file", sourceName.toUtf8().data(), line);
      return false;
    }


    // @todo collect global variables from stack.top()
    Chunk & ch = chunk;
    for (auto it = ch.chunks()->begin(); it != ch.chunks()->end(); ++it) {
      c->set(it->first, it->second);
    }

    return true;
  }

  bool readConfig(Config *c, const char *filename)
  {
    const char * fname = "Radiant::readConfig";

    QFile in(filename);

    if(!in.open(QIODevice::ReadOnly)) {
      fprintf(stderr, "%s # Failed to open file \"%s\"\n", fname, filename);
      return false;
    }

    QByteArray buf = in.readAll();

    return readConfig(c, buf.data(), buf.size(), filename);
  }

  bool writeConfig(const Config *config, const char *filename)
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


