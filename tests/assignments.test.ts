import { setupEnvironment } from './setup';
import { Document } from './types/Document';
import { last } from './utils/Arrays';
import { getContent, getContentGroupByLabel, getDetailsGroup, getDocumentByHash, getDocumentsByType, getEdgesByFilter } from './utils/Dao';
import { getDaoExpect } from './utils/Expect';
import { UnderwaterBasketweaver } from './sample-data/RoleSamples';
import { DaoBlockchain } from './dao/DaoBlockchain';
import { getAssignmentProposal } from './sample-data/AssignmentSamples';
import { passProposal, proposeAndPass } from './utils/Proposal';
import { assetToNumber } from './utils/Parsers';
import { PHASE_TO_YEAR_RATIO } from './utils/Constants';
import { DocumentBuilder } from './utils/DocumentBuilder';
import { setDate } from './utils/Date';
import { getAccountPermission } from './utils/Permissions';

const getStartPeriod = (environment: DaoBlockchain, assignment: Document): Document => {

    const assignmentDetails = getContentGroupByLabel(assignment, 'details');

    const startPeriod = getContent(assignmentDetails, 'start_period').value[1];

    return getDocumentByHash(environment.getDaoDocuments(), startPeriod as string);
}

const getPeriodStartDate = (period: Document): Date => {

    const details = getContentGroupByLabel(period, 'details');

    const startDate = getContent(details, 'start_time');

    return new Date(startDate.value[1]);
}

const getEditProposal = (original: string,
                         newTitle: string, 
                         newPeriods: number): Document => {
    return DocumentBuilder
    .builder()
    .contentGroup(builder => {
      builder
      .groupLabel('details')
      .string('title', newTitle)
      .string('ballot_description', 'Edit assignment')
      .checksum256('original_document', original)
      .int64('period_count', newPeriods)
    })
    .build();
}

const claimRemainingPeriods = async (assignment: Document,
                                     environment: DaoBlockchain): Promise<number> => {

    let claimedPeriods = 0;
    let edges = environment.getDaoEdges();
    let docs = environment.getDaoDocuments();

    let currentPeriod = getStartPeriod(environment, assignment);

    while (true) {

        let nextEdge = getEdgesByFilter(edges, { from_node: currentPeriod.hash, edge_name: 'next' });

        expect(nextEdge).toHaveLength(1);

        const nextPeriod = getDocumentByHash(docs, nextEdge[0].to_node);

        let now = getPeriodStartDate(nextPeriod);

        setDate(environment, now, 0);

        try {
          await environment.dao.contract.claimnextper({
            assignment_hash: assignment.hash
          });
        }
        catch(error) {
          if (error.toString().includes('All available periods for this assignment have been claimed:')) {
            break;
          }
          else {
            throw error;
          }
        }

        getDaoExpect(environment).toHaveEdge(assignment, currentPeriod, 'claimed');
        
        currentPeriod = nextPeriod;
    }

    return claimedPeriods;
}

