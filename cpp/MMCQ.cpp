#include "MMCQ.hpp"
#include <NitroModules/ArrayBuffer.hpp>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>

std::string MMCQ::Color::toString() const {
  std::ostringstream oss;
  oss << "rgb(" << static_cast<int>(r) << "," << static_cast<int>(g) << ","
      << static_cast<int>(b) << ")";
  return oss.str();
}

std::vector<MMCQ::Color> MMCQ::ColorMap::makePalette() const {
  std::vector<Color> palette;
  palette.reserve(vboxes.size());

  for (const auto& vbox : vboxes) {
    palette.push_back(vbox.getAverage());
  }

  return palette;
}

MMCQ::Color MMCQ::ColorMap::makeNearestColor(const MMCQ::Color& color) const {
  int minDistance = std::numeric_limits<int>::max();
  MMCQ::Color nearestColor(0, 0, 0);

  for (const auto& vbox : vboxes) {
    MMCQ::Color vboxColor = vbox.getAverage();
    int dr =
        std::abs(static_cast<int>(color.r) - static_cast<int>(vboxColor.r));
    int dg =
        std::abs(static_cast<int>(color.g) - static_cast<int>(vboxColor.g));
    int db =
        std::abs(static_cast<int>(color.b) - static_cast<int>(vboxColor.b));
    int distance = dr + dg + db;
    if (distance < minDistance) {
      minDistance = distance;
      nearestColor = vboxColor;
    }
  }
  return nearestColor;
}

void MMCQ::ColorMap::push(const MMCQ::VBox& vbox) { vboxes.push_back(vbox); }

MMCQ::ColorMap::ColorMap(const ColorMap& other) : vboxes(other.vboxes) {}

MMCQ::ColorMap& MMCQ::ColorMap::operator=(const ColorMap& other) {
  if (this != &other) {
    vboxes = other.vboxes;
  }
  return *this;
}

MMCQ::VBox::VBox(uint8_t rMin, uint8_t rMax, uint8_t gMin, uint8_t gMax,
                 uint8_t bMin, uint8_t bMax, std::vector<int>& histogram)
    : rMin(rMin),
      rMax(rMax),
      gMin(gMin),
      gMax(gMax),
      bMin(bMin),
      bMax(bMax),
      histogram(std::make_shared<std::vector<int>>(histogram)) {}

MMCQ::VBox::VBox(const VBox& vbox)
    : rMin(vbox.rMin),
      rMax(vbox.rMax),
      gMin(vbox.gMin),
      gMax(vbox.gMax),
      bMin(vbox.bMin),
      bMax(vbox.bMax),
      histogram(vbox.histogram) {}

MMCQ::VBox& MMCQ::VBox::operator=(const VBox& other) {
  if (this != &other) {
    rMin = other.rMin;
    rMax = other.rMax;
    gMin = other.gMin;
    gMax = other.gMax;
    bMin = other.bMin;
    bMax = other.bMax;
    histogram = other.histogram;
    average = other.average;
    volume = other.volume;
    count = other.count;
  }
  return *this;
}

MMCQ::VBox::Range MMCQ::VBox::makeRange(uint8_t min, uint8_t max) const {
  if (min <= max) {
    return Range{static_cast<int>(min), static_cast<int>(max) + 1};
  } else {
    return Range{
        static_cast<int>(max),
        static_cast<int>(min),
    };
  }
}

MMCQ::VBox::Range MMCQ::VBox::getRRange() const {
  return makeRange(rMin, rMax);
}

MMCQ::VBox::Range MMCQ::VBox::getGRange() const {
  return makeRange(gMin, gMax);
}

MMCQ::VBox::Range MMCQ::VBox::getBRange() const {
  return makeRange(bMin, bMax);
}

int MMCQ::VBox::getVolume(bool forceRecalculation) const {
  if (!forceRecalculation && volume.has_value()) {
    return volume.value();
  } else {
    int newVolume = (static_cast<int>(rMax) - rMin + 1) *
                    (static_cast<int>(gMax) - gMin + 1) *
                    (static_cast<int>(bMax) - bMin + 1);
    return newVolume;
  }
}

int MMCQ::VBox::getCount(bool forceRecalculation) const {
  if (!forceRecalculation && count.has_value()) {
    return count.value();
  } else {
    int totalCount = 0;
    for (int i = rMin; i <= rMax; i++) {
      for (int j = gMin; j <= gMax; j++) {
        for (int k = bMin; k <= bMax; k++) {
          int index = MMCQ::makeColorIndexOf(i, j, k);
          totalCount += histogram->at(index);
        }
      }
    }
    count = totalCount;
    return totalCount;
  }
}

