#include "MedianCut.hpp"
#include <numeric>
#include <sstream>

RGB::RGB(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}

std::string RGB::toString() const {
  char rgbString[20];
  std::ostringstream oss;
  oss << "rgb(" << static_cast<int>(r) << "," << static_cast<int>(g) << ","
      << static_cast<int>(b) << ")";
  return oss.str();
  return std::string(rgbString);
}

ColorBox::ColorBox(const std::vector<RGB>& p) : pixels(p) {
  calculateRanges();
  calculateVolume();
  findLargestDimension();
}

void ColorBox::calculateRanges() {
  minR = 255, maxR = 0;
  minG = 255, maxG = 0;
  minB = 255, maxB = 0;

  for (const auto& pixel : pixels) {
    minR = std::min(minR, pixel.r);
    maxR = std::max(maxR, pixel.r);
    minG = std::min(minG, pixel.g);
    maxG = std::max(maxG, pixel.g);
    minB = std::min(minB, pixel.b);
    maxB = std::max(maxB, pixel.b);
  }

  rangeR = maxR - minR;
  rangeG = maxG - minG;
  rangeB = maxB - minB;
}

void ColorBox::calculateVolume() {
  if (pixels.empty()) {
    volume = 0;
    return;
  }
  volume = (rangeR + 1) * (rangeG + 1) * (rangeB + 1);
}

void ColorBox::findLargestDimension() {
  if (rangeR >= rangeG && rangeR >= rangeB) {
    dimension = 0;
  } else if (rangeG >= rangeR && rangeG >= rangeB) {
    dimension = 1;
  } else {
    dimension = 2;
  }
}

size_t ColorBox::getMemorySize() const {
  return sizeof(ColorBox) + (pixels.capacity() * sizeof(RGB));
}

std::pair<std::unique_ptr<ColorBox>, std::unique_ptr<ColorBox>>
ColorBox::split() const {
  if (pixels.size() < 2) {
    return {nullptr, nullptr};
  }

  // 選択された次元でピクセルをソート
  std::vector<RGB> sortedPixels = pixels;
  std::sort(sortedPixels.begin(), sortedPixels.end(),
            [this](const RGB& a, const RGB& b) {
              switch (dimension) {
                case 0:
                  return a.r < b.r;
                case 1:
                  return a.g < b.g;
                default:
                  return a.b < b.b;
              }
            });

  // 中央で分割
  size_t medianIndex = sortedPixels.size() / 2;
  std::vector<RGB> box1Pixels(sortedPixels.begin(),
                              sortedPixels.begin() + medianIndex);
  std::vector<RGB> box2Pixels(sortedPixels.begin() + medianIndex,
                              sortedPixels.end());

  return {std::make_unique<ColorBox>(box1Pixels),
          std::make_unique<ColorBox>(box2Pixels)};
}

RGB ColorBox::getAverageColor() const {
  if (pixels.empty()) {
    return RGB(0, 0, 0);
  }

  uint64_t totalR = 0, totalG = 0, totalB = 0;
  for (const auto& pixel : pixels) {
    totalR += pixel.r;
    totalG += pixel.g;
    totalB += pixel.b;
  }

  size_t count = pixels.size();
  return RGB(static_cast<uint8_t>(totalR / count),
             static_cast<uint8_t>(totalG / count),
             static_cast<uint8_t>(totalB / count));
}

MedianCut::MedianCut(const std::vector<RGB>& inputPixels)
    : pixels(inputPixels) {}

std::vector<RGB> MedianCut::quantize(int colorCount) {
  if (colorCount < 1) colorCount = 1;
  if (colorCount > 20) colorCount = 20;

  // 最初のボックスを作成
  boxes.clear();
  boxes.push_back(std::make_unique<ColorBox>(pixels));

  // 目標の色数に達するまでボックスを分割
  while (boxes.size() < colorCount) {
    // 最大ボリュームのボックスを見つける
    auto maxVolumeIt = std::max_element(
        boxes.begin(), boxes.end(),
        [](const auto& a, const auto& b) { return a->volume < b->volume; });

    if (maxVolumeIt == boxes.end() || (*maxVolumeIt)->pixels.size() < 2) {
      break;
    }

    // ボックスを分割
    auto [box1, box2] = (*maxVolumeIt)->split();
    if (box1 && box2) {
      boxes.erase(maxVolumeIt);
      boxes.push_back(std::move(box1));
      boxes.push_back(std::move(box2));
    } else {
      break;
    }
  }

  // 各ボックスの平均色を計算
  std::vector<RGB> palette;
  palette.reserve(boxes.size());
  for (const auto& box : boxes) {
    palette.push_back(box->getAverageColor());
  }

  return palette;
}

size_t MedianCut::getMemorySize() const {
  size_t totalSize = sizeof(MedianCut);
  totalSize += pixels.capacity() * sizeof(RGB);
  for (const auto& box : boxes) {
    totalSize += box->getMemorySize();
  }
  return totalSize;
}
