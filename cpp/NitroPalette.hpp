#include <vector>
#include <string>
#include <NitroModules/ArrayBuffer.hpp>
#include "HybridNitroPaletteSpec.hpp"

namespace margelo {
namespace nitro {
namespace nitropalette {
class NitroPalette : public HybridNitroPaletteSpec {
 public:
  NitroPalette()
      : HybridObject(TAG),        // Virtual base must be initialized first
        HybridNitroPaletteSpec()  // Then the direct base class
  {}

  std::vector<std::string> extractColors(
      const std::shared_ptr<ArrayBuffer>& source, double colorCount,
      double quality, bool ignoreWhite) override;

  int32_t getExternalMemorySize() const {
    // Calculate the external memory size
    return sizeof(NitroPalette);
  }
};

}  // namespace nitropalette
}  // namespace nitro
}  // namespace margelo