#include "MMCQ.hpp"
#include <NitroModules/ArrayBuffer.hpp>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>

const double MMCQ::FRACTION_BY_POPULATION = 0.75;

std::string MMCQ::Color::toString() const {
  std::ostringstream oss;
  oss << "rgb(" << static_cast<int>(r) << "," << static_cast<int>(g) << ","
      << static_cast<int>(b) << ")";
  return oss.str();
}

MMCQ::Color::Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}

MMCQ::VBox::VBox(uint8_t rMin, uint8_t rMax, uint8_t gMin, uint8_t gMax,
                 uint8_t bMin, uint8_t bMax, const std::vector<int>& histogram)
    : rMin(rMin),
      rMax(rMax),
      gMin(gMin),
      gMax(gMax),
      bMin(bMin),
      bMax(bMax),
      histogram(histogram) {}

MMCQ::VBox::VBox(const VBox& other)
    : rMin(other.rMin),
      rMax(other.rMax),
      gMin(other.gMin),
      gMax(other.gMax),
      bMin(other.bMin),
      bMax(other.bMax),
      histogram(other.histogram),
      volume(other.volume),
      count(other.count) {
  if (other.average) {
    average = std::make_unique<Color>(*other.average);
  }
}

MMCQ::VBox::VBox(VBox&& other) noexcept
    : histogram(std::move(other.histogram)),
      average(std::move(other.average)),
      volume(other.volume),
      count(other.count) {
  rMin = other.rMin;
  rMax = other.rMax;
  gMin = other.gMin;
  gMax = other.gMax;
  bMin = other.bMin;
  bMax = other.bMax;

  // 移動元のオブジェクトをリセット
  other.volume = 0;
  other.count = -1;
}

MMCQ::VBox& MMCQ::VBox::operator=(const VBox& other) {
  if (this != &other) {
    rMin = other.rMin;
    rMax = other.rMax;
    gMin = other.gMin;
    gMax = other.gMax;
    bMin = other.bMin;
    bMax = other.bMax;
    histogram = other.histogram;
    volume = other.volume;
    count = other.count;
    if (other.average) {
      average = std::make_unique<Color>(*other.average);
    } else {
      average.reset();
    }
  }
  return *this;
}

MMCQ::VBox& MMCQ::VBox::operator=(VBox&& other) noexcept {
  if (this != &other) {
    rMin = other.rMin;
    rMax = other.rMax;
    gMin = other.gMin;
    gMax = other.gMax;
    bMin = other.bMin;
    bMax = other.bMax;
    histogram = std::move(other.histogram);
    average = std::move(other.average);
    volume = other.volume;
    count = other.count;
  }
  return *this;
}

int MMCQ::makeColorIndexOf(int red, int green, int blue) {
  // 範囲チェックを追加
  if (red < 0 || red >= (1 << SIGNAL_BITS) || green < 0 ||
      green >= (1 << SIGNAL_BITS) || blue < 0 || blue >= (1 << SIGNAL_BITS)) {
    throw std::out_of_range("Color index out of range");
  }
  return (red << (2 * SIGNAL_BITS)) + (green << SIGNAL_BITS) + blue;
}

int MMCQ::VBox::getVolume(bool forceRecalculate) const {
  if (volume == 0 || forceRecalculate) {
    volume = (static_cast<int>(rMax) - static_cast<int>(rMin) + 1) *
             (static_cast<int>(gMax) - static_cast<int>(gMin) + 1) *
             (static_cast<int>(bMax) - static_cast<int>(bMin) + 1);
  }
  return volume;
}

int MMCQ::VBox::getCount(bool forceRecalculate) const {
  if (count != -1 && !forceRecalculate) {
    return count;
  }

  int npix = 0;
  for (int i = rMin; i <= rMax; i++) {
    for (int j = gMin; j <= gMax; j++) {
      for (int k = bMin; k <= bMax; k++) {
        int index = makeColorIndexOf(i, j, k);
        npix += histogram[index];
      }
    }
  }
  count = npix;
  return count;
}

