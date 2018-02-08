#include "FfmpegVideoFormatSelector.hpp"

namespace VideoDisplay
{
  std::vector<VideoInputFormat> scanInputFormats(
      const QString &, AVInputFormat *, QMap<QString, QString>)
  {
    return std::vector<VideoInputFormat>();
  }
}
