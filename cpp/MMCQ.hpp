#ifndef MMCQ_HPP
#define MMCQ_HPP

#include <vector>
#include <cstdint>
#include <memory>
#include <NitroModules/ArrayBuffer.hpp>

class MMCQ {
 public:
  struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    std::string toString() const;
    Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0);
  };

  class VBox {
   public:
    uint8_t rMin, rMax;
    uint8_t gMin, gMax;
    uint8_t bMin, bMax;

    VBox()
        : rMin(0),
          rMax(0),
          gMin(0),
          gMax(0),
          bMin(0),
          bMax(0) {}  // デフォルトコンストラクタを追加
    VBox(uint8_t rMin, uint8_t rMax, uint8_t gMin, uint8_t gMax, uint8_t bMin,
         uint8_t bMax, const std::vector<int>& histogram);
    VBox(const VBox& other);
    VBox(VBox&& other) noexcept;
    VBox& operator=(const VBox& other);
    VBox& operator=(VBox&& other) noexcept;

    int getVolume(bool forceRecalculate = false) const;
    int getCount(bool forceRecalculate = false) const;
    Color getAverage(bool forceRecalculate = false) const;

    enum class ColorChannel { R, G, B };
    ColorChannel widestColorChannel() const;

   private:
    std::vector<int> histogram;  // 参照からコピーに変更
    mutable std::unique_ptr<Color> average;
    mutable int volume{0};
    mutable int count{-1};
  };

  class ColorMap {
   public:
    ColorMap() = default;
    ColorMap(const ColorMap&) = delete;             // コピー禁止
    ColorMap& operator=(const ColorMap&) = delete;  // コピー禁止
    ColorMap(ColorMap&&) noexcept = default;  // ムーブコンストラクタを追加
    ColorMap& operator=(ColorMap&&) noexcept = default;  // ムーブ代入を追加

    void push(VBox&& vbox);  // ムーブセマンティクスのみを使用
    std::vector<Color> makePalette() const;
    Color makeNearestColor(const Color& color) const;

   private:
    std::vector<VBox> vboxes;
  };

  static std::unique_ptr<ColorMap> quantize(
      const std::shared_ptr<margelo::nitro::ArrayBuffer>& pixelsBuffer,
      int quality, bool ignoreWhite, int maxColors);

 private:
  static const int SIGNAL_BITS = 5;
  static const int RIGHT_SHIFT = 8 - SIGNAL_BITS;
  static const int MULTIPLIER = 1 << RIGHT_SHIFT;
  static const int HISTOGRAM_SIZE = 1 << (3 * SIGNAL_BITS);
  static const int VBOX_LENGTH = 1 << SIGNAL_BITS;
  static const double FRACTION_BY_POPULATION;
  static const int MAX_ITERATIONS = 1000;

  static int makeColorIndexOf(int red, int green, int blue);

  // 新しい構造体を追加
  struct HistogramAndVBox {
    std::vector<int> histogram;
    VBox vbox;
  };

  // メソッドの宣言を変更
  static HistogramAndVBox makeHistogramAndVBox(
      const std::vector<uint8_t>& pixels, int quality, bool ignoreWhite);

  static std::vector<VBox> applyMedianCut(const std::vector<int>& histogram,
                                          const VBox& vbox);
  static std::vector<VBox> cut(VBox::ColorChannel channel, const VBox& vbox,
                               const std::vector<int>& partialSum,
                               const std::vector<int>& lookAheadSum, int total);

  static void iterate(std::vector<VBox>& queue,
                      bool (*comparator)(const VBox&, const VBox&), int target,
                      const std::vector<int>& histogram);

  static bool compareByCount(const VBox& a, const VBox& b);
  static bool compareByProduct(const VBox& a, const VBox& b);
};

#endif
