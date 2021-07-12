import { setupEnvironment } from './setup';
import { Document } from './types/Document';
import { last } from './utils/Arrays';
import { getContent, getContentGroupByLabel, getDetailsGroup, getDocumentsByType } from './utils/Dao';
import { nextDay, setDate } from './utils/Date';
import { getDaoExpect } from './utils/Expect';
import { UnderwaterBasketweaver } from './sample-data/RoleSamples';
import { DocumentBuilder } from './utils/DocumentBuilder';
import { proposeAndPass } from './utils/Proposal';

const getEditProposal = (original: string,
  newTitle: string, 
  newTimeShare: number,
  newSalary: string): Document => {
    return DocumentBuilder
    .builder()
    .contentGroup(builder => {
    builder
    .groupLabel('details')
    .string('title', newTitle)
    .string('ballot_description', 'Edit role')
    .checksum256('original_document', original)
    .int64('min_time_share_x100', newTimeShare)
    .asset('annual_usd_salary', newSalary)
    })
    .build();
}

describe('Roles', () => {

    const testRole = UnderwaterBasketweaver;
    
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

        expect(getContent(proposalDetails, "state").value[1])
        .toBe('proposed');
        
        nextDay(environment, new Date());

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

        expect(getContent(proposalDetails, "state").value[1])
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

        expect(getContent(proposalDetails, "state").value[1])
              .toBe('proposed');

        for (let i = 0; i < environment.members.length; ++i) {
          await environment.dao.contract.vote({
            voter: environment.members[i].account.accountName,
            proposal_hash: proposal.hash,
            vote: 'pass',
            notes: 'votes pass'
          });
        }

        const expiration = getContent(getContentGroupByLabel(proposal, "ballot"), "expiration");
        
        setDate(environment, new Date(expiration.value[1]), 15);

        // closes proposal
        await environment.dao.contract.closedocprop({
          proposal_hash: proposal.hash
        }, environment.members[0].getPermissions());
      
        proposal = last(getDocumentsByType(
          environment.getDaoDocuments(),
          'role'
        ));

        proposalDetails = getDetailsGroup(proposal);

        getDaoExpect(environment).toHaveEdge(environment.getRoot(), proposal, 'passedprops');
        getDaoExpect(environment).toHaveEdge(environment.getRoot(), proposal, 'role');

        expect(getContent(proposalDetails, "state").value[1])
              .toBe('approved');
    });

    it('Edit role', async () => {

      const environment = await setupEnvironment();
      // Time doesn't move in the test unless we set the current time.
      const now = new Date();

      // We have to set the initial time if we care about any timing
      environment.setCurrentTime(now);

      // Proposing
      let role = await proposeAndPass(testRole, 'role', environment);

      const newRoleTitle = 'This role was edited';
      const newRoleTimeShare = 60;
      const newRoleSalary = '100000.00 USD';

      let editProposal = await proposeAndPass(
        getEditProposal(role.hash, newRoleTitle, newRoleTimeShare, newRoleSalary), 
        'edit', 
        environment
      );

      let details = getContentGroupByLabel(role, 'details');

      expect(getContent(details, 'title').value[1])
      .toBe(newRoleTitle)

      expect(getContent(details, 'min_time_share_x100').value[1])
      .toBe(newRoleTimeShare.toString())

      expect(getContent(details, 'annual_usd_salary').value[1])
      .toBe(newRoleSalary.toString())
  });
});