describe('Assignments', () => {

    const getAssignmentProp = (role: Document, accountName: string) => {
      return getAssignmentProposal({ 
        role: role.hash, 
        assignee: accountName,
        deferred_perc: 50,
        time_share: 100,
        period_count: 3,
      });
    }

    it('Create assignment', async () => {            

        let assignment: Document;
      
        const environment = await setupEnvironment();

        let now = new Date();

        environment.setCurrentTime(now);

        let role = await proposeAndPass(UnderwaterBasketweaver, 'role', environment);

        const assignee = environment.members[0];

        const assignProposal = getAssignmentProp(role,  assignee.account.accountName);
        
        assignment = await proposeAndPass(assignProposal, 'assignment', environment);

        let assignmentDetails = getContentGroupByLabel(assignment, 'details');

        expect(getContent(assignmentDetails, 'state').value[1])
        .toBe('approved');

        let annualSalary = getContent(getContentGroupByLabel(role, 'details'), 'annual_usd_salary')?.value[1];

        let usdSalaryPerPhase = assetToNumber(annualSalary as string) * PHASE_TO_YEAR_RATIO;

        expect(
          assetToNumber(
            getContent(assignmentDetails, 'usd_salary_value_per_phase')?.value[1] as string
          )
        )
        .toBeCloseTo(usdSalaryPerPhase);

        //0.5 Since we are using 50% commitment
        expect(
          assetToNumber(
            getContent(assignmentDetails, 'husd_salary_per_phase')?.value[1] as string
          )
        )
        .toBeCloseTo(usdSalaryPerPhase * 0.5, 1);

        expect(
          assetToNumber(
            getContent(assignmentDetails, 'hypha_salary_per_phase')?.value[1] as string
          )
        )
        .toBeDefined();

        expect(
          assetToNumber(
            getContent(assignmentDetails, 'hvoice_salary_per_phase')?.value[1] as string
          )
        )
        .toBeCloseTo(usdSalaryPerPhase * 2, 1);

        expect(getContent(assignmentDetails, 'period_count')?.value[1])
        .toBe("3");

        expect(getContent(getContentGroupByLabel(assignment, 'system'), 'type')?.value[1])
        .toBe("assignment");

        const startPeriod = getContent(assignmentDetails, 'start_period')?.value[1];

        expect(startPeriod).toBeDefined();

        const docs = environment.getDaoDocuments();

        getDaoExpect(environment).toHaveEdge(assignment, getDocumentByHash(docs, startPeriod as string), 'start');

        getDaoExpect(environment).toHaveEdge(assignment, role, 'role');

        getDaoExpect(environment).toHaveEdge(role, assignment, 'assignment');

        getDaoExpect(environment).toHaveEdge(assignee.doc, assignment, 'assigned');
        
        getDaoExpect(environment).toHaveEdge(assignment, assignee.doc, 'assignee');

        const timeShareDocs = getDocumentsByType(docs, 'timeshare');

        expect(timeShareDocs).toHaveLength(1);

        const assignmentTimeShare = timeShareDocs[0];

        getDaoExpect(environment).toHaveEdge(assignment, assignmentTimeShare, 'initimeshare');

        getDaoExpect(environment).toHaveEdge(assignment, assignmentTimeShare, 'lastimeshare');
        
        getDaoExpect(environment).toHaveEdge(assignment, assignmentTimeShare, 'curtimeshare');
    });

    it('Claim assignment periods', async () => {            
      
        let assignment: Document;
        
        const environment = await setupEnvironment();

        let now = new Date(environment.periods[0].startTime);

        setDate(environment, now, 0);

        let role = await proposeAndPass(UnderwaterBasketweaver, 'role', environment);

        const assignee = environment.members[0];

        const assignProposal = getAssignmentProp(role, assignee.account.accountName);

        assignment = await proposeAndPass(assignProposal, 'assignment', environment);

        let currentPeriod = getStartPeriod(environment, assignment);

        const assignmentDetails = getContentGroupByLabel(assignment, 'details');

        const periodCount = 
        parseInt(
          getContent(assignmentDetails, 'period_count').value[1] as string
        );

        let edges = environment.getDaoEdges();
        let docs = environment.getDaoDocuments();

        for (let i = 0; i < periodCount; ++i) {

            let nextEdge = getEdgesByFilter(edges, { from_node: currentPeriod.hash, edge_name: 'next' });

            expect(nextEdge).toHaveLength(1);

            const nextPeriod = getDocumentByHash(docs, nextEdge[0].to_node);

            now = getPeriodStartDate(nextPeriod);

            setDate(environment, now, 0);

            await environment.dao.contract.claimnextper({
              assignment_hash: assignment.hash
            });

            getDaoExpect(environment).toHaveEdge(assignment, currentPeriod, 'claimed');
            
            currentPeriod = nextPeriod;
        }
    });

    it('Edit assignment', async () => {            
      
        let assignment: Document;
        
        const environment = await setupEnvironment();

        let now = new Date();

        environment.setCurrentTime(now);

        let role = await proposeAndPass(UnderwaterBasketweaver, 'role', environment);

        const assignee = environment.members[0];

        const assignProposal = getAssignmentProp(role, assignee.account.accountName);

        assignment = await proposeAndPass(assignProposal, 'assignment', environment);

        const editProp = getEditProposal(assignment.hash, 'Edited Title', 6);

        await proposeAndPass(editProp, 'edit', environment);

        let editedAssignment = last(
          getDocumentsByType(environment.getDaoDocuments(), 'assignment')
        );

        const details = getContentGroupByLabel(editedAssignment, 'details');

        //Original assignment shouldn't exist anymore
        expect(getDocumentByHash(environment.getDaoDocuments(), assignment.hash))
        .toBeUndefined();
        
        expect(getContent(details, 'title').value[1])
        .toBe('Edited Title');
          
        expect(getContent(details, 'period_count').value[1])
        .toBe('6');
    });

    it('Suspend  assignment', async () => {            
        
        let assignment: Document;
        
        const environment = await setupEnvironment();

        let now = new Date();

        environment.setCurrentTime(now);

        let role = await proposeAndPass(UnderwaterBasketweaver, 'role', environment);

        const assignee = environment.members[0];

        const suspender = environment.members[1];

        const assignProposal = getAssignmentProp(role, assignee.account.accountName);

        assignment = await proposeAndPass(assignProposal, 'assignment', environment);

        const suspendReason = 'Testing porpouses';
        
        await environment.dao.contract.suspend({
          proposer: suspender.account.accountName,
          hash: assignment.hash,
          reason: suspendReason
        }, getAccountPermission(suspender.account));

        let suspendProp = last(
          getDocumentsByType(environment.getDaoDocuments(), 'suspend')
        );

        const suspendDetails = getContentGroupByLabel(suspendProp, 'details');

        expect(getContent(suspendDetails, 'description').value[1])
        .toBe(suspendReason);

        expect(getContent(suspendDetails, 'original_document').value[1])
        .toBe(assignment.hash);

        getDaoExpect(environment).toHaveEdge(suspendProp, assignment, 'suspend');

        suspendProp = await passProposal(suspendProp, 'suspend', environment);

        let suspendedAssignment = last(
          getDocumentsByType(environment.getDaoDocuments(), 'assignment')
        );

        const details = getContentGroupByLabel(suspendedAssignment, 'details');

        expect(getContent(details, 'state').value[1])
        .toBe('suspended');

        getDaoExpect(environment)
        .toHaveEdge(environment.getRoot(), suspendedAssignment, 'suspended');
    });

    it('Withdraw assignment', async () => {            
        
        let assignment: Document;
        
        const environment = await setupEnvironment();

        let now = new Date();

        environment.setCurrentTime(now);

        let role = await proposeAndPass(UnderwaterBasketweaver, 'role', environment);

        const assignee = environment.members[0];

        const assignProposal = getAssignmentProp(role, assignee.account.accountName);

        assignment = await proposeAndPass(assignProposal, 'assignment', environment);
        
        await environment.dao.contract.withdraw({
          owner: assignee.account.accountName,
          hash: assignment.hash
        }, getAccountPermission(assignee.account));

        let withdrawedAssignment = last(
          getDocumentsByType(environment.getDaoDocuments(), 'assignment')
        );

        const details = getContentGroupByLabel(withdrawedAssignment, 'details');

        expect(getContent(details, 'state').value[1])
        .toBe('withdrawed');
    });
});