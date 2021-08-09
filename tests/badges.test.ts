import { setupEnvironment } from './setup';
import { Document } from './types/Document';
import { last } from './utils/Arrays';
import { getContent, getContentGroupByLabel, getDetailsGroup, getDocumentByHash, getDocumentsByType, getEdgesByFilter } from './utils/Dao';
import { getDaoExpect } from './utils/Expect';
import { UnderwaterBasketweaver } from './sample-data/RoleSamples';
import { DaoBlockchain } from './dao/DaoBlockchain';
import { getAssignmentProposal } from './sample-data/AssignmentSamples';
import { passProposal, proposeAndPass } from './utils/Proposal';
import { PHASE_TO_YEAR_RATIO } from './utils/Constants';
import { DocumentBuilder } from './utils/DocumentBuilder';
import { setDate } from './utils/Date';
import { getAccountPermission } from './utils/Permissions';
import { Asset } from './types/Asset';
import { getBadgeAssignmentProposal, getBadgeProposal, masterOfPuppets, masterOfPuppetsAssignment } from './sample-data/BadgeSamples';
import { getStartPeriod } from './assignments.test';

describe('Badges', () => {

    const getAssignmentProp = (role: Document, accountName: string, title?: string) => {
      return getAssignmentProposal({ 
        role: role.hash, 
        assignee: accountName,
        deferred_perc: 50,
        time_share: 100,
        period_count: 3,
        title
      });
    }

    it('Create badge & badge assignment', async () => {            

        let badge: Document;
      
        const environment = await setupEnvironment({votingDurationSeconds: 60 * 5});

        let now = environment.periods[0].startTime;

        environment.setCurrentTime(now);

        let badgeProp = getBadgeProposal();
        
        badge = await proposeAndPass(badgeProp, 'badge', environment);

        let badgeDetails = getContentGroupByLabel(badge, 'details');

        expect(getContent(badgeDetails, 'state').value[1])
        .toBe('approved');

        expect(
          getContent(badgeDetails, 'husd_coefficient_x10000')?.value[1] as string
        )
        .toBe(masterOfPuppets.husd_coefficient_x10000);

        expect(
          getContent(badgeDetails, 'hypha_coefficient_x10000')?.value[1] as string
        )
        .toBe(masterOfPuppets.hypha_coefficient_x10000);

        expect(
          getContent(badgeDetails, 'hvoice_coefficient_x10000')?.value[1] as string
        )
        .toBe(masterOfPuppets.hvoice_coefficient_x10000);

        expect(
          getContent(badgeDetails, 'seeds_coefficient_x10000')?.value[1] as string
        )
        .toBe(masterOfPuppets.seeds_coefficient_x10000);

        {
          let badgeSystem = getContentGroupByLabel(badge, 'system');

          expect(
            getContent(badgeSystem, 'type').value[1]
          )
          .toBe('badge')
        }

        const assignee = environment.members[0];

        let badgeAssignProp = getBadgeAssignmentProposal({ badge: badge.hash, assignee: assignee.doc.hash });

        let badgeAssignment = await proposeAndPass(badgeAssignProp, 'assignbadge', environment);

        let badgeAssignStartPeriod;

        {
          let badgeAssignDetails = getContentGroupByLabel(badgeAssignment, 'details');

          expect(
            getContent(badgeAssignDetails, 'state').value[1]
          )
          .toBe('approved');

          expect(
            getContent(badgeAssignDetails, 'assignee').value[1]
          )
          .toBe(assignee.doc.hash);

          expect(
            getContent(badgeAssignDetails, 'badge').value[1]
          )
          .toBe(badge.hash);

          expect(
            getContent(badgeAssignDetails, 'period_count').value[1]
          )
          .toBe(masterOfPuppetsAssignment.period_count.toString());

          badgeAssignStartPeriod = getContent(badgeAssignDetails, 'start_period').value[1];

          let badgeAssignSystem = getContentGroupByLabel(badgeAssignment, 'system');

          expect(
            getContent(badgeAssignSystem, 'type').value[1]
          )
          .toBe('assignbadge')
        }

        let role = await proposeAndPass(UnderwaterBasketweaver, 'role', environment);

        const assignProposal = getAssignmentProp(role, assignee.account.accountName);

        let assignment = await proposeAndPass(assignProposal, 'assignment', environment);

        let assignmentStartPeriod = getStartPeriod(environment, assignment);

        //They should be the same since they
        //were voted & passed under the same period
        expect(badgeAssignStartPeriod).toBe(assignmentStartPeriod);
    });
});