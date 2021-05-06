import { loadConfig, Blockchain } from '@klevoya/hydra';
import { setupEnvironment } from './setup';

const config = loadConfig("hydra.yml");

describe("dao", () => {
  let environment;
  /*
  let blockchain = new Blockchain(config);
  let tester = blockchain.createAccount(`dao`);

  beforeAll(async () => {
    tester.setContract(blockchain.contractTemplates[`dao`]);
    tester.updateAuth(`active`, `owner`, {
      accounts: [
        {
          permission: {
            actor: tester.accountName,
            permission: `eosio.code`
          },
          weight: 1
        }
      ]
    });
  });*/

  beforeEach(async () => {
    // tester.resetTables();
    environment = await setupEnvironment();
  });

  const role1 = {
    content_groups: [
      [
        {
          label: "content_group_label",
          value: [ "string", "details" ]
        },
        {
          label: "title",
          value: [ "string", "Underwater Basketweaver" ]
        },
        {
          label: "description",
          value: [ "string", "Weave baskets at the bottom of the sea" ]
        },
        {
          label: "annual_usd_salary",
          value: [ "asset", "150000.00 USD" ]
        },
        {
          label: "start_period",
          value: [ "int64", 0 ]
        },
        {
          label: "end_period",
          value: [ "int64", 9 ]
        },
        {
          label: "fulltime_capacity_x100",
          value: [ "int64", 100 ]
        },
        {
          label: "min_time_share_x100",
          value: [ "int64", 50 ]
        },
        {
          label: "min_deferred_x100",
          value: [ "int64", 50 ]
        }
      ]
    ]
  };

  it("can send the propose action", async () => {
    await environment.dao.contract.propose({
      proposer: environment.alice.accountName,
      proposal_type: "role",
      ...role1
    });

    expect(environment.dao.getTableRowsScoped(`documents`)[`dao`]).toEqual([{}]);
  });
});
