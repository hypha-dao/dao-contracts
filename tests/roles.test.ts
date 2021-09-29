import { setupEnvironment } from './setup';
import { Document } from './types/Document';
import { last } from './utils/Arrays';
import { getContent, getContentGroupByLabel, getDetailsGroup, getDocumentsByType } from './utils/Dao';
import { nextDay, setDate } from './utils/Date';
import { getDaoExpect } from './utils/Expect';
import { UnderwaterBasketweaver } from './sample-data/RoleSamples';
import { DocumentBuilder } from './utils/DocumentBuilder';
import { closeProposal, passProposal, proposeAndPass } from './utils/Proposal';
import { getAccountPermission } from './utils/Permissions';
import { getAssignmentProposal } from './sample-data/AssignmentSamples';

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

        const environment = await setupEnvironment({ periodCount: 2 });

        const now = new Date();

        environment.setCurrentTime(now);

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
        
        proposal = await closeProposal(proposal, 'role', environment);
    
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

        proposal = await passProposal(proposal, 'role', environment);

        proposalDetails = getDetailsGroup(proposal);

        getDaoExpect(environment).toHaveEdge(environment.getRoot(), proposal, 'passedprops');
        getDaoExpect(environment).toHaveEdge(environment.getRoot(), proposal, 'role');

        expect(getContent(proposalDetails, "state").value[1])
              .toBe('approved');
    });

    it('Edit role', async () => {

        const environment = await setupEnvironment({ periodCount: 2 });
        // Time doesn't move in the test unless we set the current time.
        const now = new Date();

        // We have to set the initial time if we care about any timing
        environment.setCurrentTime(now);

        // Proposing
        let role = await proposeAndPass(testRole, 'role', environment);

        const newRoleTitle = 'This role was edited';
        const newRoleTimeShare = 60;
        const newRoleSalary = '100000.00 USD';

        await proposeAndPass(
          getEditProposal(role.hash, newRoleTitle, newRoleTimeShare, newRoleSalary), 
          'edit', 
          environment
        );

        role = last(
          getDocumentsByType(environment.getDaoDocuments(), 'role')
        );

        let details = getContentGroupByLabel(role, 'details');

        expect(getContent(details, 'title').value[1])
        .toBe(newRoleTitle)

        expect(getContent(details, 'min_time_share_x100').value[1])
        .toBe(newRoleTimeShare.toString())

        expect(getContent(details, 'annual_usd_salary').value[1])
        .toBe(newRoleSalary.toString())
    });

    it('Suspend role', async () => {

      //TODO: Check why this test is hanging up
      return;

      const environment = await setupEnvironment({ periodCount: 3 });
      
      const now = new Date();

      environment.setCurrentTime(now);

      let role = await proposeAndPass(testRole, 'role', environment);

      let suspender = environment.members[1];
      
      const suspendReason = 'Testing porpouses';
        
      await environment.dao.contract.suspend({
        proposer: suspender.account.accountName,
        hash: role.hash,
        reason: suspendReason
      }, getAccountPermission(suspender.account));

      let suspendProp = last(
        getDocumentsByType(environment.getDaoDocuments(), 'suspend')
      );

      const suspendDetails = getContentGroupByLabel(suspendProp, 'details');

      expect(getContent(suspendDetails, 'description').value[1])
      .toBe(suspendReason);

      expect(getContent(suspendDetails, 'original_document').value[1])
      .toBe(role.hash);

      getDaoExpect(environment).toHaveEdge(suspendProp, role, 'suspend');

      suspendProp = await passProposal(suspendProp, 'suspend', environment);

      let suspendedRole = last(
        getDocumentsByType(environment.getDaoDocuments(), 'role')
      );

      const details = getContentGroupByLabel(suspendedRole, 'details');

      expect(getContent(details, 'state').value[1])
      .toBe('suspended');

      getDaoExpect(environment)
      .toHaveEdge(environment.getRoot(), suspendedRole, 'suspended');

      //We shouldn't be able to create assignment proposal of suspended roles
      const assignee = environment.members[1].account;

      const assignmentProp = getAssignmentProposal({
        assignee: assignee.accountName, 
        role: suspendedRole.hash
      });

      await expect(environment.dao.contract.propose({
        proposer: assignee.accountName,
        proposal_type: 'assignment',
        ...assignmentProp
      })).rejects.toThrow('Cannot create assignment proposal of suspened role');
  });
});