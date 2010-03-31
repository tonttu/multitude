/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_GLRESOURCES_HPP
#define LUMINOUS_GLRESOURCES_HPP

#include <Luminous/Export.hpp>
#include <Luminous/MultiHead.hpp>

#include <Radiant/ResourceLocator.hpp>

#include <map>

namespace Luminous
{
  class Collectable;
  class GarbageCollector;
  class GLResource;

  /// Collection of OpenGL-context -specific resources (textures, FBOs etc).
  /** This class is also used to store information about how much
      resources are used (texture, FBO -memory etc).

      This object also keeps track of change history in
      allocation/deallocation sums. This information is handy when you
      want to make sure that you do not push too many texture pixels
      to the GPU during one frame etc.
      
      The GLResource objects are deleted if they are too old -
      i.e. not used for some time. The old objects are only deleted
      when the GPU RAM usage exceeds given threshold. By default the
      threshold is 70MB, but it can be changed by setting environment
      variable MULTI_GPU_RAM to a numeric value that represents the
      number of GPU megabytes that one allowed to use. The value can
      also be changed programmatically, using the setComfortableGPURAM
      function.

      For example setting MULTI_GPU_RAM to 200, GLResources starts to
      drop old resources from GPU as the GPU RAM usage exceeds 200MB.
  */
  class LUMINOUS_API GLResources
  {
  public:

    typedef std::map<const Collectable *, GLResource *> container;
    typedef container::iterator iterator;

    GLResources(Radiant::ResourceLocator & rl);
    virtual ~GLResources();

    /// Get a handle to a resource 
    GLResource * getResource(const Collectable * key);
    /// Adds a resource
    void addResource(const Collectable * key, GLResource * resource);
    /// Erases a single GLResource
    bool eraseResource(const Collectable * key);
    /// Erase the resources that are no longer required
    void eraseResources();
    /// Erases all resources.
    void clear();
    /// Tell the resource manager that byte consumption was changed
    /** Individual resource objects should call this function when
      their byte consumption changes. A typical example might be a
      texture object that is resized, or gets new mipmaps. */
    void changeByteConsumption(long deallocated, long allocated);
    
    /// Total number of bytes used on the GPU
    long consumesBytes() const { return m_consumingBytes; }

    /// Number of bytes deallocated since last counter reset
    long deallocationSum() { return m_deallocationSum; }
    /// Number of bytes allocated since last counter reset
    long allocationSum()  { return m_allocationSum; }

    /// Resets the allocation/deallocation sum counters
    /** A typical place to call this function is right before
      rendering the OpenGL scene. */
    void resetSumCounters() { m_deallocationSum = m_allocationSum = 0; }

    void deleteAfter(GLResource * resource, int frames);
    /** Sets the threshold for deleting old objects from GPU memory. */
    void setComfortableGPURAM(long bytes)
    { m_comfortableGPURAM = bytes; }

    Radiant::ResourceLocator & resourceLocator() { return m_resourceLocator; }

    // static void setThreadResources(GLResources *);
    static void setThreadResources(GLResources *,
				   const MultiHead::Window *,
				   const MultiHead::Area *);
    static GLResources * getThreadResources();
    static void getThreadMultiHead(const MultiHead::Window ** w,
				   const MultiHead::Area **);
 
 private:
    
    void eraseOnce();

    container m_resources;

    long m_deallocationSum;
    long m_allocationSum;
    /// The number of bytes that reside on th GPU
    /** This number is likely to be quite approximate as we cannot
	estimate exactly much GPU memory a particular object uses. */
    long m_consumingBytes;
    /** The maximum amount of GPU RAM to use before starting to erase
	objects. */
    long m_comfortableGPURAM;

    long m_frame;

    Radiant::ResourceLocator & m_resourceLocator;
  };
}

/// A macro for creating accessing GLResource objects
/** This macro will try to find a #GLResource object from the
    resources. If the relevant object is not accessible, then new
    object is created. This macro will also define the object so that
    it is available after this macro has been called.

    @arg type The class name of the object to be found (e.g. Texture2D etc).
    
    @arg name The variable name for this object (e.g. mytex etc.).

    @karg ey The object that this resource is related to. Often the
    this-pointer is used as the key, but one can create other keys.

    @arg resources The GLResources object that is holding the OpenGL
    resources for this thread.
*/
#define GLRESOURCE_ENSURE(type, name, key, resources)	\
  type * name = dynamic_cast<type *> (resources->getResource(key));	\
  if(!name) { \
    name = new type(resources); \
    resources->addResource(key, name); \
  }

#define GLRESOURCE_ENSURE2(type, name, key)	\
  Luminous::GLResources * grs = Luminous::GLResources::getThreadResources(); \
  type * name = dynamic_cast<type *> (grs->getResource(key));	\
  if(!name) { \
    name = new type();	\
    grs->addResource(key, name); \
  }

#define GLRESOURCE_ENSURE3(type, name, key)	\
  Luminous::GLResources * grs = Luminous::GLResources::getThreadResources(); \
  type * name = dynamic_cast<type *> (grs->getResource(key));	\
  if(!name) { \
    name = new type(grs);	\
    grs->addResource(key, name); \
  }

#endif
