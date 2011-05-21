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

#ifndef CONFIG_READER_TMPL_HPP
#define CONFIG_READER_TMPL_HPP

#include <Radiant/ConfigReader.hpp>

namespace Radiant {

  template <class T>
  int ChunkT<T>::numberOf(const QString & id) const
  {
    const_iterator it = m_variants.find(id);
    if(it == m_variants.end())
      return 0;

    int n = 1;
    for(it++ ; (it != m_variants.end()) && ((*it).first == id); it++)
      n++;

    return n;
  }

  template <class T>
  T ChunkT<T>::get(const QString &id) const
  {
    const_iterator it = m_variants.find(id);
    if(it != m_variants.end())
    return (*it).second;

    return T();
  }

  template <class T>
  T ChunkT<T>::get(const QString &id,
                   const QString &alternateId) const
  {
    const_iterator it = m_variants.find(id);
    if(it != m_variants.end())
      return (*it).second;

    it = m_variants.find(alternateId);

    if(it != m_variants.end())
      return (*it).second;

    return T();
  }
  
  template <class T>
  bool ChunkT<T>::contains(const QString &id) const
  {
    return m_variants.find(id) != m_variants.end();
  }

  template <class T>
  void ChunkT<T>::set(const QString & name, const T &v)
  {
		if(clearFirst)
	  m_variants.erase(name);
    m_variants.insert(std::pair<QString, T>(name, v));
	
  }

  template <class T>
  void ChunkT<T>::addChunk(const QString & name, const ChunkT<T> &v)
  {
    m_chunks.insert(std::make_pair(name, v));
  }

  template <class T>
  const ChunkT<T> & ChunkT<T>::getChunk(const QString &id) const
  {
    static ChunkT<T> empty;
    const_chunk_iterator it = m_chunks.find(id);
    if (it == m_chunks.end())
      return empty;
    else
      return it->second;
  }
   
  template <class T>
  void ChunkT<T>::setClearFlag(bool clearF)
  {
	  clearFirst=clearF;

  }

  template <class T>
  void ChunkT<T>::override(const QString & name, const T &v)
  { 
    iterator it;
    while((it = m_variants.find(name)) != m_variants.end())
      m_variants.erase(it);
    m_variants.insert(std::pair<QString, T>(name, v));
  }

  template <class T>
  void ChunkT<T>::dump(std::ostream& os, int indent)
  {
    QString ws(indent, ' ');
    for(chunk_iterator it = chunkBegin(); it != chunkEnd(); ++it) {
      os << ws << it->first << " {\n";
      it->second.dump(os, indent+2);
      os << ws << "}\n";
    }

    for(iterator it = m_variants.begin();it != m_variants.end(); it++) {
      os << ws << (*it).first << " {\n";
      (*it).second.dump(os, indent+2);
      os << ws << "}\n\n";
    }
  }

}

#endif
