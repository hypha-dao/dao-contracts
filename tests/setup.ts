import { loadConfig, Blockchain } from '@klevoya/hydra';
import Account from '@klevoya/hydra/lib/main/account';
import { StrictBuilder } from 'builder-pattern';
import { DaoBlockchain } from './DaoBlockchain';

const config = loadConfig("hydra.yml");

const MINUTE = 60;
const HOUR = MINUTE * 60;
const DAY = HOUR * 24;

export const setupEnvironment = async (): Promise<DaoBlockchain> => {
    const votingDurationSeconds = DAY;
    const periodDurations = 120;

    const blockchain = await DaoBlockchain.build(config, {
        votingDurationSeconds,
        voice: {
            decayPeriod: 5,
            decayPerPeriodx10M: 5000000
        }
    }, {
        createMembers: 3
    });
    
    return blockchain;
};
