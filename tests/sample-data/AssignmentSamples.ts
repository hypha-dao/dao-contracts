import { DocumentBuilder } from "../utils/DocumentBuilder";
import { Document } from "../types/Document"
import { getContent, getContentGroupByLabel, getDocumentById } from "../utils/Dao";
import { DaoBlockchain } from "../dao/DaoBlockchain";

export interface AssignmentProposal {
  title?: string
  description?: string
  annual_usd_salary?: string
  start_period?: number
  period_count?: number
  time_share?: number
  deferred_perc?: number
  role: string
  assignee: string
}

const getAssignmentProposal = ({
  title = 'Underwater Basketweaver - Assignment',
  description = 'Weave baskets at the bottom of the sea',
  start_period,
  period_count = 12,
  time_share = 100,
  deferred_perc = 50,
  role,
  assignee
}: AssignmentProposal): Document => DocumentBuilder
.builder()
.contentGroup(builder => {
  builder
  .groupLabel('details')
  .string('title', title)
  .string('description', description)
  .name('assignee', assignee)
  .int64('role', role)
  .int64('period_count', period_count)
  .int64('time_share_x100', time_share)
  .int64('deferred_perc_x100', deferred_perc);

  if (start_period) {
    builder.int64('start_period', start_period);
  }
})
.build();

export const getStartPeriod = (environment: DaoBlockchain, assignment: Document): Document => {

  const assignmentDetails = getContentGroupByLabel(assignment, 'details');

  const startPeriod = getContent(assignmentDetails, 'start_period').value[1];

  return getDocumentById(environment.getDaoDocuments(), startPeriod as string);
}

export {
  getAssignmentProposal,
}
