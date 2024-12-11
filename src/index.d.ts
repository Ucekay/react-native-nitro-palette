declare module 'react-native-nitro-palette' {
  /**
   * Extracts a color palette from an image.
   * @param source - The image source URI
   * @param colorCount - The number of colors to extract (default: 5)
   * @param quality - The quality of the color extraction (1-10, default: 10)
   * @param ignoreWhite - Whether to ignore white colors (default: true)
   * @returns Promise resolving to an array of rgb color strings
   */
  export function getPalette(
    source: string,
    colorCount?: number,
    quality?: number,
    ignoreWhite?: boolean
  ): Promise<string[]>;
}
