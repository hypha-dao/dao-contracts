import {Blockchain} from '@klevoya/hydra';
import {THydraConfig} from '@klevoya/hydra/lib/config/hydra';
import Account from '@klevoya/hydra/lib/main/account';
import {TTransaction} from '@klevoya/hydra/lib/types';
import {Asset} from '../types/Asset';
import {ContentType, Document } from '../types/Document';
import {Edge} from '../types/Edge';
import {Member} from '../types/Member';
import {last} from '../utils/Arrays';
import {
    getContent,
    getDetailsGroup,
    getDocumentById,
    getDocumentsByType,
} from '../utils/Dao';
import {ContentGroupBuilder} from '../utils/DocumentBuilder';
import {getAccountPermission} from '../utils/Permissions';
import {Dao, DaoSettings} from "./Dao";
import {Period} from "../types/Periods";
import { performance } from 'perf_hooks';

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

expect.extend({
    _toBeAbsent(received, account, action) {
        const message = `Did not expect the following params in call ${account.accountName}.${action}`;
        if (this.equals(received, [])) {
            return {
                message: () => message + this.utils.printExpected(received),
                pass: true
            };
        } else {
            return {
                message: () => message + this.utils.printExpected(received),
                pass: false
            }
        }
    },
    _toBePresent(received, account, action) {
        const explain = () => {
            const includeNullRequired = received.some(r => r.nullRequired);
            return `Expected params in call ${account.accountName}.${action} but were missing.`
            + (includeNullRequired ? ' Some parameters are optional but are required to be in the data as null.' : '')
            + this.utils.printExpected(received);
        };

        if (this.equals(received, [])) {
            return {
                message: () => explain(),
                pass: true
            };
        } else {
            return {
                message: () => explain(),
                pass: false
            }
        }
    }
});

export class DaoBlockchain extends Blockchain {

    readonly config: THydraConfig;
    readonly daoContract: Account;
    readonly daos: Array<Dao>;
    readonly peerContracts: DaoPeerContracts;

    // There should be a better way to get this - But currently seems stable
    private root: Document;
    private setupDate: Date;
    public currentDate: Date;

    private constructor(config: THydraConfig, settings: Record<string, DaoSettings>) {
        super(config);
        this.config = config;
        this.daoContract = this.createContract('dao', 'dao');
        this.daos = Object.keys(settings).map(key => new Dao(key, settings[key]));
        this.peerContracts = {
            voice: this.createContract('hvoice.hypha', 'voice'),
            bank:  this.createContract('bank.hypha', 'treasury'),
            hypha: this.createContract('token.hypha', 'token'),
            husd: this.createContract('husd.hypha', 'token'),
            comments: this.createContract('comments', 'comments')
        };
    }

    public getDao(name: string): Dao {
        const dao = this.daos.find(d => d.name === name);
        if (!dao) {
            throw new Error(`Dao with name ${name} not found`);
        }

        return dao;
    }

    static async build(config: THydraConfig, settings: Record<string, DaoSettings>, testSettings?: Record<string, TestSettings>) {
        const startTime = performance.now();

        const blockchain = new DaoBlockchain(config, settings);
        await blockchain.setup();

        if (testSettings) {
            for (const name of Object.keys(testSettings)) {
                const dao = blockchain.getDao(name);
                const numberOfMembers = testSettings[name].createMembers;
                if (numberOfMembers) {

                    const actions: TTransaction['actions'] = [];

                    if (numberOfMembers > 5) {
                        throw new Error('Can only create 5 accounts at most (because of the name limitation of only using digits 1 to 5)');
                    }

                    for (let index = 0; index < numberOfMembers; ++index) {
                        const member = `mem${index + 1}.hypha`;
                        actions.push(
                            ...blockchain.createMemberActions(name, member)
                        );
                        if (index === 0) {
                            // Member 0 is always awarded 99 voice (for a total of 100)
                            actions.push(
                                ...blockchain.increaseVoiceActions(name, member, '99.00 ' + dao.settings.tokens.voice.asset.symbol)
                            );
                        }
                    }

                    await blockchain.sendTransaction({
                        actions
                    });

                    const docMembers = getDocumentsByType(
                        blockchain.getDaoDocuments(),
                        'member'
                    ).slice(-numberOfMembers);

                    for (const docMember of docMembers) {
                        const memberName = getContent(getDetailsGroup(docMember), 'member').value[1].toString();
                        dao.members.push(new Member(
                            blockchain.accounts[
                                memberName
                            ],
                            docMember
                        ));
                    }
                }
            }
        }

        const endTime = performance.now();
        if (process.env.PRINT_SETUP_TIME) {
            console.log(`It took ${Math.round((endTime - startTime)/1000.0)}s to run the setup`);
        }

        return blockchain;
    }