MMCQ::Color MMCQ::VBox::getAverage(bool forceRecalculate) const {
  if (average && !forceRecalculate) {
    return *average;
  }

  long long ntot = 0;
  long long rSum = 0;
  long long gSum = 0;
  long long bSum = 0;

  for (int i = rMin; i <= rMax; i++) {
    for (int j = gMin; j <= gMax; j++) {
      for (int k = bMin; k <= bMax; k++) {
        int index = makeColorIndexOf(i, j, k);
        int hval = histogram[index];
        ntot += hval;
        rSum += hval * (i + 0.5) * MULTIPLIER;
        gSum += hval * (j + 0.5) * MULTIPLIER;
        bSum += hval * (k + 0.5) * MULTIPLIER;
      }
    }
  }

  Color avg;
  if (ntot > 0) {
    avg.r = static_cast<uint8_t>(rSum / ntot);
    avg.g = static_cast<uint8_t>(gSum / ntot);
    avg.b = static_cast<uint8_t>(bSum / ntot);
  } else {
    avg.r =
        static_cast<uint8_t>(std::min(MULTIPLIER * (rMin + rMax + 1) / 2, 255));
    avg.g =
        static_cast<uint8_t>(std::min(MULTIPLIER * (gMin + gMax + 1) / 2, 255));
    avg.b =
        static_cast<uint8_t>(std::min(MULTIPLIER * (bMin + bMax + 1) / 2, 255));
  }

  average = std::make_unique<Color>(avg);
  return avg;
}

MMCQ::VBox::ColorChannel MMCQ::VBox::widestColorChannel() const {
  int rWidth = rMax - rMin;
  int gWidth = gMax - gMin;
  int bWidth = bMax - bMin;

  int maxWidth = std::max({rWidth, gWidth, bWidth});

  if (maxWidth == rWidth) return ColorChannel::R;
  if (maxWidth == gWidth) return ColorChannel::G;
  return ColorChannel::B;
}

void MMCQ::ColorMap::push(VBox&& vbox) { vboxes.push_back(std::move(vbox)); }

std::vector<MMCQ::Color> MMCQ::ColorMap::makePalette() const {
  std::vector<Color> palette;

  // サイズチェックを追加
  if (vboxes.size() == 0) {
    palette.push_back(Color(0, 0, 0));
    return palette;
  }

  palette.reserve(vboxes.size());
  for (const auto& vbox : vboxes) {
    palette.push_back(vbox.getAverage());
  }

  return palette;
}

MMCQ::Color MMCQ::ColorMap::makeNearestColor(const Color& color) const {
  int nearestDistance = std::numeric_limits<int>::max();
  Color nearestColor;

  for (const auto& vbox : vboxes) {
    Color vbColor = vbox.getAverage();
    int dr = std::abs(static_cast<int>(color.r) - static_cast<int>(vbColor.r));
    int dg = std::abs(static_cast<int>(color.g) - static_cast<int>(vbColor.g));
    int db = std::abs(static_cast<int>(color.b) - static_cast<int>(vbColor.b));
    int distance = dr + dg + db;

    if (distance < nearestDistance) {
      nearestDistance = distance;
      nearestColor = vbColor;
    }
  }

  return nearestColor;
}

MMCQ::HistogramAndVBox MMCQ::makeHistogramAndVBox(
    const std::vector<uint8_t>& pixels, int quality, bool ignoreWhite) {
  std::vector<int> histogram(HISTOGRAM_SIZE, 0);
  uint8_t rMin = 255, rMax = 0;
  uint8_t gMin = 255, gMax = 0;
  uint8_t bMin = 255, bMax = 0;

  size_t pixelCount = pixels.size() / 4;
  for (size_t i = 0; i < pixelCount; i += quality) {
    uint8_t r = pixels[i * 4 + 0];  // RGBAの順序に修正
    uint8_t g = pixels[i * 4 + 1];
    uint8_t b = pixels[i * 4 + 2];
    uint8_t a = pixels[i * 4 + 3];

    if (a < 125 || (ignoreWhite && r > 250 && g > 250 && b > 250)) {
      continue;
    }

    uint8_t shiftedR = r >> RIGHT_SHIFT;
    uint8_t shiftedG = g >> RIGHT_SHIFT;
    uint8_t shiftedB = b >> RIGHT_SHIFT;

    rMin = std::min(rMin, shiftedR);
    rMax = std::max(rMax, shiftedR);
    gMin = std::min(gMin, shiftedG);
    gMax = std::max(gMax, shiftedG);
    bMin = std::min(bMin, shiftedB);
    bMax = std::max(bMax, shiftedB);

    int index = makeColorIndexOf(shiftedR, shiftedG, shiftedB);
    histogram[index]++;
  }

  VBox vbox(rMin, rMax, gMin, gMax, bMin, bMax, histogram);
  return HistogramAndVBox{std::move(histogram), std::move(vbox)};
}

static bool compareByCount(const MMCQ::VBox& a, const MMCQ::VBox& b) {
  return a.getCount() < b.getCount();
}

