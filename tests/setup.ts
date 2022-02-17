import { loadConfig } from '@klevoya/hydra';
import { DaoBlockchain, TestSettings } from './dao/DaoBlockchain';
import { DaoSettings } from './dao/Dao';
import {Asset} from "./types/Asset";
import {DeepPartial} from "ts-essentials";

const config = loadConfig("hydra.yml");

const MINUTE = 60;
const HOUR = MINUTE * 60;
const DAY = HOUR * 24;

export const setupEnvironment = async (
    settings?: Record<string, DeepPartial<DaoSettings>>,
    testSettings?: Record<string, DeepPartial<TestSettings>>
): Promise<DaoBlockchain> => {
    const votingDurationSeconds = DAY;
    const periodDurationSeconds = DAY;
    const periodCount = 9;
    const onboarderAccount = 'hypha';
    const quorumFactorX100 = 60;
    const alignmentFactorX100 = 60;

    const fullSettings: Record<string, DaoSettings> = {};

    if (!settings) {
        settings = { test: { } }
    }

    for (const key of Object.keys(settings)) {
        const partial = settings ? settings[key] : {};
        fullSettings[key] = {
            title: partial?.title ?? key,
            description: partial?.description ?? 'No description',
            hyphaDeferralFactor: partial?.hyphaDeferralFactor ?? 2,
            periodCount: partial?.periodCount ?? periodCount,
            periodDurationSeconds: partial?.periodDurationSeconds ?? periodDurationSeconds,
            onboarderAccount: partial?.onboarderAccount ?? onboarderAccount,
            votingDurationSeconds: partial?.votingDurationSeconds ?? votingDurationSeconds,
            voting: {
                quorumFactorX100: partial?.voting?.quorumFactorX100 ?? quorumFactorX100,
                alignmentFactorX100: partial?.voting?.alignmentFactorX100 ?? alignmentFactorX100
            },
            tokens: {
                peg: {
                    name: partial?.tokens?.peg?.name ?? '1.00 PEGTO'
                },
                voice: {
                    asset: Asset.fromPartialAsset(partial?.tokens?.voice?.asset, 100, 'HVOICE', 2),
                    decayPerPeriodx10M: partial?.tokens?.voice?.decayPerPeriodx10M ?? 5000000,
                    decayPeriod: partial?.tokens?.voice?.decayPeriod ?? DAY
                },
                reward: {
                    name: partial?.tokens?.reward?.name ?? '1.00 REWARD',
                    toPegRatio: Asset.fromPartialAsset(partial?.tokens?.reward?.toPegRatio, 100, 'REWARD', 2)
                }
            }

        }
    }

    const fullTestSettings: Record<string, TestSettings> = {};

    // taking the keys from the settings, to ensure we hav full test settings for the same dhos
    for (const key of Object.keys(settings)) {
        const partial = testSettings ? testSettings[key] : {};
        fullTestSettings[key] = {
            createMembers: partial?.createMembers ?? 5
        };
    }

    return await DaoBlockchain.build(config, fullSettings, fullTestSettings);
};
