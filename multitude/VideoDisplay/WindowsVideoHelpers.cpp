#include "WindowsVideoHelpers.hpp"

namespace VideoDisplay
{

  QString VideoInput::asString() const
  {
    return !devicePath.isEmpty() ? devicePath : friendlyName;
  }

  bool VideoInput::operator==(const VideoInput& other) const
  {
    if(!devicePath.isEmpty() || !other.devicePath.isEmpty())
      return devicePath == other.devicePath;
    return friendlyName == other.friendlyName;
  }

  bool VideoInput::operator<(const VideoInput& other) const
  {
    const bool hasDev1 = !devicePath.isEmpty();
    const bool hasDev2 = !other.devicePath.isEmpty();
    if(!hasDev1 && !hasDev2) {
      return friendlyName < other.friendlyName;
    } else if(!hasDev2) {
      return true;
    } else if(!hasDev1){
      return false;
    } else {
      return devicePath < other.devicePath;
    }
  }

  // ---------------------------------------------------------------------

  bool AudioInput::isValid() const
  {
    // our audio is always referred with friendly name
    return !friendlyName.isEmpty();
  }

  QString AudioInput::asString() const
  {
    // if we happen to change this
    return friendlyName;
  }

  bool AudioInput::operator==(const AudioInput& other) const
  {
    return waveInId == other.waveInId && friendlyName == other.friendlyName;
  }

  bool AudioInput::operator<(const AudioInput& other) const
  {
    const bool hasWave1 = waveInId != -1;
    const bool hasWave2 = other.waveInId != -1;
    if(!hasWave1 && !hasWave2) {
      return friendlyName < other.friendlyName;
    } else if(!hasWave2) {
      return true;
    } else if(!hasWave1) {
      return false;
    } else {
      return waveInId < other.waveInId;
    }
  }

  // ---------------------------------------------------------------------

  QString Source::ffmpegName() const
  {
    QString result = "video="+video.asString();
    if(audio.isValid())
      result += ":audio=" + audio.asString();
    return result;
  }

  SourceState Source::update()
  {
    SourceState state;
    state.enabled = true;
    return state;
  }

  bool Source::isValid() const
  {
    return !video.asString().isEmpty();
  }

  bool Source::operator<(const Source& other) const
  {
    return std::tie(video, audio) < std::tie(other.video, other.audio);
  }

  bool Source::operator==(const Source& other) const
  {
    return video == other.video && audio == other.audio;
  }
}
