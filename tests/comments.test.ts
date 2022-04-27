import { setupEnvironment } from "./setup";
import { Document } from './types/Document';
import { last } from './utils/Arrays';
import { getContent, getDocumentsByType, getSystemContentGroup } from './utils/Dao';
import { DocumentBuilder } from './utils/DocumentBuilder';
import { getDaoExpect } from './utils/Expect';
import { passProposal } from './utils/Proposal';

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

    it('Proposals have a comment section', async() => {
        const environment = await setupEnvironment();
        const dao = environment.getDao('test');
        const daoExpect = getDaoExpect(environment);

        const now = new Date();
        const whenVoteExpires = new Date();
        whenVoteExpires.setSeconds( whenVoteExpires.getSeconds() + dao.settings.votingDurationSeconds + 1);
        environment.setCurrentTime(now);

        // Proposing
        await environment.daoContract.contract.propose({
            dao_id: dao.getId(),
            proposer: dao.members[0].account.accountName,
            proposal_type: 'role',
            publish: true,
            content_groups: getSampleRole().content_groups
        });

        let proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));

        let commentSection = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'cmnt.section'
        ));

        // proposal <-----> comment section
        daoExpect.toHaveEdge(proposal, commentSection, 'cmntsect');
        daoExpect.toHaveEdge(commentSection, proposal, 'cmntsectof');

        /*
        await environment.daoContract.contract.vote({
            voter: dao.members[0].account.accountName,
            proposal_id: proposal.id,
            vote: 'pass',
            notes: 'vote pass'
        });

        // Sets the time to the end of proposal
        environment.setCurrentTime(whenVoteExpires);

        // Now we can close the proposal
        await environment.daoContract.contract.closedocprop({
            proposal_id: proposal.id
        }, dao.members[0].getPermissions());

        // When passing, the proposal is updated
        proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));

        daoExpect.toHaveEdge(dao.getRoot(), proposal, 'passedprops');*/
    });

});
