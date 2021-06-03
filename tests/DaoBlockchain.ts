import { loadConfig, Blockchain, Contract } from '@klevoya/hydra';
import { THydraConfig } from '@klevoya/hydra/lib/config/hydra';
import Account from '@klevoya/hydra/lib/main/account';
import { Document } from './types/Document';

export interface DaoSettings {
    votingDurationSeconds: number;
    voice: {
        decayPeriod: number,
        decayPerPeriodx10M: number
    }
}

export interface TestSettings {
    createMembers: number;
}

export interface DaoPeerContracts {
    voice: Account;
    bank: Account;
}

export class DaoBlockchain extends Blockchain {

    readonly config: THydraConfig;
    readonly dao: Account;
    readonly settings: DaoSettings;
    readonly peerContracts: DaoPeerContracts;
    readonly members: Array<Account>;

    private constructor(config: THydraConfig, settings: DaoSettings) {
        super(config);
        this.config = config;
        this.dao = this.createContract('dao', 'dao');
        this.settings = settings;
        this.peerContracts = {
            voice: this.createContract('hvoice.hypha', 'voice'),
            bank:  this.createContract('bank.hypha', 'treasury')
        };
        this.members = [];
    }

    static async build(config: THydraConfig, settings: DaoSettings, testSettings?: TestSettings) {
        const blockchain = new DaoBlockchain(config, settings);
        await blockchain.setup();

        if (testSettings) {
            if (testSettings.createMembers) {
                if (testSettings.createMembers > 5) {
                    throw new Error('Can only create 5 accounts at most (because of the name limitant of only using digits 1 to 5)');
                }

                for (let index = 0; index < testSettings.createMembers; ++index) {
                    const member: Account = await blockchain.createMember(`mem${index + 1}.hypha`);
                    blockchain.members.push(member);
                }
            }
        }

        return blockchain;
    }

    static getAccountPermission(account: Account): import('eosjs/dist/eosjs-serialize').Authorization[] {
        return [
            {
                actor: account.accountName,
                permission: 'active'
            } 
        ];
    }

    createContract(accountName: string, templateName: string): Account {
        const account = this.createAccount(accountName);
        account.setContract(this.contractTemplates[templateName]);
        return account;
    }

    async createMember(accountName: string) {
        const account = this.createAccount(accountName);

        account.updateAuth(`child`, `active`, {
            threshold: 1,
            accounts: [
                {
                permission: {
                    actor: account.accountName,
                    permission: `eosio.code`
                },
                weight: 1
                }
            ]
        });

        await this.dao.contract.apply({
            applicant: account.accountName,
            content: 'Apply to DAO'
        }, DaoBlockchain.getAccountPermission(account));
    
        await this.dao.contract.enroll({
            enroller: this.dao.accountName,
            applicant: account.accountName,
            content: 'Enroll in dao'
        });

        return account;
    }

    async setup() {
        this.dao.resetTables();
        
        this.dao.updateAuth(`active`, `owner`, {
            accounts: [
                {
                permission: {
                    actor: this.dao.accountName,
                    permission: `eosio.code`
                },
                weight: 1
                }
            ]
        });

        // Initialize voice contract
        await this.peerContracts.voice.contract.create({
            issuer: this.dao.accountName,
            maximum_supply: '-1.00 HVOICE',
            decay_period: this.settings.voice.decayPeriod,
            decay_per_period_x10M: this.settings.voice.decayPerPeriodx10M
        });
        

        await this.dao.contract.createroot({
            notes: "notes"
        });

        await this.dao.contract.setsetting({
            key: 'voting_duration_sec',
            value: [ 'int64', this.settings.votingDurationSeconds ]
        });
    
        await this.dao.contract.setsetting({
            key: 'hvoice_token_contract',
            value: [ 'name', this.peerContracts.voice.accountName ]
        });
    
        await this.dao.contract.setsetting({
            key: 'treasury_contract',
            value: [ 'name', this.peerContracts.bank.accountName ]
        });
    }

    public getDaoDocuments(): Array<Document> {
        return this.dao.getTableRowsScoped('documents')['dao'];
    }

}