MMCQ::Color MMCQ::VBox::getAverage(bool forceRecalculation) const {
  if (!forceRecalculation && average.has_value()) {
    MMCQ_DEBUG("Calculating box average"
               << (forceRecalculation ? " (forced)" : ""));

    return average.value();
  } else {
    int histogramValueSum = 0;
    int rSum = 0, gSum = 0, bSum = 0;

    for (int i = rMin; i <= rMax; i++) {
      for (int j = gMin; j <= gMax; j++) {
        for (int k = bMin; k <= bMax; k++) {
          int index = MMCQ::makeColorIndexOf(i, j, k);
          int histogramValue = histogram->at(index);
          histogramValueSum += histogramValue;
          rSum += static_cast<int>(static_cast<double>(histogramValue) *
                                   (static_cast<double>(i) + 0.5) *
                                   static_cast<double>(MULTIPLIER));
          gSum += static_cast<int>(static_cast<double>(histogramValue) *
                                   (static_cast<double>(j) + 0.5) *
                                   static_cast<double>(MULTIPLIER));
          bSum += static_cast<int>(static_cast<double>(histogramValue) *
                                   (static_cast<double>(k) + 0.5) *
                                   static_cast<double>(MULTIPLIER));
        }
      }
    }

    average.emplace(histogramValueSum > 0
                        ? Color(static_cast<uint8_t>(rSum / histogramValueSum),
                                static_cast<uint8_t>(gSum / histogramValueSum),
                                static_cast<uint8_t>(bSum / histogramValueSum))
                        : Color(static_cast<uint8_t>(
                                    std::min(MULTIPLIER *
                                                 (static_cast<int>(rMin) +
                                                  static_cast<int>(rMax) + 1) /
                                                 2,
                                             255)),
                                static_cast<uint8_t>(
                                    std::min(MULTIPLIER *
                                                 (static_cast<int>(gMin) +
                                                  static_cast<int>(gMax) + 1) /
                                                 2,
                                             255)),
                                static_cast<uint8_t>(
                                    std::min(MULTIPLIER *
                                                 (static_cast<int>(bMin) +
                                                  static_cast<int>(bMax) + 1) /
                                                 2,
                                             255))));
    MMCQ_DEBUG("Box average calculated - R:"
               << static_cast<int>(average.value().r)
               << " G:" << static_cast<int>(average.value().g)
               << " B:" << static_cast<int>(average.value().b));
    return average.value();
  }
}

MMCQ::ColorChannel MMCQ::VBox::widestColorChannel() const {
  int rWidth = rMax - rMin;
  int gWidth = gMax - gMin;
  int bWidth = bMax - bMin;

  if (rWidth >= gWidth && rWidth >= bWidth) {
    return ColorChannel::R;
  } else if (gWidth >= rWidth && gWidth >= bWidth) {
    return ColorChannel::G;
  } else {
    return ColorChannel::B;
  }
}

std::unique_ptr<MMCQ::ColorMap> MMCQ::quantize(
    const std::vector<uint8_t>& pixels, int maxColors, int quality,
    bool ignoreWhite) {
  MMCQ_LOG("Starting quantization process");
  MMCQ_DEBUG("Parameters: quality=" << quality
                                    << ", ignoreWhite=" << ignoreWhite
                                    << ", maxColors=" << maxColors);

  if (pixels.empty() && maxColors < 1 && maxColors > 255) {
    MMCQ_ERROR("Invalid input parameters");
    return nullptr;
  }

  MMCQ_LOG("Creating histogram and initial box");
  std::pair<std::vector<int, std::allocator<int>>, VBox> histogramAndBox =
      makeHistogramAndBox(pixels, quality, ignoreWhite);
  MMCQ_DEBUG("Initial histogram size: " << histogramAndBox.first.size());
  std::vector<VBox> pqueue;
  pqueue.push_back(histogramAndBox.second);
  int target = static_cast<int>(FRACTION_BY_POPULATION * maxColors);
  MMCQ_DEBUG("Target colors: " << target);

  MMCQ_LOG("Starting first iteration phase");

  iterate(pqueue, compareByCount, target, histogramAndBox.first);
  MMCQ_DEBUG("First iteration complete. Queue size: " << pqueue.size());

  MMCQ_LOG("Sorting by product");
  std::sort(pqueue.begin(), pqueue.end(), compareByProduct);
  MMCQ_LOG("Starting second iteration phase");

  iterate(pqueue, compareByProduct, maxColors, histogramAndBox.first);
  MMCQ_DEBUG("Second iteration complete. Final queue size: " << pqueue.size());
  std::reverse(pqueue.begin(), pqueue.end());
  MMCQ_LOG("Creating color map");

  MMCQ::ColorMap colorMap;
  for (const auto& vbox : pqueue) {
    colorMap.push(vbox);
  }
  MMCQ_LOG("Quantization complete");
  return std::make_unique<ColorMap>(colorMap);
}

