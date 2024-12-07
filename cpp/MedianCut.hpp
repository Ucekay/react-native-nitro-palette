#pragma once
#include <vector>
#include <queue>
#include <algorithm>
#include <cstdint>
#include <string>
#include <memory>

struct RGB {
  uint8_t r, g, b;
  RGB(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0);
  std::string toString() const;
};

class ColorBox {
 public:
  std::vector<RGB> pixels;
  int volume;
  int dimension;

  explicit ColorBox(const std::vector<RGB>& p);
  size_t getMemorySize() const;

  std::pair<std::unique_ptr<ColorBox>, std::unique_ptr<ColorBox>> split() const;
  RGB getAverageColor() const;

 private:
  uint8_t minR, maxR, minG, maxG, minB, maxB, rangeR, rangeG, rangeB;
  void calculateRanges();
  void calculateVolume();
  void findLargestDimension();
};

class MedianCut {
 public:
  explicit MedianCut(const std::vector<RGB>& pixels);
  std::vector<RGB> quantize(int colorCount);
  size_t getMemorySize() const;

 private:
  std::vector<RGB> pixels;
  std::vector<std::unique_ptr<ColorBox>> boxes;
};
