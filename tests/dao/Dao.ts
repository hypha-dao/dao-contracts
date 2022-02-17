import {Member} from "../types/Member";
import {Period} from "../types/Periods";
import {DeepReadonly} from "ts-essentials";
import {Document} from "../types/Document";
import {Asset} from "../types/Asset";

export class Dao {
    readonly name: string;
    readonly settings: DeepReadonly<DaoSettings>;
    readonly members: Array<Member>;
    readonly periods: Array<Period>;
    private hash: string;
    private root: Document;

    public constructor(name: string, settings: DaoSettings) {
        this.name = name;
        this.settings = settings;
        this.members = [];
        this.periods = [];
    }

    public setHash(hash: string) {
        if (this.hash !== undefined) {
            throw new Error('Hash already set');
        }

        this.hash = hash;
    }

    public getHash(): string {
        return this.hash;
    }

    public setRoot(root: Document) {
        if (this.root !== undefined) {
            throw new Error('Root already set');
        }

        this.root = root;
    }

    public getRoot(): Document {
        return this.root;
    }
}

export interface DaoSettings {
    title: string;
    description: string;
    votingDurationSeconds: number;
    periodDurationSeconds: number;
    periodCount: number;
    hyphaDeferralFactor: number;
    // hyphaUSDValue: number;
    onboarderAccount: string;
    voting: {
        quorumFactorX100: number;
        alignmentFactorX100: number;
    }
    tokens: {
        peg: {
            name: string;
        },
        voice: {
            asset: Asset;
            decayPeriod: number,
            decayPerPeriodx10M: number
        },
        reward: {
            name: string;
            toPegRatio: Asset;
        }
    }
}
