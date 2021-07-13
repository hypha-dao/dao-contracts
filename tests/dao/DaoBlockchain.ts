import { Blockchain } from '@klevoya/hydra';
import { THydraConfig } from '@klevoya/hydra/lib/config/hydra';
import Account from '@klevoya/hydra/lib/main/account';
import { Asset } from '../types/Asset';
import { Document } from '../types/Document';
import { Edge } from '../types/Edge';
import { Member } from '../types/Member';
import { Period } from '../types/Periods';
import { last } from '../utils/Arrays';
import { getDocumentByHash, getDocumentsByType } from '../utils/Dao';
import { getAccountPermission } from '../utils/Permissions';

export interface DaoSettings {
    votingDurationSeconds: number;
    voice: {
        decayPeriod: number,
        decayPerPeriodx10M: number
    }
    periodDurationSeconds: number;
    periodCount: number;
    hyphaDeferralFactor: number;
}

export interface TestSettings {
    createMembers: number;
}

export interface DaoPeerContracts {
    voice: Account;
    bank: Account;
    hypha: Account;
    husd: Account;
}

const HVOICE_SYMBOL = 'HVOICE';

export class DaoBlockchain extends Blockchain {

    readonly config: THydraConfig;
    readonly dao: Account;
    readonly settings: DaoSettings;
    readonly peerContracts: DaoPeerContracts;
    readonly members: Array<Member>;
    readonly periods: Array<Period>;

    // There should be a better way to get this - But currently seems stable
    readonly rootHash = 'D9B40C418C850A65B71CA55ABB0DE9B0E4646A02B95B230E3917E877610BFAE5';
    private root: Document;
    private setupDate: Date;
    public currentDate: Date;

    private constructor(config: THydraConfig, settings: DaoSettings) {
        super(config);
        this.config = config;
        this.dao = this.createContract('dao', 'dao');
        this.settings = settings;
        this.peerContracts = {
            voice: this.createContract('hvoice.hypha', 'voice'),
            bank:  this.createContract('bank.hypha', 'treasury'),
            hypha: this.createContract('token.hypha', 'token'),
            husd: this.createContract('husd.hypha', 'token')
        };
        this.members = [];
        this.periods = [];
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
                    const member: Member = await blockchain.createMember(`mem${index + 1}.hypha`);
                    blockchain.members.push(member);
                }

                // Member 0 is always awarded 99 HVOICE (for a total of 100)
                if (testSettings.createMembers > 0) {
                    await blockchain.increaseVoice(blockchain.members[0].account.accountName, '99.00 HVOICE');
                }           
            }
        }

        return blockchain;
    }

    createContract(accountName: string, templateName: string): Account {
        const account = this.createAccount(accountName);
        account.setContract(this.contractTemplates[templateName]);
        return account;
    }

    private async createPeriods() {

      if (this.root === undefined) {
        throw new Error('Root document has to be created before creating periods');
      }

      let predecesor = this.root.hash;

      let periodDate = new Date(this.setupDate);

      let numPeriods = this.settings.periodCount;

      for (let i = 0; i < numPeriods; ++i) {

        let label = `Period #${i + 1} of ${numPeriods}`;
        
        await this.dao.contract.addperiod({
          predecessor: predecesor,
          start_time: periodDate.toISOString().replace('Z', ''),
          label,
        });

        let periodDoc = last(getDocumentsByType(this.getDaoDocuments(), 'period'));
        
        this.periods.push(new Period(new Date(periodDate), label, periodDoc));

        periodDate.setSeconds(periodDate.getSeconds() + this.settings.periodDurationSeconds);

        predecesor = periodDoc.hash;
      }
    }

    async createMember(accountName: string): Promise<Member> {
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
        }, getAccountPermission(account));
    
        await this.dao.contract.enroll({
            enroller: this.dao.accountName,
            applicant: account.accountName,
            content: 'Enroll in dao'
        });

        const doc = last(getDocumentsByType(this.getDaoDocuments(), 'member'));

        return new Member(
            account,
            doc
        );
    }

    async increaseVoice(accountName: string, quantity: string) {
        await this.peerContracts.voice.contract.issue({
            to: this.dao.accountName,
            quantity,
            memo: 'Increasing voice'
        }, getAccountPermission(this.dao));

        await this.peerContracts.voice.contract.transfer({
            from: this.dao.accountName,
            to: accountName,
            quantity,
            memo: 'Increasing voice'
        }, getAccountPermission(this.dao));
    }

    async setup() {
        this.dao.resetTables();
        this.setupDate = new Date();
        this.currentDate = new Date(this.setupDate);
        this.setCurrentTime(this.setupDate);
        
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

        this.peerContracts.bank.updateAuth(`active`, `owner`, {
          accounts: [
              {
              permission: {
                  actor: this.peerContracts.bank.accountName,
                  permission: `eosio.code`
              },
              weight: 1
              },
              {
                permission: {
                    actor: this.dao.accountName,
                    permission: `active`
                },
                weight: 1
              }              
          ]
        });

        this.peerContracts.husd.updateAuth(`active`, `owner`, {
          accounts: [
              {
              permission: {
                  actor: this.peerContracts.husd.accountName,
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
        
        // Initialize voice contract
        await this.peerContracts.husd.contract.create({
            issuer: this.peerContracts.bank.accountName,
            maximum_supply: '1000000000.00 HUSD'
        });
        
        // Initialize voice contract
        await this.peerContracts.hypha.contract.create({
            issuer: this.dao.accountName,
            maximum_supply: '1000000000.00 HYPHA'
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
            key: 'hypha_token_contract',
            value: [ 'name', this.peerContracts.hypha.accountName ]
        });

        await this.dao.contract.setsetting({
            key: 'husd_token_contract',
            value: [ 'name', this.peerContracts.husd.accountName ]
        });
    
        await this.dao.contract.setsetting({
            key: 'treasury_contract',
            value: [ 'name', this.peerContracts.bank.accountName ]
        });

        await this.dao.contract.setsetting({
            key: 'hypha_deferral_factor_x100',
            value: [ 'int64', this.settings.hyphaDeferralFactor ]
        });

        this.root = getDocumentByHash(this.getDaoDocuments(), this.rootHash);

        await this.createPeriods();
    }

    public getDaoDocuments(): Array<Document> {
        return this.dao.getTableRowsScoped('documents')['dao'];
    }

    public getDaoEdges(): Array<Edge> {
        return this.dao.getTableRowsScoped('edges')['dao'];
    }

    public getIssuedHvoice(): Asset {
        return Asset.fromString(
            this.peerContracts.voice.getTableRowsScoped('stat')[HVOICE_SYMBOL][0].supply
        );
    }

    public getHvoiceForMember(member: string): Asset {
        return Asset.fromString(
            this.peerContracts.voice.getTableRowsScoped('accounts')[member][0].balance
        );
    }

    public getHusdForMember(member: string): Asset {
      return Asset.fromString(
        this.peerContracts.husd.getTableRowsScoped('accounts')[member][0].balance
      );
    }

    public getRoot(): Document {
        return this.root;
    }

    public getSetupDate(): Date {
        return this.setupDate;
    }
    
}
