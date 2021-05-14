import { loadConfig, Blockchain } from '@klevoya/hydra';
import Account from '@klevoya/hydra/lib/main/account';
import { StrictBuilder } from 'builder-pattern';
import { DaoBlockchain } from './DaoBlockchain';

const config = loadConfig("hydra.yml");

export const setupEnvironment = async (): Promise<DaoBlockchain> => {
    const votingDurationSeconds = 30;
    const periodDurations = 120;

    const blockchain = await DaoBlockchain.build(config, {
        votingDurationSeconds,
        voice: {
            decayPeriod: 5,
            decayPerPeriodx10M: 5000000
        }
    }, {
        createMembers: 1
    });
    
    return blockchain;
};
