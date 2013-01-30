#include "AVDecoder.hpp"

/// @todo this include is just for create(), should be removed
#include "AVDecoderFFMPEG.hpp"

namespace VideoDisplay
{
  AVDecoder::AVDecoder()
  {
    eventAddOut("error");
    eventAddOut("ready");
    eventAddOut("finished");
  }

  AVDecoder::~AVDecoder()
  {
  }

  std::unique_ptr<AVDecoder> AVDecoder::create(const Options & options, const QString & /*backend*/)
  {
    /// @todo add some great factory registry thing here
    std::unique_ptr<AVDecoder> decoder(new AVDecoderFFMPEG());
    decoder->load(options);
    return decoder;
  }
}
