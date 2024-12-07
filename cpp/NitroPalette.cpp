#include <NitroModules/ArrayBuffer.hpp>
#include "NitroPalette.hpp"
#include "MedianCut.hpp"

std::vector<std::string>
margelo::nitro::nitropalette::NitroPalette::extractColors(
    const std::shared_ptr<ArrayBuffer>& source, double width, double height,
    double colorCount, double quality) {
  if (colorCount < 1) colorCount = 1;
  if (colorCount > 20) colorCount = 20;

  // Clamp quality between 1 and 10
  quality = std::max(1.0, std::min(10.0, quality));

  const uint8_t* data = static_cast<const uint8_t*>(source->data());
  size_t size = source->size();

  // Calculate sampling rate based on quality
  int samplingRate =
      static_cast<int>(11 - quality);  // 1(high quality) to 10(low quality)

  std::vector<RGB> pixels;
  // Sample pixels based on quality setting
  for (size_t i = 0; i < size; i += (3 * samplingRate)) {
    pixels.emplace_back(data[i], data[i + 1], data[i + 2]);
  }

  auto compareBoxes = [](const ColorBox& a, const ColorBox& b) {
    return a.volume < b.volume;
  };
  std::priority_queue<ColorBox, std::vector<ColorBox>, decltype(compareBoxes)>
      boxes(compareBoxes);

  boxes.push(ColorBox(pixels));

  while (boxes.size() < colorCount) {
    if (boxes.empty()) break;

    ColorBox largestBox = boxes.top();
    boxes.pop();

    auto& boxPixels = largestBox.pixels;
    int dim = largestBox.dimension;
    std::sort(boxPixels.begin(), boxPixels.end(),
              [dim](const RGB& a, const RGB& b) {
                if (dim == 0) return a.r < b.r;
                if (dim == 1) return a.g < b.g;
                return a.b < b.b;
              });

    size_t medianIndex = boxPixels.size() / 2;
    std::vector<RGB> box1(boxPixels.begin(), boxPixels.begin() + medianIndex);
    std::vector<RGB> box2(boxPixels.begin() + medianIndex, boxPixels.end());

    if (!box1.empty()) boxes.push(ColorBox(box1));
    if (!box2.empty()) boxes.push(ColorBox(box2));
  }

  std::vector<std::string> result;
  while (!boxes.empty()) {
    const auto& box = boxes.top();
    long r = 0, g = 0, b = 0;

    for (const auto& pixel : box.pixels) {
      r += pixel.r;
      g += pixel.g;
      b += pixel.b;
    }

    size_t count = box.pixels.size();
    RGB averageColor(static_cast<uint8_t>(r / count),
                     static_cast<uint8_t>(g / count),
                     static_cast<uint8_t>(b / count));

    result.push_back(averageColor.toString());
    boxes.pop();
  }

  return result;
}
