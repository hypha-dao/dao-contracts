import { loadConfig, Blockchain } from '@klevoya/hydra';
import Account from '@klevoya/hydra/lib/main/account';
import { StrictBuilder } from 'builder-pattern';
import { DaoBlockchain, DaoSettings, TestSettings } from './dao/DaoBlockchain';

const config = loadConfig("hydra.yml");

const MINUTE = 60;
const HOUR = MINUTE * 60;
const DAY = HOUR * 24;

export const setupEnvironment = async (settings?: Partial<DaoSettings>, testSettings?: Partial<TestSettings>): Promise<DaoBlockchain> => {
    const votingDurationSeconds = DAY;
    const periodDurationSeconds = DAY;
    const periodCount = 9;

    const blockchain = await DaoBlockchain.build(config, {
        votingDurationSeconds: settings?.votingDurationSeconds ?? votingDurationSeconds,

        voice: {
            decayPeriod: settings?.voice?.decayPeriod ?? DAY,
            decayPerPeriodx10M: settings?.voice?.decayPerPeriodx10M ?? 5000000 // 50%
        },
        periodCount: settings?.periodCount ?? periodCount,
        periodDurationSeconds: settings?.periodDurationSeconds ?? periodDurationSeconds,
        hyphaDeferralFactor: settings?.hyphaDeferralFactor ?? 3,
        hyphaUSDValue: settings?.hyphaUSDValue ?? 8
    }, {
        createMembers: testSettings?.createMembers ?? 5
    });
    
    return blockchain;
};
