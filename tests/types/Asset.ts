export class Asset {

    readonly amount: number;
    readonly symbol: string;
    readonly precision: number;

    public constructor(amount: number, symbol: string, precision: number) {
        this.amount = amount;
        this.symbol = symbol;
        this.precision = precision;
    }

    public static fromPartialAsset(partial: Partial<Asset> | undefined, amount: number, symbol: string, precision: number) {
        return new Asset(
            partial?.amount ?? amount,
            partial?.symbol ?? symbol,
            partial?.precision ?? precision
        );
    }

    public static fromString(assetAsString: string) {
        const pieces = assetAsString.split(' ');
        if (pieces.length !== 2) {
            throw new Error(`Invalid asset ${assetAsString}`);
        }

        const amountPart = pieces[0].split('.');
        if (amountPart.length !== 2) {
            throw new Error(`Invalid asset ${assetAsString}`);
        }

        const amount = parseInt(amountPart[0] + amountPart[1]);
        const precision = amountPart[1].length;
        const symbol = pieces[1];

        return new Asset(amount, symbol, precision);
    }

    public toFloat() {
        return this.amount / Math.pow(10, this.precision);
    }

    public toString() {
        const amountStr = this.amount.toString();

        let numericAsset = amountStr.slice(0, amountStr.length - this.precision);
        if (this.precision > 0) {
            numericAsset += '.' + amountStr.slice(amountStr.length - this.precision);
        }

        return `${numericAsset} ${this.symbol}`;
    }
}
