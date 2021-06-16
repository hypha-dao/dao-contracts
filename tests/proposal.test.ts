import { setupEnvironment } from "./setup";
import { Document } from './types/Document';
import { last } from './utils/Arrays';
import { getDocumentsByType } from './utils/Dao';
import { DocumentBuilder } from './utils/DocumentBuilder';
import { getDaoExpect } from './utils/Expect';
import { getAccountPermission } from './utils/Permissions';

describe('Proposal', () => {
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

    it('Proposal failed', async() => {
        const environment = await setupEnvironment();

        const now = new Date();
        const whenVoteExpires = new Date();
        whenVoteExpires.setSeconds( whenVoteExpires.getSeconds() + environment.settings.votingDurationSeconds + 1);

        environment.setCurrentTime(now);

        // Proposing
        await environment.dao.contract.propose({
            proposer: environment.members[0].account.accountName,
            proposal_type: 'role',
            content_groups: getSampleRole().content_groups
        });

        const proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));

        const daoExpect = getDaoExpect(environment);
        daoExpect.toHaveEdge(environment.getRoot(), proposal, 'proposal');
        daoExpect.toHaveEdge(environment.members[0].doc, proposal, 'owns');
        daoExpect.toHaveEdge(proposal, environment.members[0].doc, 'ownedby');

        // Sets the time to the end of proposal
        environment.setCurrentTime(whenVoteExpires);

        // Now we can close the proposal
        await environment.dao.contract.closedocprop({
            proposal_hash: proposal.hash
        }, getAccountPermission(environment.members[0].account));

        daoExpect.toHaveEdge(environment.getRoot(), proposal, 'failedprops');
    });

    it('Proposal passes', async() => {
        const environment = await setupEnvironment();

        const now = new Date();
        const whenVoteExpires = new Date();
        whenVoteExpires.setSeconds( whenVoteExpires.getSeconds() + environment.settings.votingDurationSeconds + 1);

        environment.setCurrentTime(now);

        // Proposing
        await environment.dao.contract.propose({
            proposer: environment.members[0].account.accountName,
            proposal_type: 'role',
            content_groups: getSampleRole().content_groups
        });

        let proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));

        const daoExpect = getDaoExpect(environment);
        daoExpect.toHaveEdge(environment.getRoot(), proposal, 'proposal');
        daoExpect.toHaveEdge(environment.members[0].doc, proposal, 'owns');
        daoExpect.toHaveEdge(proposal, environment.members[0].doc, 'ownedby');

        await environment.dao.contract.vote({
            voter: environment.members[0].account.accountName,
            proposal_hash: proposal.hash,
            vote: 'pass'
        });

        // Sets the time to the end of proposal
        environment.setCurrentTime(whenVoteExpires);

        // Now we can close the proposal
        await environment.dao.contract.closedocprop({
            proposal_hash: proposal.hash
        }, getAccountPermission(environment.members[0].account));

        // When passing, the proposal is updated
        proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));

        daoExpect.toHaveEdge(environment.getRoot(), proposal, 'passedprops');
    });
});
