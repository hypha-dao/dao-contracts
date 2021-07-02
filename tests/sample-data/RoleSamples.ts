import { DocumentBuilder } from "../utils/DocumentBuilder";
import { Document } from "../types/Document"

export interface RoleProposal {
  title?: string
  description?: string
  annual_usd_salary?: string
  start_period?: string | undefined
  fulltime_capacity?: number
  min_time_share?: number
  min_deferred?: number
}

const getRoleProposal = ({
  title = 'Underwater Basketweaver', 
  description = 'Weave baskets at the bottom of the sea',
  annual_usd_salary = '150000.00 USD',
  fulltime_capacity = 100,
  min_time_share = 30,
  min_deferred = 30
}: RoleProposal): Document => DocumentBuilder
.builder()
.contentGroup(builder => {
  builder
  .groupLabel('details')
  .string('title', title)
  .string('description', description)
  .asset('annual_usd_salary', annual_usd_salary)
  .int64('fulltime_capacity_x100', fulltime_capacity)
  .int64('min_time_share_x100', min_time_share)
  .int64('min_deferred_x100', min_deferred);
})
.build();

const UnderwaterBasketweaver = getRoleProposal({
  min_time_share: 50,
  min_deferred: 50, 
}); 

export {
  getRoleProposal,
  UnderwaterBasketweaver
}