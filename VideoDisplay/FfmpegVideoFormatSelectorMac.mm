#include "FfmpegVideoFormatSelector.hpp"

#import <AVFoundation/AVFoundation.h>

namespace VideoDisplay
{
  std::vector<VideoInputFormat> scanInputFormats(
      const QString & input, AVInputFormat *, QMap<QString, QString>)
  {
    QString inputName = input.section(':', 0, 0);
    bool isInt = false;
    int inputIndex = inputName.toInt(&isInt);

    std::vector<VideoInputFormat> out;

    QMacAutoReleasePool pool;
    NSArray * devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    int index = 0;
    for (AVCaptureDevice * dev: devices) {
      const char * name = [[dev localizedName] UTF8String];
      if ((isInt && index == inputIndex) || (!isInt && name == inputName)) {
        for (AVCaptureDeviceFormat * format: [dev formats]) {
          auto res = CMVideoFormatDescriptionGetDimensions([format formatDescription]);
          for (AVFrameRateRange * range: [format videoSupportedFrameRateRanges]) {
            double fps = [range maxFrameRate];

            /// TODO: pixel format using CMFormatDescriptionGetMediaSubType

            VideoInputFormat format;
            format.fps = fps;
            format.resolution.make(res.width, res.height);
            out.push_back(format);
          }
        }
        break;
      }
    }
    return out;
  }
}