static bool compareByProduct(const MMCQ::VBox& a, const MMCQ::VBox& b) {
  int aCount = a.getCount();
  int bCount = b.getCount();
  int aVolume = a.getVolume();
  int bVolume = b.getVolume();

  if (aCount == bCount) {
    return aVolume < bVolume;
  }

  int64_t aProduct = static_cast<int64_t>(aCount) * aVolume;
  int64_t bProduct = static_cast<int64_t>(bCount) * bVolume;
  return aProduct < bProduct;
}

// applyMedianCutの完全な実装
std::vector<MMCQ::VBox> MMCQ::applyMedianCut(const std::vector<int>& histogram,
                                             const VBox& vbox) {
  if (vbox.getCount() == 0) return {vbox};
  if (vbox.getCount() == 1) return {vbox};

  std::vector<int> partialSum(VBOX_LENGTH, -1);
  std::vector<int> lookAheadSum(VBOX_LENGTH, -1);
  int total = 0;

  auto channel = vbox.widestColorChannel();
  switch (channel) {
    case VBox::ColorChannel::R:
      for (int i = vbox.rMin; i <= vbox.rMax; i++) {
        int sum = 0;
        for (int j = vbox.gMin; j <= vbox.gMax; j++) {
          for (int k = vbox.bMin; k <= vbox.bMax; k++) {
            int index = makeColorIndexOf(i, j, k);
            if (index >= 0 && index < static_cast<int>(histogram.size())) {
              sum += histogram[index];
            }
          }
        }
        total += sum;
        partialSum[i] = total;
      }
      break;
    case VBox::ColorChannel::G:
      for (int i = vbox.gMin; i <= vbox.gMax; i++) {
        int sum = 0;
        for (int j = vbox.rMin; j <= vbox.rMax; j++) {
          for (int k = vbox.bMin; k <= vbox.bMax; k++) {
            int index = makeColorIndexOf(j, i, k);
            if (index >= 0 && index < static_cast<int>(histogram.size())) {
              sum += histogram[index];
            }
          }
        }
        total += sum;
        partialSum[i] = total;
      }
      break;
    case VBox::ColorChannel::B:
      for (int i = vbox.bMin; i <= vbox.bMax; i++) {
        int sum = 0;
        for (int j = vbox.rMin; j <= vbox.rMax; j++) {
          for (int k = vbox.gMin; k <= vbox.gMax; k++) {
            int index = makeColorIndexOf(j, k, i);
            if (index >= 0 && index < static_cast<int>(histogram.size())) {
              sum += histogram[index];
            }
          }
        }
        total += sum;
        partialSum[i] = total;
      }
      break;
  }

  if (total == 0) return {vbox};

  for (int i = 0; i < VBOX_LENGTH; i++) {
    if (partialSum[i] != -1) {
      lookAheadSum[i] = total - partialSum[i];
    }
  }

  return cut(channel, vbox, partialSum, lookAheadSum, total);
}

// cut関数の実装
std::vector<MMCQ::VBox> MMCQ::cut(VBox::ColorChannel channel, const VBox& vbox,
                                  const std::vector<int>& partialSum,
                                  const std::vector<int>& lookAheadSum,
                                  int total) {
  int vboxMin, vboxMax;
  switch (channel) {
    case VBox::ColorChannel::R:
      vboxMin = vbox.rMin;
      vboxMax = vbox.rMax;
      break;
    case VBox::ColorChannel::G:
      vboxMin = vbox.gMin;
      vboxMax = vbox.gMax;
      break;
    case VBox::ColorChannel::B:
      vboxMin = vbox.bMin;
      vboxMax = vbox.bMax;
      break;
  }

  // より適切な分割点を見つける
  int bestLoc = -1;
  int bestDiff = std::numeric_limits<int>::max();
  int half = total / 2;

  for (int i = vboxMin; i <= vboxMax - 1; i++) {
    if (partialSum[i] > 0) {
      int diff = std::abs(half - partialSum[i]);
      if (diff < bestDiff) {
        bestDiff = diff;
        bestLoc = i;
      }
    }
  }

  if (bestLoc == -1) {
    return {vbox};  // 分割できない場合は元のボックスを返す
  }

  VBox vbox1(vbox);
  VBox vbox2(vbox);

  // 分割点を設定
  switch (channel) {
    case VBox::ColorChannel::R:
      vbox1.rMax = bestLoc;
      vbox2.rMin = bestLoc + 1;
      break;
    case VBox::ColorChannel::G:
      vbox1.gMax = bestLoc;
      vbox2.gMin = bestLoc + 1;
      break;
    case VBox::ColorChannel::B:
      vbox1.bMax = bestLoc;
      vbox2.bMin = bestLoc + 1;
      break;
  }

  // 分割が意味をなすか確認
  if (vbox1.getCount() > 0 && vbox2.getCount() > 0) {
    return {std::move(vbox1), std::move(vbox2)};
  }
  return {vbox};
}

