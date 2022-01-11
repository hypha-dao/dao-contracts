import {Asset} from "./Asset";

describe('Asset', () => {
    it('Tests asset', () => {

        const asset = Asset.fromString('1.00 HVOICE');
        expect(asset.precision).toEqual(2);
        expect(asset.amount).toEqual(100);
        expect(asset.symbol).toEqual('HVOICE');
        expect(asset.toString()).toEqual('1.00 HVOICE');
        expect(asset.toFloat()).toEqual(1.00);

        const asset2 = Asset.fromString('12.456 STUFF');
        expect(asset2.precision).toEqual(3);
        expect(asset2.amount).toEqual(12456);
        expect(asset2.symbol).toEqual('STUFF');
        expect(asset2.toString()).toEqual('12.456 STUFF');
        expect(asset2.toFloat()).toEqual(12.456);
    });
});