    createContract(accountName: string, templateName: string): Account {
        const contractAccount = new Proxy(this.createAccount(accountName), {
            get: (account: Account, p: string | symbol) => {
                // We need to do this everytime, because this gets constantly override at some other point
                if (p === 'contract') {
                    return new Proxy(account[p], {
                        get: (target: any, action: string | symbol) => {
                            return new Proxy(target[String(action)], {
                                apply: (target: any, thisArg: any, argArray: any[]) => {
                                    this.validateParams(account, String(action), argArray[0]);
                                    return target(...argArray);
                                }
                            })
                        }
                    });
                }

                return account[p];
            }
        });
        contractAccount.setContract(this.contractTemplates[templateName]);
        return contractAccount;
    }

    private async createPeriods(dao: Dao) {

      if (this.root === undefined) {
        throw new Error('Root document has to be created before creating periods');
      }

      await this.sendTransaction({
          actions: [
              this.buildAction(this.daoContract, 'genperiods', {
                  dao_id: dao.getId(),
                  period_count: dao.settings.periodCount
              }, getAccountPermission(dao.settings.onboarderAccount))
          ]
      });

      dao.periods.push(
          ... getDocumentsByType(this.getDaoDocuments(), 'period').map(p => new Period(p))
      );
    }

    public buildAction(
        account: Account,
        action: string,
        data: TTransaction['actions'][number]['data'],
        permissions?: TTransaction['actions'][number]['authorization']
    ): TTransaction['actions'][number] {
        this.validateParams(account, action, data);
        return {
            account: account.accountName,
            name: action,
            authorization: permissions ?? getAccountPermission(account),
            data
        };
    }

    private createMemberActions(daoName: string, accountName: string): TTransaction['actions'] {
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

        const dao = this.getDao(daoName);
        return [
            this.buildAction(this.daoContract, 'apply', {
                applicant: account.accountName,
                dao_id: dao.getId(),
                content: 'Apply to DAO'
            }, getAccountPermission(account)),
            this.buildAction(this.daoContract, 'enroll', {
                enroller: dao.settings.onboarderAccount,
                dao_id: dao.getId(),
                applicant: account.accountName,
                content: 'Enroll in dao'
            }, getAccountPermission(dao.settings.onboarderAccount))
        ];
    }

    async createMember(daoName: string, accountName: string): Promise<Member> {
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

        const dao = this.getDao(daoName);
        await this.sendTransaction({
            actions: [
                this.buildAction(this.daoContract, 'apply', {
                    applicant: account.accountName,
                    dao_id: dao.getId(),
                    content: 'Apply to DAO'
                }, getAccountPermission(account)),
                this.buildAction(this.daoContract, 'enroll', {
                    enroller: dao.settings.onboarderAccount,
                    dao_id: dao.getId(),
                    applicant: account.accountName,
                    content: 'Enroll in dao'
                }, getAccountPermission(dao.settings.onboarderAccount))
            ]
        });

        const doc = last(getDocumentsByType(this.getDaoDocuments(), 'member'));

        return new Member(
            account,
            doc
        );
    }

