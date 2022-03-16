import { setupEnvironment } from './setup';
import { Document } from './types/Document';
import { getContent, getContentGroupByLabel, getDocumentsByType, getNextPeriod } from './utils/Dao';
import { getDaoExpect } from './utils/Expect';
import { UnderwaterBasketweaver } from './sample-data/RoleSamples';
import { DaoBlockchain } from './dao/DaoBlockchain';
import { getAssignmentProposal, getStartPeriod } from './sample-data/AssignmentSamples';
import { proposeAndPass } from './utils/Proposal';
import { getBadgeAssignmentProposal, getBadgeProposal, masterOfPuppets, masterOfPuppetsAssignment } from './sample-data/BadgeSamples';
import { assetToNumber, fixDecimals, getAssetContent } from './utils/Parsers';

describe('Badges', () => {

    const getAssignmentProp = (role: Document, accountName: string, title?: string) => {
      return getAssignmentProposal({
        role: role.id,
        assignee: accountName,
        deferred_perc: 50,
        time_share: 100,
        period_count: 6,
        title
      });
    }

    it('Create badge & badge assignment', async () => {

        let badge: Document;

        //Set voting duration to 5 minutes
        const environment = await setupEnvironment({
            test: {
                votingDurationSeconds: 5 * 60
            }
        });
        const dao = environment.getDao('test');

        const hyphaUSDValue = parseFloat(fixDecimals(dao.settings.tokens.reward.toPegRatio.toFloat(), 4));

        let currentPeriod = dao.periods[0];

        environment.setCurrentTime(currentPeriod.startTime);

        let badgeProp = getBadgeProposal();

        badge = await proposeAndPass(dao, badgeProp, 'badge', environment);

        let badgeDetails = getContentGroupByLabel(badge, 'details');

        let husdMultiplier = parseInt(getContent(badgeDetails, 'husd_coefficient_x10000')?.value[1] as string);
        let hyphaMultiplier = parseInt(getContent(badgeDetails, 'hypha_coefficient_x10000')?.value[1] as string);
        let hvoiceMultiplier = parseInt(getContent(badgeDetails, 'hvoice_coefficient_x10000')?.value[1] as string);

        expect(getContent(badgeDetails, 'state').value[1])
        .toBe('approved');

        expect(husdMultiplier).toBe(masterOfPuppets.husd_coefficient_x10000);

        expect(hyphaMultiplier).toBe(masterOfPuppets.hypha_coefficient_x10000);

        expect(hvoiceMultiplier).toBe(masterOfPuppets.hvoice_coefficient_x10000);

        expect(
          parseInt(getContent(badgeDetails, 'seeds_coefficient_x10000')?.value[1] as string)
        )
        .toBe(masterOfPuppets.seeds_coefficient_x10000);

        husdMultiplier = husdMultiplier / 10000;
        hyphaMultiplier = hyphaMultiplier / 10000;
        hvoiceMultiplier = hvoiceMultiplier / 10000;

        {
          let badgeSystem = getContentGroupByLabel(badge, 'system');

          expect(
            getContent(badgeSystem, 'type').value[1]
          )
          .toBe('badge')
        }

        const assignee = dao.members[0];
        let badgeAssignProp = getBadgeAssignmentProposal({ badge: badge.id,
                                                           assignee: assignee.account.accountName,
                                                           period_count: 2 });

        let badgeAssignment = await proposeAndPass(dao, badgeAssignProp, 'assignbadge', environment);

        let badgeAssignStartPeriod: Document;
        let badgeAssingPeriodCount: number;

        {
          let badgeAssignDetails = getContentGroupByLabel(badgeAssignment, 'details');

          expect(
            getContent(badgeAssignDetails, 'state').value[1]
          )
          .toBe('approved');

          expect(
            getContent(badgeAssignDetails, 'assignee').value[1]
          )
          .toBe(assignee.account.accountName);

          expect(
            getContent(badgeAssignDetails, 'badge').value[1]
          )
          .toBe(badge.id);

          badgeAssingPeriodCount = parseInt(
            getContent(badgeAssignDetails, 'period_count').value[1] as string
          );

          expect(
            badgeAssingPeriodCount
          )
          .toBe(2);

          badgeAssignStartPeriod = getStartPeriod(environment, badgeAssignment);

          let badgeAssignSystem = getContentGroupByLabel(badgeAssignment, 'system');

          expect(
            getContent(badgeAssignSystem, 'type').value[1]
          )
          .toBe('assignbadge')
        }

        let role = await proposeAndPass(dao, UnderwaterBasketweaver, 'role', environment);

        let periodUSD = parseFloat(
          getContent(
            getContentGroupByLabel(role, 'details'),
            'annual_usd_salary'
          ).value[1] as string
        );

        const assignProposal = getAssignmentProp(role, assignee.account.accountName);

        let assignment = await proposeAndPass(dao, assignProposal, 'assignment', environment);

        let assignmentPeriodCount = parseInt(
          getContent(
            getContentGroupByLabel(assignment, 'details'),
            'period_count'
          ).value[1] as string
        )

        let assignmentStartPeriod = getStartPeriod(environment, assignment);

        //They should be the same since they
        //were voted & passed under the same period
        expect(badgeAssignStartPeriod.id).toBe(assignmentStartPeriod.id);

        //TODO: Update so it doens't relies on the assingment salary amount, since it could change due
        //global settings configuration
        {
          //let edges = environment.getDaoEdges();

          //Assignment and badge start at the second period, so let's skip the
          //next 4 periods after that (6th period)

          const assignmentDetails = getContentGroupByLabel(assignment, 'details');

          const husd = getAssetContent(assignmentDetails, 'peg_salary_per_period');

          const hvoice = getAssetContent(assignmentDetails, 'voice_salary_per_period');

          const usdSalary = getAssetContent(assignmentDetails, 'usd_salary_value_per_phase');

          const hypha = (usdSalary - husd) / hyphaUSDValue;

          currentPeriod = dao.periods[5];

          environment.setCurrentTime(currentPeriod.startTime);

          const checkPayments = async ({ husd, hypha, hvoice } : { husd: number, hypha: number, hvoice: number}) => {

            //Get payment documents (last 3)
            let docs = environment.getDaoDocuments();

            let payments: any = getDocumentsByType(docs, 'payment').slice(-3);

            payments = payments.map(
              (p: Document) => assetToNumber(
                getContent(
                  getContentGroupByLabel(p, 'details'),
                  'amount'
                ).value[1] as string
              )
            )

            let hyphaPayment, hvoicePayment, husdPayment;

            [hyphaPayment, hvoicePayment, husdPayment] = payments;

            expect(parseFloat(hyphaPayment)).toBeCloseTo(hypha, 1);
            expect(parseFloat(hvoicePayment)).toBeCloseTo(hvoice, 1);
            expect(parseFloat(husdPayment)).toBeCloseTo(husd, 1);
          };

          //
          for (let i = 0; i < badgeAssingPeriodCount; ++i) {

            await environment.daoContract.contract.claimnextper({
                assignment_id: assignment.id
            });

            checkPayments({
              hypha: hypha * hyphaMultiplier,
              husd: husd * husdMultiplier,
              hvoice: hvoice * hvoiceMultiplier
            });
          }

          //Claim the assignment once or none if the badge assignment period count is
          //greater than the assignment period count
          for (let i = 0; i < Math.min(1, assignmentPeriodCount - badgeAssingPeriodCount); ++i) {

            await environment.daoContract.contract.claimnextper({
                assignment_id: assignment.id
            });

            checkPayments({ hypha, husd, hvoice });
          }
        }
    }, 600000);
});