int MMCQ::makeColorIndexOf(int red, int green, int blue) {
  return (red << (2 * SIGNAL_BITS)) + (green << SIGNAL_BITS) + blue;
}

std::pair<std::vector<int, std::allocator<int>>, MMCQ::VBox>
MMCQ::makeHistogramAndBox(
    const std::vector<uint8_t, std::allocator<uint8_t>>& pixels, int quality,
    bool ignoreWhite) {
  MMCQ_LOG("Creating histogram");
  std::vector<int> histogram(HISTOGRAM_SIZE, 0);
  uint8_t rMin, gMin, bMin = std::numeric_limits<uint8_t>::max();
  uint8_t rMax, gMax, bmax = std::numeric_limits<uint8_t>::min();

  if (pixels.size() % 4 != 0 || pixels.size() < 4) {
    MMCQ_ERROR("Invalid pixel data size: " << pixels.size());
    throw std::runtime_error("Invalid pixel data");
  }

  size_t pixelCount = pixels.size() / 4;
  MMCQ_DEBUG("Processing " << pixelCount << " pixels");
  for (size_t i = 0; i < pixelCount; i += quality) {
    uint8_t r = pixels[i * 4];
    uint8_t g = pixels[i * 4 + 1];
    uint8_t b = pixels[i * 4 + 2];
    uint8_t a = pixels[i * 4 + 3];

    if (a <= 125 || (ignoreWhite && r > 250 && g > 250 && b > 250)) {
      continue;
    }

    uint8_t shiftedR = r >> RIGHT_SHIFT;
    uint8_t shiftedG = g >> RIGHT_SHIFT;
    uint8_t shiftedB = b >> RIGHT_SHIFT;

    rMin = std::min(rMin, shiftedR);
    gMin = std::min(gMin, shiftedG);
    bMin = std::min(bMin, shiftedB);
    rMax = std::max(rMax, shiftedR);
    gMax = std::max(gMax, shiftedG);
    bmax = std::max(bmax, shiftedB);

    int index =
        makeColorIndexOf(static_cast<int>(shiftedR), static_cast<int>(shiftedG),
                         static_cast<int>(shiftedB));

    histogram[index]++;
  }
  MMCQ_DEBUG("Color ranges - R: " << (int)rMin << "-" << (int)rMax
                                  << " G: " << (int)gMin << "-" << (int)gMax
                                  << " B: " << (int)bMin << "-" << (int)bmax);

  MMCQ::VBox vbox(rMin, rMax, gMin, gMax, bMin, bmax, histogram);
  return {histogram, vbox};
}

std::vector<MMCQ::VBox, std::allocator<MMCQ::VBox>> MMCQ::applyMedianCut(
    const std::vector<int, std::allocator<int>>& histogram, const VBox& vbox) {
  if (vbox.getCount() == 0) {
    return {};
  }

  if (vbox.getCount() == 1) {
    return {vbox};
  }

  int total = 0;
  std::vector<int> partialSum;
  partialSum.reserve(VBOX_LENGTH);
  partialSum.resize(VBOX_LENGTH, -1);  // -1 = not set / 0 = 0

  ColorChannel axis = vbox.widestColorChannel();
  switch (axis) {
    case ColorChannel::R:
      for (int i = vbox.rMin; i <= vbox.rMax; i++) {
        int sum = 0;
        for (int j = vbox.gMin; j <= vbox.gMax; j++) {
          for (int k = vbox.bMin; k <= vbox.bMax; k++) {
            int index = MMCQ::makeColorIndexOf(i, j, k);
            sum += histogram[index];
          }
        }
        total += sum;
        partialSum[i] = total;
      }
      break;
    case ColorChannel::G:
      for (int i = vbox.gMin; i <= vbox.gMax; i++) {
        int sum = 0;
        for (int j = vbox.rMin; j <= vbox.rMax; j++) {
          for (int k = vbox.bMin; k <= vbox.bMax; k++) {
            int index = MMCQ::makeColorIndexOf(j, i, k);
            sum += histogram[index];
          }
        }
        total += sum;
        partialSum[i] = total;
      }
      break;
    case ColorChannel::B:
      for (int i = vbox.bMin; i <= vbox.bMax; i++) {
        int sum = 0;
        for (int j = vbox.rMin; j <= vbox.rMax; j++) {
          for (int k = vbox.gMin; k <= vbox.gMax; k++) {
            int index = MMCQ::makeColorIndexOf(j, k, i);
            sum += histogram[index];
          }
        }
        total += sum;
        partialSum[i] = total;
      }
      break;
  }

  std::vector<int> lookAheadSum;
  lookAheadSum.reserve(VBOX_LENGTH);
  lookAheadSum.resize(VBOX_LENGTH, -1);  // -1 = not set / 0 = 0
  for (int i = 0; i < VBOX_LENGTH; i++) {
    int cumulativeSum = partialSum[i];
    if (cumulativeSum != -1) {
      lookAheadSum[i] = total - cumulativeSum;
    }
  }

  std::vector<VBox> result;
  result.reserve(2);
  result = cut(axis, vbox, partialSum, lookAheadSum, total);
  return result;
}

