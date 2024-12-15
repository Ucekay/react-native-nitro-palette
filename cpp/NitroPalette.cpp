#include <NitroModules/ArrayBuffer.hpp>
#include "NitroPalette.hpp"
#include "MMCQ.hpp"

std::vector<std::string>
margelo::nitro::nitropalette::NitroPalette::extractColors(
    const std::shared_ptr<ArrayBuffer>& source, double colorCount,
    double quality, bool ignoreWhite) {
  currentImageSize_ = source->size();
  if (!source || currentImageSize_ < 4) {
    return {};
  }

  if (currentImageSize_ % 4 != 0) {
    return {};
  }

  colorCount = std::clamp(colorCount, 1.0, 20.0);
  quality = std::clamp(quality, 1.0, 10.0);

  auto colorMap = MMCQ::quantize(source, static_cast<int>(colorCount),
                                 static_cast<int>(quality), ignoreWhite);

  auto palette = colorMap->makePalette();

  if (palette.empty()) {
    return {};
  }

  std::vector<std::string> result;
  result.reserve(colorCount);
  for (int i = 0; i < colorCount; i++) {
    result.push_back(palette[i].toString());
  }
  return result;
}
