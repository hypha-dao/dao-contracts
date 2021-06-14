import { loadConfig, Blockchain } from '@klevoya/hydra';
import { DaoBlockchain } from './DaoBlockchain';
import { setupEnvironment } from './setup';
import { Document, ContentType, makeStringContent, makeAssetContent, makeInt64Content, makeNameContent, makeContentGroup } from './types/Document';
import { last } from './utils/Arrays';
import { getDocumentsByType } from './utils/Dao';
import { DocumentBuilder } from './utils/DocumentBuilder';

const config = loadConfig('hydra.yml');

describe('Voting', () => {

    const getSampleRole = (): Document => DocumentBuilder
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

    const getVoteTally = (props: { pass: number, abstain: number, fail: number }) => DocumentBuilder.builder()
    .contentGroup(builder => builder.groupLabel('pass').asset('vote_power', `${props.pass}.00 HVOICE`))
    .contentGroup(builder => builder.groupLabel('abstain').asset('vote_power', `${props.abstain}.00 HVOICE`))
    .contentGroup(builder => builder.groupLabel('fail').asset('vote_power', `${props.fail}.00 HVOICE`))
    .contentGroup(builder => builder.groupLabel('system').name('type', 'vote.tally').string('node_label', 'VoteTally'))
    .build();

    const testLastVoteTally = (environment: DaoBlockchain, props: { pass: number, abstain: number, fail: number }) => {
        const documents = environment.getDaoDocuments();
        const votetallies = getDocumentsByType(documents, 'vote.tally');
        const expected = getVoteTally(props);
        expect(last(votetallies).content_groups).toEqual(expected.content_groups);
    };

    it('Proposals voting', async () => {
        const environment = await setupEnvironment();

        // Proposing
        await environment.dao.contract.propose({
            proposer: environment.members[0].accountName,
            proposal_type: 'role',
            ...getSampleRole()
        });
        
        const proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));

        testLastVoteTally(environment, { pass: 0, fail: 0, abstain: 0 });

        // Member 1 votes something wrong
        await expect(environment.dao.contract.vote({
            voter: environment.members[0].accountName,
            proposal_hash: proposal.hash,
            vote: 'foobar'
        })).rejects.toThrow(/invalid vote/i);

        // Now votes pass
        await environment.dao.contract.vote({
            voter: environment.members[0].accountName,
            proposal_hash: proposal.hash,
            vote: 'pass'
        });

        testLastVoteTally(environment, { pass: 100, fail: 0, abstain: 0 });

        // More to come...
    });
});
