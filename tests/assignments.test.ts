import { setupEnvironment } from './setup';
import { Document } from './types/Document';
import { last } from './utils/Arrays';
import { getContent, getContentGroupByLabel, getDetailsGroup, getDocumentsByType } from './utils/Dao';
import { nextDay, setDate } from './utils/Date';
import { getDaoExpect } from './utils/Expect';
import { UnderwaterBasketweaver } from './sample-data/RoleSamples';
import { DaoBlockchain } from './dao/DaoBlockchain';
import { getAssignmentProposal } from './sample-data/AssignmentSamples';
import { proposeAndPass } from './utils/Proposal';

describe('Assignments', () => {
    
    it('Create assignment', async () => {

        //TODO: Generate initial periods

        const environment = await setupEnvironment();

        const now = new Date();

        environment.setCurrentTime(now);

        let role = await proposeAndPass(UnderwaterBasketweaver, 'role', now, environment);

        const assigne = environment.members[0];

        const assignProposal = getAssignmentProposal({ role: role.hash, assignee: assigne.account.accountName });

        // let assignment = await proposeAndPass(assignProposal, 'assignment', now, environment);

        // console.log("Passed assignment:", assignment);

        // let assignmetDetails = getContentGroupByLabel(assignment, 'details');
        
        // await expect(getContent(assignmetDetails, 'state').value[1])
        //       .toBe('approved');
    });
});