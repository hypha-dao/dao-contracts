import { setupEnvironment } from './setup';
import { Document } from './types/Document';
import { last } from './utils/Arrays';
import { getContent, getContentGroupByLabel, getDetailsGroup, getDocumentsByType } from './utils/Dao';
import { nextDay, setDate } from './utils/Date';
import { getDaoExpect } from './utils/Expect';
import { UnderwaterBasketweaver } from './sample-data/RoleSamples';

describe('Roles', () => {

    const testRole = UnderwaterBasketweaver;
    let roleDocument: Document;
    
    it('Create role', async () => {

        const environment = await setupEnvironment();
        // Time doesn't move in the test unless we set the current time.
        const now = new Date();

        // We have to set the initial time if we care about any timing
        environment.setCurrentTime(now);

        // Proposing
        await environment.dao.contract.propose({
            proposer: environment.members[0].account.accountName,
            proposal_type: 'role',
            ...testRole
        });
        
        let proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));

        getDaoExpect(environment).toHaveEdge(environment.getRoot(), proposal, 'proposal');

        let proposalDetails = getDetailsGroup(proposal);

        await expect(getContent(proposalDetails, "state").value[1])
              .toBe('proposed');
        
        let date = nextDay(environment, new Date());

        console.log("About to close failed proposal");

        // closes proposal
        await environment.dao.contract.closedocprop({
          proposal_hash: proposal.hash
        }, environment.members[0].getPermissions())
      
        proposal = last(getDocumentsByType(
          environment.getDaoDocuments(),
          'role'
        ));

        getDaoExpect(environment).toHaveEdge(environment.getRoot(), proposal, 'failedprops');

        proposalDetails = getDetailsGroup(proposal);

        await expect(getContent(proposalDetails, "state").value[1])
              .toBe('rejected');

        //Propose the role again
       
        // Proposing
        await environment.dao.contract.propose({
            proposer: environment.members[0].account.accountName,
            proposal_type: 'role',
            ...testRole
        });
        
        proposal = last(getDocumentsByType(
            environment.getDaoDocuments(),
            'role'
        ));

        proposalDetails = getDetailsGroup(proposal);

        await expect(getContent(proposalDetails, "state").value[1])
              .toBe('proposed');

        await Promise.all([
          environment.dao.contract.vote({
            voter: environment.members[0].account.accountName,
            proposal_hash: proposal.hash,
            vote: 'pass',
            notes: 'votes pass'
          }),
          environment.dao.contract.vote({
            voter: environment.members[1].account.accountName,
            proposal_hash: proposal.hash,
            vote: 'pass',
            notes: 'votes pass'
          }),
          environment.dao.contract.vote({
            voter: environment.members[2].account.accountName,
            proposal_hash: proposal.hash,
            vote: 'pass',
            notes: 'votes pass'
          }),
          environment.dao.contract.vote({
            voter: environment.members[3].account.accountName,
            proposal_hash: proposal.hash,
            vote: 'pass',
            notes: 'votes pass'
          }),
          environment.dao.contract.vote({
            voter: environment.members[4].account.accountName,
            proposal_hash: proposal.hash,
            vote: 'pass',
            notes: 'votes pass'
          })
        ]);
          

        const expiration = getContent(getContentGroupByLabel(proposal, "ballot"), "expiration");
        
        date = setDate(environment, new Date(expiration.value[1]), 15);

        console.log("About to close passed proposal on:", date, 'expiration date:', expiration);

        // closes proposal
        await environment.dao.contract.closedocprop({
          proposal_hash: proposal.hash
        }, environment.members[0].getPermissions());
      
        proposal = last(getDocumentsByType(
          environment.getDaoDocuments(),
          'role'
        ));

        console.log("Proposal after closed", JSON.stringify(proposal));

        proposalDetails = getDetailsGroup(proposal);

        getDaoExpect(environment).toHaveEdge(environment.getRoot(), proposal, 'passedprops');
        getDaoExpect(environment).toHaveEdge(environment.getRoot(), proposal, 'role');

        await expect(getContent(proposalDetails, "state").value[1])
              .toBe('approved');
    });
});