void MMCQ::iterate(std::vector<VBox>& queue,
                   bool (*comparator)(const VBox&, const VBox&), int target,
                   const std::vector<int>& histogram) {
  int color = queue.size();  // 現在のキューサイズから開始

  while (color < target) {
    if (queue.empty()) {
      return;
    }

    // 最大のボックスを取得して分割を試みる
    std::sort(queue.begin(), queue.end(), comparator);
    VBox vbox = std::move(queue.back());
    queue.pop_back();

    if (vbox.getCount() == 0) {
      continue;
    }

    // メディアンカットを適用
    try {
      auto vboxes = applyMedianCut(histogram, vbox);
      queue.push_back(std::move(vboxes[0]));
      if (vboxes.size() == 2) {
        queue.push_back(std::move(vboxes[1]));
        color++;
      }
    } catch (const std::exception&) {
      // 分割に失敗した場合は元のボックスを戻す
      queue.push_back(std::move(vbox));
      break;
    }
  }

  // 最終的なソートを適用
  std::sort(queue.begin(), queue.end(), comparator);
}

// quantize関数の完全な実装
std::unique_ptr<MMCQ::ColorMap> MMCQ::quantize(
    const std::shared_ptr<margelo::nitro::ArrayBuffer>& pixelsBuffer,
    int quality, bool ignoreWhite, int maxColors) {
  if (!pixelsBuffer || !pixelsBuffer->data() || pixelsBuffer->size() == 0) {
    return nullptr;
  }

  try {
    const uint8_t* data = pixelsBuffer->data();
    size_t size = pixelsBuffer->size();

    if (size < 4 || size % 4 != 0) {
      return nullptr;
    }

    std::vector<uint8_t> pixels(data, data + size);

    // 入力値の検証
    maxColors = std::clamp(maxColors, 2, 256);
    quality = std::max(1, quality);

    auto [histogram, vbox] = makeHistogramAndVBox(pixels, quality, ignoreWhite);
    std::vector<VBox> pq{std::move(vbox)};

    // 最初のターゲット数を設定
    int initialTarget =
        static_cast<int>(std::ceil(FRACTION_BY_POPULATION * maxColors));
    iterate(pq, compareByCount, initialTarget, histogram);

    // 中間結果を保存
    std::vector<VBox> intermediateBoxes = pq;

    // 残りの色数を計算
    int remainingColors = maxColors - static_cast<int>(pq.size());

    if (remainingColors > 0) {
      // 製品メトリックでソート
      std::sort(pq.begin(), pq.end(), compareByProduct);

      // 残りの色を抽出
      iterate(pq, compareByProduct, maxColors, histogram);

      // 目標数に達しない場合、体積に基づいて分割を試みる
      while (static_cast<int>(pq.size()) < maxColors) {
        auto maxVolumeIt = std::max_element(
            pq.begin(), pq.end(), [](const VBox& a, const VBox& b) {
              return a.getVolume() < b.getVolume();
            });

        if (maxVolumeIt == pq.end()) break;

        try {
          auto newBoxes = applyMedianCut(histogram, *maxVolumeIt);
          if (newBoxes.size() > 1) {
            pq.erase(maxVolumeIt);
            pq.insert(pq.end(), newBoxes.begin(), newBoxes.end());
          } else {
            break;
          }
        } catch (const std::exception&) {
          break;
        }
      }
    }

    // 最終的なソートを適用
    std::sort(pq.begin(), pq.end(), compareByProduct);

    // 結果の確認と調整
    if (pq.empty()) {
      return nullptr;
    }

    // 必要な数の色を確保
    while (static_cast<int>(pq.size()) < maxColors) {
      // 最大のボックスを複製
      auto maxIt = std::max_element(pq.begin(), pq.end(),
                                    [](const VBox& a, const VBox& b) {
                                      return a.getCount() < b.getCount();
                                    });
      if (maxIt != pq.end()) {
        pq.push_back(*maxIt);
      } else {
        break;
      }
    }

    auto colorMap = std::make_unique<ColorMap>();
    for (auto& box : pq) {
      colorMap->push(std::move(box));
    }

    return colorMap;
  } catch (const std::exception&) {
    return nullptr;
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
  }

  int64_t aProduct = static_cast<int64_t>(aCount) * aVolume;
  int64_t bProduct = static_cast<int64_t>(bCount) * bVolume;
  return aProduct < bProduct;
}
