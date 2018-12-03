/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RESONANT_MODULE_SPLITTER_HPP
#define RESONANT_MODULE_SPLITTER_HPP

#include "Export.hpp"
#include "Module.hpp"
#include "SoundRectangle.hpp"

#include <Nimble/Ramp.hpp>
#include <Nimble/Vector2.hpp>

#include <memory>

#include <Valuable/AttributeFloat.hpp>
#include <Valuable/AttributeVector.hpp>
#include <Valuable/AttributeContainer.hpp>

#include <vector>

namespace Resonant {
	
  /** Pans/splits audio signals to multiple outputs.

      ModulePanner is used to handle multi-loudspeaker (or head-phone)
      situations, where the sound should move with videos or other
      visual content.

      Currently, there are two panning modes, see #Mode
  */
  class RESONANT_API ModulePanner : public Module
  {
  public:

    /// @cond

    class Pipe
    {
    public:
      Pipe()
      {
        m_ramp.reset(0.0);
      }

      bool isDone() const { return (m_ramp.left() == 0) && (m_ramp.value() < 1.0e-4f);}

      Nimble::Rampf m_ramp;

      unsigned m_from = 0;
      unsigned m_to = 0;
    };

    class Source
    {
    public:
      std::map<QByteArray, Nimble::Vector2f> locations;
      QByteArray moduleId;
      long generation = 0; /// @see ModulePanner::m_generation
      unsigned int channels = 0;
      unsigned int channelOffset = 0;

      std::vector<Pipe> pipes;
    };

    class LoudSpeaker : public Valuable::Node
    {
    public:
      LoudSpeaker()
        : m_location(this, "location", Nimble::Vector2(1111111,0))
      {
        setName("speaker");
      }

      Valuable::AttributeVector2f m_location; // PixelLocation.
    };

    typedef std::vector<Source> Sources;
    typedef std::vector<SoundRectangle*> Rectangles;
    typedef std::vector<std::shared_ptr<LoudSpeaker>> LoudSpeakers;

    /// @endcond

  public:
    /// Panning mode
    enum Mode {
      /// Radial mode, where the panning is based on the distance of loudspeaker and
      /// sound source.
      /// @sa ModulePanner::setCaptureRadius
      RADIAL = 0,
      /// Rectangles, where the panning is based on rectangular regions
      /// @sa ModulePanner::addSoundRectangle
      /// @sa SoundRectangle
      RECTANGLES = 1
    };


    /// Constructs the panner module
    ModulePanner(Mode mode=RADIAL);
    virtual ~ModulePanner();

    virtual bool deserialize(const Valuable::ArchiveElement & element) OVERRIDE;

    virtual bool prepare(int & channelsIn, int & channelsOut) OVERRIDE;
    virtual void eventProcess(const QByteArray &, Radiant::BinaryData &) OVERRIDE;
    virtual void process(float ** in, float ** out, int n, const CallbackTime &) OVERRIDE;

    /** Creates a loudspeaker/headphone setup for full-HD displays.

        One source on the left at [0, 540], one source at right, at
        [1920, 540]. */

    void makeFullHDStereo();

    /// Sets the radius for the distance for collecting the audio to a single loudspeaker.
    /// Only has an effect if using the radial #Mode.
    /// When a given sound source gets closer than he maximum radius its volume is faded in
    /// so that at radius/2 the volume is at 100% (aka unity gain).
    /// @param r Radial mode radius
    void setCaptureRadius(float r) { m_maxRadius = r; ++m_generation; }

    /// Add a SoundRectangle. The ownership is transferred to this object.
    void addSoundRectangle(SoundRectangle * r);

    /// Sets the panner mode
    /// @param mode New panner mode
    void setMode(Mode mode);
    /// Query the current panner mode
    /// @return Current panner mode
    Mode getMode() const;

    /// @cond

    /// These are only used by CalibrationOverlayWidget, see m_sourceMutex
    const Rectangles & rectangles() const { return *m_rectangles; }
    Sources sources() const;
    const LoudSpeakers & speakers() const { return *m_speakers; }

    int channels() const;

    void addSource(const QByteArray & moduleId, unsigned int channels);
    void removeSource(const QByteArray & moduleId);

    /// @endcond

  private:

    friend class ModuleSamplePlayer;

    /// @todo this should be removed. There is no one to one mapping between
    ///       a location and a channel
    int locationToChannel(Nimble::Vector2) const;
    /// Adds or update a source location
    /// @param id Source id
    /// @param path Location name. One source can have several locations. When
    ///             used with widgets, this is typically
    ///             Luminous::RenderContext::viewWidgetPathId. This can be also
    ///             an empty string.
    /// @param location source location
    void setSourceLocation(const QByteArray & moduleId, const QByteArray & path, Nimble::Vector2 location);
    /// Removes a single location from a source
    void clearSourceLocation(const QByteArray & moduleId, const QByteArray & path);
    void syncSource(Source & src);
    void addSoundRectangleSpeakers(SoundRectangle * r);

    /// @cond


    /// Computes the gain for the given channel based on sound source location
    virtual float computeGain(unsigned int inputChannel, unsigned int outputChannel, Nimble::Vector2 srcLocation) const;

    float computeGainRadial(unsigned int channel, Nimble::Vector2 srcLocation) const;
    float computeGainRectangle(unsigned int channel, Nimble::Vector2 srcLocation) const;

    void updateChannelCount();

    /// sources() is the only function that is used outside the audio thread,
    /// so to make that function thread-safe, we just need to lock this mutex
    /// in sources() and everywhere where we modify m_sources,
    /// Source::locations or Source::pipes vectors. This could mean that the data that
    /// sources() returns might not be fully consistent, but that doesn't matter.
    /// The function is undocumented and only used by the
    /// CalibrationOverlayWidget.
    mutable Radiant::Mutex m_sourceMutex;
    Sources      m_sources;
    /// Used only with RADIAL mode
    Valuable::AttributeContainer<LoudSpeakers> m_speakers;

    unsigned int m_channelCount = 0;

    /// generation is increased every time speaker setup is changed
    long m_generation;

    Valuable::AttributeFloat m_maxRadius;
    /// Used only with RECTANGLES mode
    Valuable::AttributeContainer<Rectangles> m_rectangles;
    Valuable::AttributeInt m_operatingMode;
    /// @endcond
  };

}

#endif
