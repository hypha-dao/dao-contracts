import { loadConfig, Blockchain } from '@klevoya/hydra';
import { DaoBlockchain } from './DaoBlockchain';
import { setupEnvironment } from './setup';

const config = loadConfig("hydra.yml");

describe("dao", () => {
  let environment: DaoBlockchain;

  beforeEach(async () => {
    environment = await setupEnvironment();
  });

  const getSystemContent = (contentGroups: any) => {
    return contentGroups.find(group => group.find(content => content.label === 'content_group_label' && content.value[0] === 'string' && content.value[1] === 'system'));
  };

  const getDocumentByType = (table: any, type: string) => {
    return table.find(element => {
      const system = getSystemContent(element.content_groups);
      if (system) {
        return system.find(el => el.label === 'type' && el.value[1] === type);
      }

      return false;
    });
  };

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
      proposer: environment.members[0].accountName,
      proposal_type: "role",
      ...role1
    });

    const table = environment.dao.getTableRowsScoped(`documents`)[`dao`];
    const votetally = getDocumentByType(table, 'vote.tally');
    expect(votetally.content_groups).toEqual([
      [
        {
          label: 'content_group_label',
          value: [
            'string', 'pass'
          ]
        },
        {
          label: 'vote_power',
          value: [ 'asset', '0.00 HVOICE' ]
        }
      ],
      [
        {
          label: 'content_group_label',
          value: [
            'string', 'abstain'
          ]
        },
        {
          label: 'vote_power',
          value: [ 'asset', '0.00 HVOICE' ]
        }
      ],
      [
        {
          label: 'content_group_label',
          value: [
            'string', 'fail'
          ]
        },
        {
          label: 'vote_power',
          value: [ 'asset', '0.00 HVOICE' ]
        }
      ],
      [
        {
          label: 'content_group_label',
          value: [ 'string', 'system' ]
        },
        {
          label: 'type',
          value: [ 'name', 'vote.tally' ]
        },
        {
          label: 'node_label',
          value: [ 'string', 'VoteTally' ] 
        }
      ]
    ]);
  });
});
