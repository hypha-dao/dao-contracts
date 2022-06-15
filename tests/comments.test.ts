import { setupEnvironment } from "./setup";
import { Document } from './types/Document';
import { last } from './utils/Arrays';
import {getContent, getContentGroupByLabel, getDocumentsByType, getEdgesByFilter} from './utils/Dao';
import { DocumentBuilder } from './utils/DocumentBuilder';
import { getDaoExpect } from './utils/Expect';
import { getAccountPermission } from "./utils/Permissions";

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

    const checkReaction = (reaction: Document, who: string, what: string) => {
        const likeDetails = getContentGroupByLabel(reaction, 'reaction');
        expect(getContent(likeDetails, 'type').value[1]).toBe(what);
    }

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
            publish: false,
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


        expect(getDocumentsByType(
            environment.getDaoDocuments(),
            "reaction"
        ).length).toBe(0);

        // proposal <-----> comment section
        daoExpect.toHaveEdge(proposal, commentSection, 'cmntsect');
        daoExpect.toHaveEdge(commentSection, proposal, 'cmntsectof');

        // likes
        await environment.daoContract.contract.reactadd({
            user: dao.members[0].account.accountName,
            reaction: 'liked',
            document_id: commentSection.id
        }, getAccountPermission(dao.members[0].account));

        commentSection = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'cmnt.section'
        ));

        let reaction = last(getDocumentsByType(
            environment.getDaoDocuments(),
            "reaction"
        ));
        let firstReactionId = reaction.id;

        // likeable <-----> reaction
        daoExpect.toHaveEdge(dao.members[0].doc, commentSection, 'reactedto');
        daoExpect.toHaveEdge(commentSection, dao.members[0].doc, 'reactedby');

        daoExpect.toHaveEdge(commentSection, reaction, 'reaction');
        daoExpect.toHaveEdge(reaction, commentSection, 'reactionof');

        daoExpect.toHaveEdge(dao.members[0].doc, reaction, 'reactionlnk');
        daoExpect.toHaveEdge(reaction, dao.members[0].doc, 'reactionlnkr');

        checkReaction(reaction, dao.members[0].account.accountName, 'liked');

        await environment.daoContract.contract.reactadd({
            user: dao.members[1].account.accountName,
            reaction: 'liked',
            document_id: commentSection.id
        }, getAccountPermission(dao.members[1].account));
        commentSection = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'cmnt.section'
        ));
        reaction = last(getDocumentsByType(
            environment.getDaoDocuments(),
            "reaction"
        ));
        checkReaction(reaction, dao.members[1].account.accountName, 'liked');

        // unlike
        await environment.daoContract.contract.reactrem({
            user: dao.members[0].account.accountName,
            document_id: commentSection.id
        }, getAccountPermission(dao.members[0].account));
        commentSection = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'cmnt.section'
        ));

        expect(getDocumentsByType(
            environment.getDaoDocuments(),
            "reactionlnk"
        ).filter(d => d.id === firstReactionId).length).toBe(0);

        // add comment to a section
        await environment.daoContract.contract.cmntadd({
            author: dao.members[0].account.accountName,
            content: 'This is sweet!',
            comment_or_section_id: commentSection.id
        }, getAccountPermission(dao.members[0].account));
        let comment = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'comment'
        ));

        let commentContent = getContentGroupByLabel(comment, 'comment');
        expect(getContent(commentContent, 'author').value[1]).toBe(dao.members[0].account.accountName);
        expect(getContent(commentContent, 'content').value[1]).toBe('This is sweet!');
        expect(getContent(commentContent, 'edited')).toBeUndefined();
        expect(getContent(commentContent, 'deleted')).toBeUndefined();
        daoExpect.toHaveEdge(commentSection, comment, 'comment');
        daoExpect.toHaveEdge(comment, commentSection, 'commentof');

        // like comment acc0
        await environment.daoContract.contract.reactadd({
            user: dao.members[0].account.accountName,
            reaction: 'liked',
            document_id: comment.id
        }, getAccountPermission(dao.members[0].account));
        comment = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'comment'
        ));

        reaction = last(getDocumentsByType(
            environment.getDaoDocuments(),
            "reaction"
        ));
        checkReaction(reaction, dao.members[0].account.accountName, 'liked');

        // like comment acc1
        await environment.daoContract.contract.reactadd({
            user: dao.members[1].account.accountName,
            reaction: 'liked',
            document_id: comment.id
        }, getAccountPermission(dao.members[1].account));
        comment = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'comment'
        ));

        reaction = last(getDocumentsByType(
            environment.getDaoDocuments(),
            "reaction"
        ));
        checkReaction(reaction, dao.members[1].account.accountName, 'liked');
        expect(getEdgesByFilter(environment.getDaoEdges(), {
            edge_name: 'reaction',
            from_node: comment.id
        })).toHaveLength(1);

        // unlike comment acc1
        await environment.daoContract.contract.reactrem({
            user: dao.members[1].account.accountName,
            document_id: comment.id
        }, getAccountPermission(dao.members[1].account));
        comment = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'comment'
        ));

        // update
        await environment.daoContract.contract.cmntupd({
            new_content: 'This is terrible!',
            comment_id: comment.id
        }, getAccountPermission(dao.members[0].account));
        comment = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'comment'
        ));
        commentContent = getContentGroupByLabel(comment, 'comment');
        expect(getContent(commentContent, 'author').value[1]).toBe(dao.members[0].account.accountName);
        expect(getContent(commentContent, 'content').value[1]).toBe('This is terrible!');
        expect(getContent(commentContent, 'edited').value[1]).toBeTruthy();
        expect(getContent(commentContent, 'deleted')).toBeUndefined();
        daoExpect.toHaveEdge(commentSection, comment, 'comment');
        daoExpect.toHaveEdge(comment, commentSection, 'commentof');

        // a different user updating will fail
        await expect(environment.daoContract.contract.cmntupd({
            new_content: 'This is terrible!',
            comment_id: comment.id
        }, getAccountPermission(dao.members[1].account))).rejects.toThrow();

        // a different user removing will fail
        await expect(environment.daoContract.contract.cmntrem({
            comment_id: comment.id
        }, getAccountPermission(dao.members[1].account))).rejects.toThrow();

        // add sub comment
        await environment.daoContract.contract.cmntadd({
            author: dao.members[1].account.accountName,
            content: 'agree!',
            comment_or_section_id: comment.id
        }, getAccountPermission(dao.members[1].account));
        let subComment = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'comment'
        ));
        let subCommentContent = getContentGroupByLabel(subComment, 'comment');
        expect(getContent(subCommentContent, 'author').value[1]).toBe(dao.members[1].account.accountName);
        expect(getContent(subCommentContent, 'content').value[1]).toBe('agree!');
        expect(getContent(subCommentContent, 'edited')).toBeUndefined();
        expect(getContent(subCommentContent, 'deleted')).toBeUndefined();
        daoExpect.toHaveEdge(comment, subComment, 'comment');
        daoExpect.toHaveEdge(subComment, comment, 'commentof');

        // remove main comment
        await environment.daoContract.contract.cmntrem({
            comment_id: comment.id
        }, getAccountPermission(dao.members[0].account));
        comment = getDocumentsByType(
            environment.getDaoDocuments(),
            'comment'
        ).slice(-2, -1)[0];
        commentContent = getContentGroupByLabel(comment, 'comment');
        expect(getContent(commentContent, 'author').value[1]).toBe(dao.members[0].account.accountName);
        expect(getContent(commentContent, 'content').value[1]).toBe('This is terrible!');
        expect(getContent(commentContent, 'edited').value[1]).toBeTruthy();
        expect(getContent(commentContent, 'deleted').value[1]).toBeTruthy();
        daoExpect.toHaveEdge(commentSection, comment, 'comment');
        daoExpect.toHaveEdge(comment, commentSection, 'commentof');

        // deleting or removing again fails
        await expect(environment.daoContract.contract.cmntupd({
            new_content: 'new',
            comment_id: comment.id
        }, getAccountPermission(dao.members[0].account))).rejects.toThrow();

        await expect(environment.daoContract.contract.cmntrem({
            comment_id: comment.id
        }, getAccountPermission(dao.members[0].account))).rejects.toThrow();

        // Updating proposal doesn't affect comment section
        await environment.daoContract.contract.proposeupd({
            proposer: dao.members[0].account.accountName,
            proposal_id: proposal.id,
            content_groups: getSampleRole('new-proposal').content_groups
        });

        proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));

        // still the same
        expect(last(getDocumentsByType(
            environment.getDaoDocuments(),
            'cmnt.section'
        )).id).toBe(commentSection.id)

        // proposal <-----> comment section
        daoExpect.toHaveEdge(proposal, commentSection, 'cmntsect');
        daoExpect.toHaveEdge(commentSection, proposal, 'cmntsectof');

        // delete the proposal will delete everything...
        await environment.daoContract.contract.proposerem({
            proposer: dao.members[0].account.accountName,
            proposal_id: proposal.id
        });

        daoExpect.toNotHaveEdge(proposal, commentSection, 'cmntsect');
        daoExpect.toNotHaveEdge(commentSection, proposal, 'cmntsectof');
        expect(getDocumentsByType(environment.getDaoDocuments(), 'cmnt.section').length).toBe(0);
        expect(getDocumentsByType(environment.getDaoDocuments(), 'comment').length).toBe(0);
    });

});
