import { Blockchain, Contract } from '@klevoya/hydra';
import { THydraConfig } from '@klevoya/hydra/lib/config/hydra';
import Account from '@klevoya/hydra/lib/main/account';
import { TTransaction } from '@klevoya/hydra/lib/types';
import { Asset } from '../types/Asset';
import { Document, getHash } from '../types/Document';
import { Edge } from '../types/Edge';
import { Member } from '../types/Member';
import { Period } from '../types/Periods';
import { last } from '../utils/Arrays';
import { getDocumentByHash, getDocumentsByType } from '../utils/Dao';
import { toISOString, toUTC } from '../utils/Date';
import { DocumentBuilder } from '../utils/DocumentBuilder';
import { fixDecimals } from '../utils/Parsers';
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
    hyphaUSDValue: number;
}

export interface TestSettings {
    createMembers: number;
}

export interface DaoPeerContracts {
    voice: Account;
    bank: Account;
    hypha: Account;
    husd: Account;
    comments: Account;
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
            husd: this.createContract('husd.hypha', 'token'),
            comments: this.createContract('comments', 'comments')
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

      let predecessor = this.root.hash;
      const periodDate = new Date(this.setupDate);
      const numPeriods = this.settings.periodCount;

      if (numPeriods === 0) {
          return;
      }

      const data: Array<any> = [];

      for (let i = 0; i < numPeriods; ++i) {
        const label = `Period #${i + 1} of ${numPeriods}`;
        const startTime = toISOString(periodDate);

        data.push({
            predecessor,
            start_time: startTime,
            label
        });

        // This part depends a lot on the Period content. This is made like this to optimize the setup
        const period: Document = DocumentBuilder
        .builder()
        .contentGroup(builder => 
            builder
            .groupLabel('details')
            .timePoint('start_time', startTime)
            .string('label', label)
        )
        .contentGroup(builder => 
            builder
            .groupLabel('system')
            .name('type', 'period')
            .string('node_label', label)
        )
        .build();
        predecessor = getHash(period);
        periodDate.setSeconds(periodDate.getSeconds() + this.settings.periodDurationSeconds);
      }

      await this.sendTransaction({
          actions: data.map(d => this.buildAction(this.dao, 'addperiod', d))
      });

      this.periods.push(
          ... getDocumentsByType(this.getDaoDocuments(), 'period').map(p => new Period(p))
      );
    }

    public buildAction(
        account: Account, 
        action: string, 
        data: TTransaction['actions'][number]['data'],
        permissions?: TTransaction['actions'][number]['authorization']
    ): TTransaction['actions'][number] {
        return {
            account: account.accountName,
            name: action,
            authorization: permissions ?? getAccountPermission(account),
            data
        };
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

        await this.sendTransaction({
            actions: [
                this.buildAction(this.dao, 'apply', {
                    applicant: account.accountName,
                    content: 'Apply to DAO'
                }, getAccountPermission(account)),
                this.buildAction(this.dao, 'enroll', {
                    enroller: this.dao.accountName,
                    applicant: account.accountName,
                    content: 'Enroll in dao'
                })
            ]
        });

        const doc = last(getDocumentsByType(this.getDaoDocuments(), 'member'));

        return new Member(
            account,
            doc
        );
    }

    async increaseVoice(accountName: string, quantity: string) {
        await this.sendTransaction({
            actions: [
                this.buildAction(this.peerContracts.voice, 'issue', {
                    to: this.dao.accountName,
                    quantity,
                    memo: 'Increasing voice'
                }, getAccountPermission(this.dao)),
                this.buildAction(this.peerContracts.voice, 'transfer', {
                    from: this.dao.accountName,
                    to: accountName,
                    quantity,
                    memo: 'Increasing voice'
                }, getAccountPermission(this.dao))
            ]
        });
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

        // Initialize this all in a batch to speed it up
        await this.sendTransaction({
            actions: [
                // Initialize HVOICE
                this.buildAction(
                    this.peerContracts.voice,
                    'create',
                    {
                        issuer: this.dao.accountName,
                        maximum_supply: '-1.00 HVOICE',
                        decay_period: this.settings.voice.decayPeriod,
                        decay_per_period_x10M: this.settings.voice.decayPerPeriodx10M
                    }
                ),
                // Initialize HUSD
                this.buildAction(
                    this.peerContracts.husd,
                    'create',
                    {
                        issuer: this.peerContracts.bank.accountName,
                        maximum_supply: '1000000000.00 HUSD'
                    }
                ),
                // Initialize HYPHA
                this.buildAction(
                    this.peerContracts.hypha,
                    'create',
                    {
                        issuer: this.dao.accountName,
                        maximum_supply: '1000000000.00 HYPHA'
                    }
                ),
                // Create root
                this.buildAction(this.dao, 'createroot', { notes: 'notes' }),
                // Settings
                this.buildAction(this.dao, 'setsetting', {
                    key: 'voting_duration_sec',
                    value: [ 'int64', this.settings.votingDurationSeconds ]
                }),
                this.buildAction(this.dao, 'setsetting', {
                    key: 'hvoice_token_contract',
                    value: [ 'name', this.peerContracts.voice.accountName ]
                }),
                this.buildAction(this.dao, 'setsetting', {
                    key: 'hypha_token_contract',
                    value: [ 'name', this.peerContracts.hypha.accountName ]
                }),
                this.buildAction(this.dao, 'setsetting', {
                    key: 'husd_token_contract',
                    value: [ 'name', this.peerContracts.husd.accountName ]
                }),
                this.buildAction(this.dao, 'setsetting', {
                    key: 'treasury_contract',
                    value: [ 'name', this.peerContracts.bank.accountName ]
                }),
                this.buildAction(this.dao, 'setsetting', {
                    key: 'hypha_deferral_factor_x100',
                    value: [ 'int64', this.settings.hyphaDeferralFactor ]
                }),
                this.buildAction(this.dao, 'setsetting', {
                    key: 'hypha_usd_value',
                    value: [ 'asset', `${fixDecimals(this.settings.hyphaUSDValue, 4)} HYPHA` ]
                })
            ]
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
