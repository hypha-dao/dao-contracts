import { loadConfig, Blockchain } from '@klevoya/hydra';
import Account from '@klevoya/hydra/lib/main/account';
import { StrictBuilder } from 'builder-pattern';

const config = loadConfig("hydra.yml");

export interface Environment {
    readonly votingDurationSeconds: number;
    readonly hyphaDeferralFactor: number;
    readonly seedsDeferralFactor: number;
    readonly numPeriods: number;
    readonly periodDurations: number;
    readonly periodPause: number;
    readonly votingPause: number;

    // Contracts
    readonly dao: Account;
    readonly hvoiceToken: Account;
    readonly bank: Account;

    // Members
    // members: Array<Account>;
    readonly alice: Account;

}

export const setupEnvironment = async (): Promise<Environment> => {
    const votingDurationSeconds = 30;
    const periodDurations = 120;

    const builder = StrictBuilder<Environment>();

    const blockchain = new Blockchain(config);

    const dao = blockchain.createAccount('dao');
    dao.setContract(blockchain.contractTemplates[`dao`]);

    const hvoiceToken = blockchain.createAccount('hvoice.hypha');
    hvoiceToken.setContract(blockchain.contractTemplates['voice']);

    const bank = blockchain.createAccount('bank.hypha');
    bank.setContract(blockchain.contractTemplates['treasury']);

    await hvoiceToken.contract.create({
        issuer: dao.accountName,
        maximum_supply: '-1.00 HVOICE',
        decay_period: 5,
        decay_per_period_x10M: 5000000
    });
    
    const alice = blockchain.createAccount('alice');

    dao.updateAuth(`active`, `owner`, {
        accounts: [
            {
            permission: {
                actor: dao.accountName,
                permission: `eosio.code`
            },
            weight: 1
            }
        ]
    });
    alice.updateAuth(`child`, `active`, {
        threshold: 1,
        accounts: [
            {
            permission: {
                actor: alice.accountName,
                permission: `eosio.code`
            },
            weight: 1
            }
        ]
    });

    const eosio = blockchain.accounts.eosio;
    await eosio.contract.linkauth(
        {
          account: alice.accountName,
          requirement: "child",
          code: alice.accountName,
          type: "apply"
        },
        [{ actor: alice.accountName, permission: `active` }]
      );
    

    dao.resetTables();

    await dao.contract.createroot({
        notes: "notes"
    });

    await dao.contract.setsetting({
        key: 'voting_duration_sec',
        value: [ 'int64', votingDurationSeconds ]
    });

    await dao.contract.setsetting({
        key: 'hvoice_token_contract',
        value: [ 'name', hvoiceToken.accountName ]
    });

    await dao.contract.setsetting({
        key: 'treasury_contract',
        value: [ 'name', bank.accountName ]
    });

    console.log('Apply');
    await dao.contract.apply({
        applicant: alice.accountName,
        content: 'Apply to DAO'
    }, [ {
        actor: alice.accountName,
        permission: 'active'
    } ]);

    console.log('-----------Enrolling');
    await dao.contract.enroll({
        enroller: dao.accountName,
        applicant: alice.accountName,
        content: 'Enroll in dao'
    });

    return Promise.resolve(
        builder.votingDurationSeconds(votingDurationSeconds)
        .seedsDeferralFactor(100)
        .hyphaDeferralFactor(25)
        .numPeriods(20)
        .periodDurations(periodDurations)
        .periodPause(periodDurations + 1)
        .votingPause(votingDurationSeconds + 2)
        .dao(dao)
        .alice(alice)
        .hvoiceToken(hvoiceToken)
        .bank(bank)
        .build()
    );
};