std::vector<MMCQ::VBox> MMCQ::cut(ColorChannel axis, const VBox& vbox,
                                  const std::vector<int>& partialSum,
                                  const std::vector<int>& lookAheadSum,
                                  int total) {
  int vboxMin;
  int vboxMax;

  switch (axis) {
    case ColorChannel::R:
      vboxMin = static_cast<int>(vbox.rMin);
      vboxMax = static_cast<int>(vbox.rMax);
      break;
    case ColorChannel::G:
      vboxMin = static_cast<int>(vbox.gMin);
      vboxMax = static_cast<int>(vbox.gMax);
      break;
    case ColorChannel::B:
      vboxMin = static_cast<int>(vbox.bMin);
      vboxMax = static_cast<int>(vbox.bMax);
      break;
  }

  for (int i = vboxMin; i <= vboxMax; i++) {
    if (partialSum[i] > total / 2) {
      MMCQ::VBox vbox1(vbox);
      MMCQ::VBox vbox2(vbox);

      int left = i - vboxMin;
      int right = vboxMax - i;

      int d2;
      if (left <= right) {
        d2 = std::min(vboxMax - 1, i + right / 2);
      } else {
        d2 = std::max(vboxMin, i - 1 - left / 2);
      }

      while (d2 < 0 || partialSum[d2] <= 0) {
        d2++;
      }
      int rightPartitionCount = lookAheadSum[d2];
      while (rightPartitionCount == 0 && d2 > 0 && partialSum[d2 - 1] > 0) {
        d2--;
        rightPartitionCount = lookAheadSum[d2];
      }

      switch (axis) {
        case ColorChannel::R:
          vbox1.rMax = static_cast<uint8_t>(d2);
          vbox2.rMin = static_cast<uint8_t>(d2 + 1);
          break;
        case ColorChannel::G:
          vbox1.gMax = static_cast<uint8_t>(d2);
          vbox2.gMin = static_cast<uint8_t>(d2 + 1);
          break;
        case ColorChannel::B:
          vbox1.bMax = static_cast<uint8_t>(d2);
          vbox2.bMin = static_cast<uint8_t>(d2 + 1);
          break;
      }

      return {vbox1, vbox2};
    }
  }
  return {};
}

void MMCQ::iterate(std::vector<VBox>& queue,
                   bool (*comparator)(const VBox&, const VBox&), int target,
                   const std::vector<int>& histogram) {
  MMCQ_LOG("Starting iteration with target: " << target);
  int color = 1;

  for (int _ = 0; _ < MAX_ITERATIONS; _++) {
    MMCQ_DEBUG("Iteration " << _ << ", current colors: " << color);

    if (queue.empty()) {
      MMCQ_LOG("Queue is empty, terminating iteration");
      return;
    }
    VBox vbox = queue.back();

    if (vbox.getCount() == 0) {
      MMCQ_DEBUG("Empty box encountered, continuing");

      std::sort(queue.begin(), queue.end(), comparator);
      continue;
    }

    queue.pop_back();

    std::vector<VBox> vboxes = applyMedianCut(histogram, vbox);
    if (vboxes.empty()) {
      MMCQ_DEBUG("Median cut produced no boxes");
      continue;
    };
    queue.push_back(vboxes[0]);
    if (vboxes.size() == 2) {
      queue.push_back(vboxes[1]);
      color++;
      MMCQ_DEBUG("Split successful, new color count: " << color);
    }

    std::sort(queue.begin(), queue.end(), comparator);

    if (color >= target) {
      MMCQ_LOG("Target colors reached: " << color);
      return;
    };
  }
}

bool MMCQ::compareByCount(const VBox& a, const VBox& b) {
  return a.getCount() < b.getCount();
}

bool MMCQ::compareByProduct(const VBox& a, const VBox& b) {
  int aCount = a.getCount();
  int bCount = b.getCount();
  int aVolume = a.getVolume();
  int bVolume = b.getVolume();

  if (aCount == bCount) {
    return aVolume < bVolume;
  } else {
    int64_t aProduct =
        static_cast<int64_t>(aCount) * static_cast<int64_t>(aVolume);
    int64_t bProduct =
        static_cast<int64_t>(bCount) * static_cast<int64_t>(bVolume);
    return aProduct < bProduct;
  }
}