    increaseVoiceActions(daoName: string, accountName: string, quantity: string): TTransaction['actions'] {
        return [
            this.buildAction(this.peerContracts.voice, 'issue', {
                tenant: daoName,
                to: this.daoContract.accountName,
                quantity,
                memo: 'Increasing voice'
            }, getAccountPermission(this.daoContract)),
            this.buildAction(this.peerContracts.voice, 'transfer', {
                tenant: daoName,
                from: this.daoContract.accountName,
                to: accountName,
                quantity,
                memo: 'Increasing voice'
            }, getAccountPermission(this.daoContract))
        ];
    }

    async setup() {
        this.daoContract.resetTables();


        this.setupDate = new Date();
        this.currentDate = new Date(this.setupDate);
        this.setCurrentTime(this.setupDate);

        this.daoContract.updateAuth(`active`, `owner`, {
            accounts: [
                {
                permission: {
                    actor: this.daoContract.accountName,
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
                    actor: this.daoContract.accountName,
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

        this.peerContracts.voice.updateAuth(`active`, `owner`, {
            accounts: [
                {
                    permission: {
                        actor: this.peerContracts.voice.accountName,
                        permission: `eosio.code`
                    },
                    weight: 1
                },
                {
                    permission: {
                        actor: this.daoContract.accountName,
                        permission: `active`
                    },
                    weight: 1
                }
            ]
        });

        const newAccounts = this.daos.reduce((accounts, dao) => {
            if (!this.accounts[dao.settings.onboarderAccount]) {
                accounts.add(dao.settings.onboarderAccount)
            }

            return accounts;
        }, new Set() as Set<string>);

        newAccounts.forEach(account => this.createAccount(account));

        await this.sendTransaction({
            actions:[
                this.buildAction(this.daoContract, 'createroot', { notes: 'notes' }),
                // governance token contract
                this.buildAction(this.daoContract, 'setsetting', {
                    key: 'comments_contract',
                    value: [ 'name', this.peerContracts.comments.accountName ],
                    group: null
                }),
                this.buildAction(this.daoContract, 'setsetting', {
                    key: 'governance_token_contract',
                    value: [ 'name', this.peerContracts.voice.accountName ],
                    group: null
                }),
                this.buildAction(this.daoContract, 'setsetting', {
                    key: 'reward_token_contract',
                    value: [ 'name', this.peerContracts.voice.accountName ],
                    group: null
                }),
                this.buildAction(this.daoContract, 'setsetting', {
                    key: 'peg_token_contract',
                    value: [ 'name', this.peerContracts.voice.accountName ],
                    group: null
                }),
                this.buildAction(this.daoContract, 'setsetting', {
                    key: 'treasury_contract',
                    value: [ 'name', this.peerContracts.bank.accountName ],
                    group: null
                }),
                ...this.daos.map(dao => this.createDaoAction(dao))
            ]
        });

        this.fillDaoDocument();

        this.root = getDocumentById(this.getDaoDocuments(), '1');
        for (const dao of this.daos) {
            await this.createPeriods(dao);
        }
    }

    public getDaoDocuments(): Array<Document> {
        return this.daoContract.getTableRowsScoped('documents')['dao'];
    }

    public getDaoEdges(): Array<Edge> {
        return this.daoContract.getTableRowsScoped('edges')['dao'];
    }

    public getIssuedHvoice(dao: Dao): Asset {
        return Asset.fromString(
            this.peerContracts.voice.getTableRowsScoped('stat')[dao.settings.tokens.voice.asset.symbol][0].supply
        );
    }

    public getHvoiceForMember(dao: Dao, member: string): Asset {
        return Asset.fromString(
            this.peerContracts.voice.getTableRowsScoped('accounts')[member].find(a => a.tenant === dao.name).balance
        );
    }

    public getHusdForMember(dao: Dao, member: string): Asset {
      return Asset.fromString(
        this.peerContracts.husd.getTableRowsScoped('accounts')[member].find(a => a.tenant === dao.name).balance
      );
    }

    public getRoot(): Document {
        return this.root;
    }

    public getSetupDate(): Date {
        return this.setupDate;
    }

    private fillDaoDocument() {
        getDocumentsByType(this.getDaoDocuments(), 'dao').forEach(daoDoc => {
            const nameContent = getContent(getDetailsGroup(daoDoc), 'dao_name');
            if (nameContent.value[0] === ContentType.NAME) {
                const dao = this.getDao(nameContent.value[1]);
                dao.setId(daoDoc.id);
                dao.setRoot(daoDoc);
            } else {
                throw new Error('Invalid dao document:' + JSON.stringify(daoDoc));
            }
        });
    }

    private validateParams(account: Account, action: string, data: any) {
        // Not currently connected.
        // Need to support optionals values
        // It can be added to the accounts by creating a Proxy
        // and in the `buildAction` is easily callable
        //
        // Validate that data does not have any more params than it is required
        // It currently warns when an abi/action/struct is not found.

        if (!account.abi) {
            console.warn(`Abi for account ${account.accountName} not found when calling action: ${action}`);
            return;
        }

        const abiAction = account.abi.actions.find(a => a.name === action);
        if (!abiAction) {
            console.warn(`action for account ${account.accountName} not found when calling action: ${action}`);
            return;
        }

        const struct = account.abi.structs.find(s => s.name === abiAction.type);
        if (!struct) {
            console.warn(`action type for account ${account.accountName} not found when calling action: ${action}`);
            return;
        }

        const targetKeys = Object.keys(data).sort();
        const expectedKeys = struct
            .fields
            .map(f => ({
                name: f.name,
                isOptional: f.type.endsWith('$'),
                nullRequired: f.type.endsWith('?'),
                type: f.type
            }))
            .sort((a,b) => a.name > b.name ? 1 : -1);

        const missing = expectedKeys.filter(e => !e.isOptional && !targetKeys.includes(e.name));
        const expectedRawKeys = expectedKeys.map(e => e.name);
        const excess = targetKeys.filter(t => !expectedRawKeys.includes(t));


        (expect(missing) as any)._toBePresent(account, action);
        (expect(excess) as any)._toBeAbsent(account, action);
    }

    private createDaoAction(dao: Dao): TTransaction['actions'][number] {
        return this.buildAction(
            this.daoContract,
            'createdao',
            {
                config: [
                    ContentGroupBuilder.builder()
                        .groupLabel('details')
                        .name('dao_name', dao.name)
                        .int64('voting_duration_sec', dao.settings.votingDurationSeconds)
                        .string('dao_description', dao.settings.description)
                        .string('dao_title', dao.settings.title)
                        .asset('peg_token', dao.settings.tokens.peg.name)
                        .asset('voice_token', dao.settings.tokens.voice.asset)
                        .asset('reward_token', dao.settings.tokens.reward.name)
                        .asset('reward_to_peg_ratio', dao.settings.tokens.reward.toPegRatio)
                        .int64('period_duration_sec', dao.settings.periodDurationSeconds)
                        .name('onboarder_account', dao.settings.onboarderAccount)
                        .int64('voting_quorum_x100', dao.settings.voting.quorumFactorX100)
                        .int64('voting_alignment_x100', dao.settings.voting.alignmentFactorX100)
                        // Not used yet
                        .int64('voice_token_decay_period', dao.settings.tokens.voice.decayPeriod)
                        .int64('voice_token_decay_per_period_x10M', dao.settings.tokens.voice.decayPerPeriodx10M)
                        .build()
                ]
            },
            [
                {
                    actor: dao.settings.onboarderAccount,
                    permission: 'active'
                }
            ]
        )
    }

}
