#include <NitroModules/ArrayBuffer.hpp>
#include "NitroPalette.hpp"
#include "MMCQ.hpp"

std::vector<std::string>
margelo::nitro::nitropalette::NitroPalette::extractColors(
    const std::shared_ptr<ArrayBuffer>& source, double width, double height,
    double colorCount, double quality) {
  // バッファサイズのチェックを4バイトに修正
  if (!source || source->size() < 4) {
    return {};
  }

  // バッファサイズがRGBAの倍数であることを確認
  if (source->size() % 4 != 0) {
    return {};
  }

  // Clamp parameters
  colorCount = std::clamp(colorCount, 1.0, 20.0);
  quality = std::clamp(quality, 1.0, 10.0);

  // MedianCutを使用してパレットを生成
  auto colorMap = MMCQ::quantize(source, static_cast<int>(colorCount),
                                 static_cast<int>(quality),
                                 true  // ignoreWhite
  );

  auto palette = colorMap->makePalette();

  // パレットが空の場合は早期リターン
  if (palette.empty()) {
    return {};
  }

  // 文字列表現に変換
  std::vector<std::string> result;
  result.reserve(palette.size());
  for (const auto& color : palette) {
    result.push_back(color.toString());
  }

  return result;
}
