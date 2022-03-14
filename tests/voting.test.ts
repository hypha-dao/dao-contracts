import { DaoBlockchain } from './dao/DaoBlockchain';
import { setupEnvironment } from './setup';
import { Document } from './types/Document';
import { last } from './utils/Arrays';
import { getDocumentsByType } from './utils/Dao';
import { DocumentBuilder } from './utils/DocumentBuilder';
import { toISOString } from './utils/Date';
import { getDaoExpect } from './utils/Expect';

describe('Voting', () => {

    const getSampleRole = (title: string = 'Underwater Basketweaver'): Document => DocumentBuilder
    .builder()
    .contentGroup(builder => {
      builder
      .groupLabel('details')
      .string('title', title)
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

    const getVote = (props: { voter: string, power: string, vote: string, date: string, notes: string}) => DocumentBuilder.builder()
    .contentGroup(builder => builder
        .groupLabel('vote')
        .name('voter', props.voter)
        .asset('vote_power', props.power)
        .string('vote', props.vote)
        .timePoint('date', props.date)
        .string('notes', props.notes)
    )
    .contentGroup(builder => builder.groupLabel('system').name('type', 'vote').string('node_label', `Vote: ${props.vote}`))
    .build();

    const getLastTally = (environment: DaoBlockchain, isEqual?: boolean, oldTally?: Document) => {
        const documents = environment.getDaoDocuments();
        const lastTally = last(getDocumentsByType(documents, 'vote.tally'));
        if (oldTally) {
            if (isEqual) {
                expect(oldTally.id).toEqual(lastTally.id);
            } else {
                expect(oldTally.id).not.toEqual(lastTally.id);
            }
        }

        return lastTally;
    };

    const testLastVoteTally = (votetally: Document, props: { pass: number, abstain: number, fail: number }) => {
        const expected = getVoteTally(props);
        expect(votetally.content_groups).toEqual(expected.content_groups);
        return votetally;
    };

    const testLastVote = (environment: DaoBlockchain, props: { voter: string, power: string, vote: string, date: string, notes: string}) => {
        const vote = last(getDocumentsByType(environment.getDaoDocuments(), 'vote'));
        const expected = getVote(props);
        expect(vote.content_groups).toEqual(expected.content_groups);
        return vote;
    };

    const testVoteEdges = (environment: DaoBlockchain, proposal: Document, voteTally: Document, vote?: Document) => {
        getDaoExpect(environment).toHaveEdge(proposal, voteTally, 'votetally');
        if (vote) {
            getDaoExpect(environment).toHaveEdge(proposal, vote, 'vote');
        }
    };

    it('Proposals voting', async () => {
        const environment = await setupEnvironment();
        // Time doesn't move in the test unless we set the current time.
        const now = new Date();

        const dao = environment.getDao('test');

        // We have to set the initial time if we care about any timing
        environment.setCurrentTime(now);

        // Proposing
        await environment.daoContract.contract.propose({
            dao_id: dao.getId(),
            proposer: environment.daos[0].members[0].account.accountName,
            proposal_type: 'role',
            publish: true,
            content_groups: getSampleRole().content_groups
        });

        const proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));
        console.log(environment.getDaoDocuments());
        let lastTally = getLastTally(environment);
        testLastVoteTally(lastTally, { pass: 0, fail: 0, abstain: 0 });
        testVoteEdges(environment, proposal, lastTally);

        // Proposing other role to test
        await environment.daoContract.contract.propose({
            dao_id: dao.getId(),
            proposer: environment.daos[0].members[0].account.accountName,
            proposal_type: 'role',
            publish: true,
            content_groups: getSampleRole('foobar').content_groups
        });

        const proposal2 = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));
        // Tallies are reused when possible. In this case the same value is the same tally
        lastTally = getLastTally(environment, true, lastTally);
        testLastVoteTally(lastTally, { pass: 0, fail: 0, abstain: 0 });
        testVoteEdges(environment, proposal2, lastTally);

        // Member 1 votes something wrong
        await expect(environment.daoContract.contract.vote({
            voter: environment.daos[0].members[0].account.accountName,
            proposal_id: proposal.id,
            vote: 'foobar',
            notes: 'vote something wrong'
        })).rejects.toThrow(/invalid vote/i);

        // Now votes pass
        await environment.daoContract.contract.vote({
            voter: environment.daos[0].members[0].account.accountName,
            proposal_id: proposal.id,
            vote: 'pass',
            notes: 'vote pass'
        });


        lastTally = getLastTally(environment, false, lastTally);
        testLastVoteTally(lastTally, { pass: 100, fail: 0, abstain: 0 });
        let lastVote = testLastVote(environment, {
            vote: 'pass',
            date: toISOString(now),
            power: '100.00 HVOICE',
            voter: environment.daos[0].members[0].account.accountName,
            notes: 'vote pass'
        });
        testVoteEdges(environment, proposal, lastTally, lastVote);

        // Changes vote to fail
        await environment.daoContract.contract.vote({
            voter: environment.daos[0].members[0].account.accountName,
            proposal_id: proposal.id,
            vote: 'fail',
            notes: 'votes fail'
        });
        lastTally = getLastTally(environment, false, lastTally);
        testLastVoteTally(lastTally, { pass: 0, fail: 100, abstain: 0 });
        lastVote = testLastVote(environment, {
            vote: 'fail',
            date: toISOString(now),
            power: '100.00 HVOICE',
            voter: environment.daos[0].members[0].account.accountName,
            notes: 'votes fail'
        });
        testVoteEdges(environment, proposal, lastTally, lastVote);

        // member[1] votes to abstain
        await environment.daoContract.contract.vote({
            voter: environment.daos[0].members[1].account.accountName,
            proposal_id: proposal.id,
            vote: 'abstain',
            notes: 'votes abstain'
        });
        lastTally = getLastTally(environment, false, lastTally);
        testLastVoteTally(lastTally, { pass: 0, fail: 100, abstain: 1 });
        lastVote = testLastVote(environment, {
            vote: 'abstain',
            date: toISOString(now),
            power: '1.00 HVOICE',
            voter: environment.daos[0].members[1].account.accountName,
            notes: 'votes abstain'
        });
        testVoteEdges(environment, proposal, lastTally, lastVote);

        // member[2] votes to abstain
        await environment.daoContract.contract.vote({
            voter: environment.daos[0].members[2].account.accountName,
            proposal_id: proposal.id,
            vote: 'abstain',
            notes: 'votes abstain 2'
        });
        lastTally = getLastTally(environment, false, lastTally);
        testLastVoteTally(lastTally, { pass: 0, fail: 100, abstain: 2 });
        lastVote = testLastVote(environment, {
            vote: 'abstain',
            date: toISOString(now),
            power: '1.00 HVOICE',
            voter: environment.daos[0].members[2].account.accountName,
            notes: 'votes abstain 2'
        });
        testVoteEdges(environment, proposal, lastTally, lastVote);

        // member[3] votes to pass
        await environment.daoContract.contract.vote({
            voter: environment.daos[0].members[3].account.accountName,
            proposal_id: proposal.id,
            vote: 'pass',
            notes: 'votes pass'
        });
        lastTally = getLastTally(environment, false, lastTally);
        testLastVoteTally(lastTally, { pass: 1, fail: 100, abstain: 2 });
        lastVote = testLastVote(environment, {
            vote: 'pass',
            date: toISOString(now),
            power: '1.00 HVOICE',
            voter: environment.daos[0].members[3].account.accountName,
            notes: 'votes pass'
        });
        testVoteEdges(environment, proposal, lastTally, lastVote);

        // member[0] changes vote to pass
        await environment.daoContract.contract.vote({
            voter: environment.daos[0].members[0].account.accountName,
            proposal_id: proposal.id,
            vote: 'pass',
            notes: 'votes pass'
        });
        lastTally = getLastTally(environment, false, lastTally);
        lastVote = testLastVote(environment, {
            vote: 'pass',
            date: toISOString(now),
            power: '100.00 HVOICE',
            voter: environment.daos[0].members[0].account.accountName,
            notes: 'votes pass'
        });
        testLastVoteTally(lastTally, { pass: 101, fail: 0, abstain: 2 });
        testVoteEdges(environment, proposal, lastTally, lastVote);

        // closes proposal
        await expect(environment.daoContract.contract.closedocprop({
                proposal_id: proposal.id
        }, environment.daos[0].members[0].getPermissions())
        ).rejects.toThrow(/voting is still active/i);

        // Sets the time to the end of proposal
        const tomorrow = new Date();
        tomorrow.setDate(tomorrow.getDate() + 1);
        environment.setCurrentTime(tomorrow);

        // Voting expired
        await expect(environment.daoContract.contract.vote({
            voter: environment.daos[0].members[0].account.accountName,
            proposal_id: proposal.id,
            vote: 'pass',
            notes: 'votes when proposal is closed'
        })).rejects.toThrow(/voting has expired/i);

        // Now we can close the proposal
        await environment.daoContract.contract.closedocprop({
            proposal_id: proposal.id
        }, environment.daos[0].members[0].getPermissions());
    });
});
