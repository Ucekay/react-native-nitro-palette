import { type HybridObject } from 'react-native-nitro-modules'

export interface NitroPalette
  extends HybridObject<{ ios: 'c++'; android: 'c++' }> {
  extractColors(
    source: ArrayBuffer,
    width: number,
    height: number,
    colorCount: number,
    quality: number
  ): string[]
}
