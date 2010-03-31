/* COPYRIGHT
 *
 * This file is part of Resonant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Resonant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RESONANT_MODULE_SPLITTER_HPP
#define RESONANT_MODULE_SPLITTER_HPP

#include <Nimble/Ramp.hpp>
#include <Nimble/Vector2.hpp>

#include <Radiant/RefObj.hpp>
#include <Radiant/RefPtr.hpp>

#include <Resonant/Module.hpp>

#include <Valuable/ValueFloat.hpp>
#include <Valuable/ValueVector.hpp>

#include <vector>

namespace Resonant {


  /** Pans/splits audio signals to multiple outputs. 

      ModulePanner is used to handle multi-loudspeaker (or head-phone)
      situations, where the sound should move with videos or other
      visual content.

  */
  class ModulePanner : public Module
  {
  public:
    RESONANT_API ModulePanner(Application *);
    RESONANT_API virtual ~ModulePanner();

    RESONANT_API virtual Valuable::DOMElement serializeXML(Valuable::DOMDocument * doc);
    RESONANT_API virtual bool readElement(Valuable::DOMElement element);
    
    RESONANT_API virtual bool prepare(int & channelsIn, int & channelsOut);
    RESONANT_API virtual void processMessage(const char *, Radiant::BinaryData *);
    RESONANT_API virtual void process(float ** in, float ** out, int n);

    /** Creates a loudspeaker/headphone setup for full-HD displays.

        One source on the left at [0, 540], one source at right, at
        [1920, 540]. */

    RESONANT_API void makeFullHDStereo();

    RESONANT_API void setSpeaker(unsigned i, Nimble::Vector2 location);
    RESONANT_API void setSpeaker(unsigned i, float x, float y);

    void setCaptureRadius(float r) { m_maxRadius = r; }

  protected:

    void setSourceLocation(const std::string &, Nimble::Vector2 location);
    void removeSource(const std::string &);

    class LoudSpeaker : public Valuable::HasValues
    {
    public:
      LoudSpeaker()
	: m_location(this, "location", Nimble::Vector2(0,0))
      {
        setName("speaker");
      }

      Valuable::ValueVector2f m_location; // PixelLocation.
    };

    class Pipe
    {
    public:
      Pipe()
	: m_to(0)
      {
	m_ramp.reset(0.0);
      }
            
      bool done() {return (m_ramp.left() == 0) && (m_ramp.value() < 1.0e-4f);}
      
      Nimble::Rampf m_ramp;
      
      unsigned m_to;
    };

    enum { PIPES_PER_SOURCE = 6 };

    class Source
    {
    public:
      Source() : m_location(0, 0), m_updates(0) {}
      
      Nimble::Vector2 m_location;
      bool  m_updates;
      std::string  m_id;

      Pipe m_pipes[PIPES_PER_SOURCE];
    };

    typedef std::vector<Radiant::RefObj<Source> > Sources;
    typedef std::vector<Radiant::RefPtr<LoudSpeaker> > LoudSpeakers;

    Sources      m_sources;
    LoudSpeakers m_speakers;

    int m_outChannels;

    Valuable::ValueFloat m_maxRadius;
  };

}

#endif
