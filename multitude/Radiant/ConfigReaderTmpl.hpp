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
  T ChunkT<T>::get(const std::string &id)
  {
    iterator it = m_variants.find(id);
    if(it != m_variants.end())
    return (*it).second;

    return T();
  }

  template <class T>
  T ChunkT<T>::get(const std::string &id,
                   const std::string &alternateId)
  {
    iterator it = m_variants.find(id);
    if(it != m_variants.end())
      return (*it).second;

    it = m_variants.find(alternateId);

    if(it != m_variants.end())
      return (*it).second;

    return T();
  }
  
  template <class T>
  bool ChunkT<T>::contains(const std::string &id)
  {
    return m_variants.find(id) != m_variants.end();
  }

  template <class T>
  void ChunkT<T>::set(const std::string & name, const T &v)
  {
		if(clearFirst)
	  m_variants.erase(name);
    m_variants.insert(std::pair<std::string, T>(name, v));
	
  }
   
  template <class T>
  void ChunkT<T>::setClearFlag(bool clearF)
  {
	  clearFirst=clearF;

  }

  template <class T>
  void ChunkT<T>::override(const std::string & name, const T &v)
  { 
    iterator it;
    while((it = m_variants.find(name)) != m_variants.end())
      m_variants.erase(it);
    m_variants.insert(std::pair<std::string, T>(name, v));
  }

  template <class T>
  void ChunkT<T>::dump(std::ostream& os)
  {
    for(iterator it = m_variants.begin();it != m_variants.end(); it++) {
      os << (*it).first << " {\n";
      (*it).second.dump(os);
      os << "}\n\n";
    }
  }

}

#endif
