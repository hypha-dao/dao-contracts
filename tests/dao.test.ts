import { loadConfig, Blockchain } from '@klevoya/hydra';
import { DaoBlockchain } from './DaoBlockchain';
import { setupEnvironment } from './setup';
import { Document, ContentType, makeStringContent, makeAssetContent, makeInt64Content, makeNameContent, makeContentGroup } from './types/Document';
import { getDocumentByType } from './utils/Dao';
import { DocumentBuilder } from './utils/DocumentBuilder';

const config = loadConfig("hydra.yml");

describe("dao", () => {
  let environment: DaoBlockchain;

  beforeEach(async () => {
    // Could probably speed this up by only calling the environment.setup() method
    environment = await setupEnvironment();
  });

  const role1: Document = DocumentBuilder
  .builder()
  .contentGroup(builder => {
    builder
    .groupLabel('details')
    .string('title', 'Underwater Basketweaver')
    .string('description', 'Weave baskets at the bottom of the sea')
    .asset('annual_usd_salary', '150000.00 USD')
    .int64('start_period', 0)
    .int64('end_period', 9)
    .int64('fulltime_capacity_x100', 100)
    .int64('min_time_share_x100', 50)
    .int64('min_deferred_x100', 50)
  })
  .build();

  it("can send the propose action", async () => {
    await environment.dao.contract.propose({
      proposer: environment.members[0].accountName,
      proposal_type: "role",
      ...role1
    });

    const documents = environment.getDaoDocuments();
    const votetally = getDocumentByType(documents, 'vote.tally');

    const expected = DocumentBuilder.builder()
    .contentGroup(builder => builder.groupLabel('pass').asset('vote_power', '0.00 HVOICE'))
    .contentGroup(builder => builder.groupLabel('abstain').asset('vote_power', '0.00 HVOICE'))
    .contentGroup(builder => builder.groupLabel('fail').asset('vote_power', '0.00 HVOICE'))
    .contentGroup(builder => builder.groupLabel('system').name('type', 'vote.tally').string('node_label', 'VoteTally'))
    .build();

    expect(votetally.content_groups).toEqual(expected.content_groups);
  });
});
