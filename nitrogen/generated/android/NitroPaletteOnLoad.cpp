///
/// NitroPaletteOnLoad.cpp
/// This file was generated by nitrogen. DO NOT MODIFY THIS FILE.
/// https://github.com/mrousavy/nitro
/// Copyright © 2024 Marc Rousavy @ Margelo
///

#include "NitroPaletteOnLoad.hpp"

#include <jni.h>
#include <fbjni/fbjni.h>
#include <NitroModules/HybridObjectRegistry.hpp>

#include "NitroPalette.hpp"

namespace margelo::nitro::nitropalette {

int initialize(JavaVM* vm) {
  using namespace margelo::nitro;
  using namespace margelo::nitro::nitropalette;
  using namespace facebook;

  return facebook::jni::initialize(vm, [] {
    // Register native JNI methods
    

    // Register Nitro Hybrid Objects
    HybridObjectRegistry::registerHybridObjectConstructor(
      "NitroPalette",
      []() -> std::shared_ptr<HybridObject> {
        static_assert(std::is_default_constructible_v<NitroPalette>,
                      "The HybridObject \"NitroPalette\" is not default-constructible! "
                      "Create a public constructor that takes zero arguments to be able to autolink this HybridObject.");
        return std::make_shared<NitroPalette>();
      }
    );
  });
}

} // namespace margelo::nitro::nitropalette