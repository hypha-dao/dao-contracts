export const assetToNumber = (asset: string): number =>  {
  const afterNumberIdx = asset.indexOf(' ');
  return parseFloat(asset.substr(0, afterNumberIdx));
}