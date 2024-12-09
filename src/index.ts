import { AlphaType, ColorType, Skia, loadData } from '@shopify/react-native-skia';
import { NitroPalette } from './specs';

const imgFactory = Skia.Image.MakeImageFromEncoded.bind(Skia.Image);

export const getPalette = async(
  source: string,
  colorCount: number = 5,
  quality: number = 10,
  ignoreWhite: boolean = true
): Promise<string[]> => {
  try {
    const start1 = performance.now();
    const image = await loadData(source, imgFactory);
    if (!image) {
      throw new Error('Failed to create image');
    }
    const pixels = image.readPixels(0, 0, {
      width: image.width(),
      height: image.height(),
      colorType: ColorType.RGBA_8888,
      alphaType: AlphaType.Unpremul,
    });
    if (!pixels) {
      throw new Error('Failed to read pixels');
    }
    const end1 = performance.now();
    const start2 = performance.now();
    const palette = NitroPalette.extractColors(pixels.buffer as ArrayBuffer, colorCount, quality, ignoreWhite);
    const end2 = performance.now();
    console.log(`Image load time: ${end1 - start1}ms`);
    console.log(`Palette extraction time: ${end2 - start2}ms`);
    return palette.slice(0, colorCount);
  } catch (error) {
    throw new Error(error instanceof Error ? error.message : String(error));
  }
}
