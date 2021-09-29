import { ContentGroup } from "../types/Document";
import { getContent } from "./Dao";

export const assetToNumber = (asset: string): number =>  {
  const afterNumberIdx = asset.indexOf(' ');
  return parseFloat(asset.substr(0, afterNumberIdx));
}

export const getAssetContent = (detailsGroup: ContentGroup, item: string): number => {
  return assetToNumber(
    getContent(detailsGroup, item)?.value[1] as string
  );
}

export const fixDecimals = (value: number, decimalPlaces: number): string => {
  return Number(Math.round(parseFloat(value + 'e' + decimalPlaces)) + 'e-' + decimalPlaces).toFixed(decimalPlaces);
}