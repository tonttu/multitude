#ifndef VIDEO_DISPLAY_FFMPEG_VIDEO_FORMAT_SELECTOR_HPP
#define VIDEO_DISPLAY_FFMPEG_VIDEO_FORMAT_SELECTOR_HPP

#include "AVDecoder.hpp"

#include <vector>

#include <QMap>

extern "C" struct AVInputFormat;

namespace VideoDisplay
{
  struct VideoInputFormat
  {
    enum FormatCategory
    {
      /// Unrecognized or unsupported format
      FORMAT_UNKNOWN,
      /// Raw RGB stream, best quality and biggest bandwidth
      FORMAT_RGB,
      /// Raw YUV stream, great quality but typically only half of the
      /// bandwidth compared to RGB
      FORMAT_YUV,
      /// MJPEG or other compressed format, sometimes low quality
      FORMAT_COMPRESSED,
    };

    FormatCategory category = FORMAT_UNKNOWN;
    /// Input source pin, for instance a capture card might have one pin for
    /// each connector.
    QString pin;
    /// If this is a compressed format, then this is something like "mjpeg"
    QString vcodec;
    /// If this is raw format, this is the pixel format, for example "yuv420p"
    QString pixelFormat;
    Nimble::SizeI resolution;
    double fps = 0;
  };

  /// Returns list of available input formats for the given dshow/v4l2 input source
  VIDEODISPLAY_API std::vector<VideoInputFormat> scanInputFormats(
      const QString & input, AVInputFormat * inputFormat, QMap<QString, QString> options);

  /// Given the list generated with scanInputFormats, choose the best format
  /// from all formats that match the criteria given in AVDecoder::Options.
  VIDEODISPLAY_API const VideoInputFormat * chooseFormat(
      const std::vector<VideoInputFormat> & formats,
      const AVDecoder::Options & avOptions);

  /// Sets the VideoInputFormat options to AVDecoder::Options
  VIDEODISPLAY_API void applyFormatOptions(const VideoInputFormat & format,
                                           AVDecoder::Options & avOptions);
}

#endif
