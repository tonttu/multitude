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

      unsigned m_sourceChannel = 0;
      unsigned m_outputChannel = 0;
    };

    struct SourceLocation
    {
      Nimble::Vector2f location{0, 0};
      float gain = 0;

      inline bool operator==(const SourceLocation & o) const
      {
        return location == o.location && gain == o.gain;
      }
    };

    class Source
    {
    public:
      /// One audio source can be multiplexed to several locations with
      /// different audio gains, for example a single video through several
      /// ViewWidgets or several VideoWidgets sharing the same AVDecoder.
      std::map<QByteArray, SourceLocation> locations;
      /// Resonant::Module::id
      QByteArray moduleId;
      /// @see ModulePanner::m_generation
      long generation = 0;
      /// Number of channels in this source
      unsigned int channelCount = 0;
      /// The total number of channels with 'in' parameter given to
      /// ModulePanner::process is the sum of all channels in all Sources in
      /// ModulePanner::m_sources. The channels are in the same order as this
      /// m_sources vector and channelOffset is the sum of all channels in all
      /// Sources in m_sources vector before this object. Channels for this
      /// Source are channelCount channels starting at in[channelOffset].
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
      /// Rectangles, where the panning is based on rectangular regions. Any
      /// multichannel audio sources are mixed to a mono, so that they can have
      /// a single positional inside a sound rectangle. If you don't care about
      /// sound position inside a sound rectangle and wish to keep stereo
      /// sources as stereo, consider using STEREO_ZONES instead.
      /// @sa ModulePanner::addSoundRectangle
      /// @sa SoundRectangle
      RECTANGLES = 1,
      /// Similar to RECTANGLES, but instead of having a positional audio inside
      /// a sound rectangle, the rectangles are just separate stereo audio zones.
      /// Any stereo sound inside one audio zone will remain stereo, so left
      /// and right channels from a audio source, like a video, are played
      /// through left and right sound rectangle speaker without mixing the
      /// audio to mono. Mono audio sources are played on both sound rectangle
      /// channels and from audio sources with more than two channels, only
      /// the first two channels are played.
      /// You might want to set SoundRectangle::stereoPan to zero, since it
      /// will just adjust stereo channel balance instead of giving an
      /// impression of positional audio. If you wish to use stereoPan to have
      /// positional audio inside one sound rectangle, use RECTANGLES mode
      /// instead.
      STEREO_ZONES = 2,
      /// Audio panning module is used for controlling audio source gain, but
      /// actual panning is not done. Rectangles and loudspeakers are not used.
      /// This is useful for muting videos that are not visible and controlling
      /// gain of multiple VideoWidgets using the same shared decoder and different
      /// gain values. MultiWidgets::Application will set up PASS_THROUGH panner
      /// by default if there's no --audio-config defined.
      PASS_THROUGH = 3,
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
    Mode mode() const;

    /// @cond

    /// These are only used by CalibrationOverlayWidget, see m_sourceMutex
    const Rectangles & rectangles() const { return *m_rectangles; }
    Sources sources() const;
    const LoudSpeakers & speakers() const { return *m_speakers; }

    int channels() const;

    void setPassthroughChannelCount(unsigned int channelCount);

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
    void setSourceLocation(const QByteArray & moduleId, const QByteArray & path, Nimble::Vector2 location, float gain);
    /// Removes a single location from a source
    void clearSourceLocation(const QByteArray & moduleId, const QByteArray & path);
    void syncSource(Source & src);
    void addSoundRectangleSpeakers(SoundRectangle * r);

    /// @cond


    /// Computes the gain for the given channel based on sound source location
    virtual float computeGain(unsigned int sourceChannel, unsigned int outputChannel,
                              unsigned int sourceChannelCount, Nimble::Vector2 srcLocation) const;

    float computeGainRadial(unsigned int outputChannel, Nimble::Vector2 srcLocation) const;
    float computeGainRectangle(unsigned int outputChannel, Nimble::Vector2 srcLocation,
                               unsigned int sourceChannel, unsigned int sourceChannelCount, bool stereo) const;

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
