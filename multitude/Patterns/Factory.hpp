/* COPYRIGHT
 *
 * This file is part of Patterns.
 *
 * Copyright: Helsinki University of Technology, MultiTouch Oy and others.
 *
 * See file "Patterns.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */
#ifndef PATTERNS_FACTORY_HPP
#define PATTERNS_FACTORY_HPP

#include <Patterns/Export.hpp>
#include <Patterns/NotCopyable.hpp>
#include <Patterns/Singleton.hpp>

#include <Radiant/Trace.hpp>

#include <map>
#include <string>

namespace Patterns
{
  /// Class for fabricating new objects
  /** The factory contains a data-base of factory objects that can be
      used to manufacture new objects. This approach allows one to add
      factories that create objects that inherit from the basic
      template type "T". */
  template<class T>
  class PATTERNS_API Factory : NotCopyable
  {
    public:
      /// Produce a new object, using a factory object that matches the name
      static T * newProduct(const std::string & productName)
      {
        FactoryMap & lines = Lines::instance().lines;
        typename FactoryMap::iterator it = lines.find(productName);
    
        if(it != lines.end()) {
          Radiant::trace("Factory::newProduct # producing %s",
                         productName.c_str());

          return it->second->produce();
        } else {
          Radiant::trace("Factory # %s not registered", productName.c_str());
          return 0;
        }
      }

    protected:
      /// Helper typedef for referencing in derived classes
      typedef T Product;

      /// Automagically register factories
      Factory(const std::string & productName) 
      : m_productName(productName)
      {
        Radiant::trace("Factory # registering %s", productName.c_str());
        FactoryMap & lines = Lines::instance().lines;
        lines[productName] = this;
      }

      virtual ~Factory()
      {
        FactoryMap & lines = Lines::instance().lines;
        typename FactoryMap::iterator it = lines.find(m_productName);
        lines.erase(it);
      }

      /// Implement this to actually make something
      virtual T * produce() const = 0;

    private:
      std::string m_productName;

      typedef std::map<std::string, Factory<T> *> FactoryMap;

      class Lines : public Singleton<Lines>
      {
        public:
          FactoryMap lines;
      };

  };

}

#endif
