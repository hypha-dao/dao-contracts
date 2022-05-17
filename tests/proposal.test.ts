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
    const getSampleRole2 = (title: string = 'Underwater Basketweaver pro'): Document => DocumentBuilder
    .builder()
    .contentGroup(builder => {
      builder
      .groupLabel('details')
      .string('title', title)
      .string('description', 'Weave baskets at the bottom of the sea without air')
      .asset('annual_usd_salary', '550000.00 USD')
      .int64('start_period', 0)
      .int64('end_period', 9)
      .int64('fulltime_capacity_x100', 100)
      .int64('min_time_share_x100', 50)
      .int64('min_deferred_x100', 50)
    })
    .build();

    it('Proposal failed', async() => {
        const environment = await setupEnvironment();
        const dao = environment.getDao('test');

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

        const daoExpect = getDaoExpect(environment);
        daoExpect.toHaveEdge(dao.getRoot(), proposal, 'proposal');
        daoExpect.toHaveEdge(dao.members[0].doc, proposal, 'owns');
        daoExpect.toHaveEdge(proposal, dao.members[0].doc, 'ownedby');

        // Sets the time to the end of proposal
        environment.setCurrentTime(whenVoteExpires);

        // Now we can close the proposal
        await environment.daoContract.contract.closedocprop({
            proposal_id: proposal.id
        }, dao.members[0].getPermissions());

        proposal = last(getDocumentsByType(
          environment.getDaoDocuments(),
          'role'
        ));

        daoExpect.toHaveEdge(dao.getRoot(), proposal, 'failedprops');
    });

    it('Proposal passes', async() => {
        const environment = await setupEnvironment();
        const dao = environment.getDao('test');

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

        const daoExpect = getDaoExpect(environment);
        daoExpect.toHaveEdge(dao.getRoot(), proposal, 'proposal');
        daoExpect.toHaveEdge(dao.members[0].doc, proposal, 'owns');
        daoExpect.toHaveEdge(proposal, dao.members[0].doc, 'ownedby');

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

        daoExpect.toHaveEdge(dao.getRoot(), proposal, 'passedprops');
    });

    it('Staging proposals', async () => {
        const environment = await setupEnvironment({
            'test': {
                periodCount: 0
            }
        });
        const dao = environment.getDao('test');
        const now = new Date();
        const whenVoteExpires = new Date();
        whenVoteExpires.setSeconds( whenVoteExpires.getSeconds() + dao.settings.votingDurationSeconds + 1);
        environment.setCurrentTime(now);

        // Stage proposal
        await environment.daoContract.contract.propose({
            dao_id: dao.getId(),
            proposer: dao.members[0].account.accountName,
            proposal_type: 'role',
            publish: false,
            content_groups: getSampleRole().content_groups
        });

        let proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));

        const daoExpect = getDaoExpect(environment);
        daoExpect.toNotHaveEdge(dao.getRoot(), proposal, 'proposal');
        daoExpect.toHaveEdge(dao.getRoot(), proposal, 'stagingprop');
        daoExpect.toHaveEdge(dao.members[0].doc, proposal, 'owns');
        daoExpect.toHaveEdge(proposal, dao.members[0].doc, 'ownedby');

        // can't vote
        await expect(environment.daoContract.contract.vote({
            voter: dao.members[0].account.accountName,
            proposal_id: proposal.id,
            vote: 'pass',
            notes: 'vote pass'
        })).rejects.toThrowError('Only published proposals can be voted');

        // can't close
        await expect(environment.daoContract.contract.closedocprop({
            proposal_id: proposal.id
        }, dao.members[0].getPermissions()))
        .rejects.toThrowError('Only published proposals can be closed');

        // publish
        await environment.daoContract.contract.proposepub({
            proposer: dao.members[0].account.accountName,
            proposal_id: proposal.id
        });

        proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));

        await passProposal(dao, proposal, 'role', environment);

        proposal = last(getDocumentsByType(environment.getDaoDocuments(), 'role'));
        // can't publish
        await expect(environment.daoContract.contract.proposepub({
            proposer: dao.members[0].account.accountName,
            proposal_id: proposal.id
        })).rejects.toThrowError(/Only proposes in staging can be published/i);

        // can't update
        await expect(environment.daoContract.contract.proposeupd({
            proposer: dao.members[1].account.accountName,
            proposal_id: proposal.id,
            content_groups: getSampleRole2().content_groups
        })).rejects.toThrowError(/Only proposes in staging can be updated/i);

        // can't remove
        await expect(environment.daoContract.contract.proposerem({
            proposer: dao.members[1].account.accountName,
            proposal_id: proposal.id,
        })).rejects.toThrowError(/Only proposes in staging can be removed/i);

        // Create suspend proposal
        await environment.daoContract.contract.suspend({
            proposer: dao.members[0].account.accountName,
            document_id: proposal.id,
            reason: 'I would like to suspend'
        }, dao.members[0].getPermissions());

        proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'suspend'
        ));

        // Should be published
        daoExpect.toHaveEdge(dao.getRoot(), proposal, 'proposal');
        daoExpect.toNotHaveEdge(dao.getRoot(), proposal, 'stagingprop');
        daoExpect.toHaveEdge(dao.members[0].doc, proposal, 'owns');
        daoExpect.toHaveEdge(proposal, dao.members[0].doc, 'ownedby');

        // Staging proposals can also be updated or removed (by proposer)
        await environment.daoContract.contract.propose({
            dao_id: dao.getId(),
            proposer: dao.members[0].account.accountName,
            proposal_type: 'role',
            publish: false,
            content_groups: getSampleRole('old-title').content_groups
        });

        proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));

        await expect(environment.daoContract.contract.proposeupd({
            proposer: dao.members[1].account.accountName,
            proposal_id: proposal.id,
            content_groups: getSampleRole2().content_groups
        })).rejects.toThrowError(/Only the proposer can update the proposal/i);

        await environment.daoContract.contract.proposeupd({
            proposer: dao.members[0].account.accountName,
            proposal_id: proposal.id,
            content_groups: getSampleRole2('new-title').content_groups
        });

        proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));

        expect(getContent(getSystemContentGroup(proposal), 'node_label').value[1]).toEqual('new-title');

        daoExpect.toNotHaveEdge(environment.getRoot(), proposal, 'proposal');
        daoExpect.toHaveEdge(dao.getRoot(), proposal, 'stagingprop');
        daoExpect.toHaveEdge(dao.members[0].doc, proposal, 'owns');
        daoExpect.toHaveEdge(proposal, dao.members[0].doc, 'ownedby');

        daoExpect.toNotHaveEdge(environment.getRoot(), proposal, 'proposal');
        daoExpect.toHaveEdge(dao.getRoot(), proposal, 'stagingprop');
        daoExpect.toHaveEdge(dao.members[0].doc, proposal, 'owns');
        daoExpect.toHaveEdge(proposal, dao.members[0].doc, 'ownedby');

        await expect(environment.daoContract.contract.proposerem({
            proposer: dao.members[1].account.accountName,
            proposal_id: proposal.id,
        })).rejects.toThrowError(/Only the proposer can remove the proposal/i);

        await environment.daoContract.contract.proposerem({
            proposer: dao.members[0].account.accountName,
            proposal_id: proposal.id
        });

        expect(last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ))).not.toEqual(proposal);
        daoExpect.toNotHaveEdge(dao.getRoot(), proposal, 'proposal');
        daoExpect.toNotHaveEdge(dao.getRoot(), proposal, 'stagingprop');
        daoExpect.toNotHaveEdge(dao.members[0].doc, proposal, 'owns');
        daoExpect.toNotHaveEdge(proposal, dao.members[0].doc, 'ownedby');
    })
});
