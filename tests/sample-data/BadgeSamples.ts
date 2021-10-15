import { DocumentBuilder } from "../utils/DocumentBuilder";
import { Document } from "../types/Document"

export interface BadgeAssignmentProposal {
  title?: string
  description?: string
  start_period?: string | undefined
  period_count?: number
  badge: string
  assignee: string
}

export interface BadgeProposal {
  title?: string
  description?: string
  icon?: string
  seeds_coefficient_x10000: number,
  hypha_coefficient_x10000: number,
  hvoice_coefficient_x10000: number,
  husd_coefficient_x10000: number,
}

const masterOfPuppetsAssignment = {
  title: `Puppet's master badge assignment`, 
  description: 'Test badge assignment',
  start_period: '',
  period_count: 4,
}

const getBadgeAssignmentProposal = ({
  title = masterOfPuppetsAssignment.title,
  description = masterOfPuppetsAssignment.description,
  period_count = masterOfPuppetsAssignment.period_count,
  start_period,
  badge,
  assignee
}: BadgeAssignmentProposal): Document => DocumentBuilder
.builder()
.contentGroup(builder => {
  builder
  .groupLabel('details')
  .string('title', title)
  .string('description', description)
  .name('assignee', assignee)
  .checksum256('badge', badge)
  .int64('period_count', period_count)

  if (start_period) {
    builder.checksum256('start_period', start_period);
  }
})
.build();

const masterOfPuppets: BadgeProposal = {
  title: 'Master of Puppets',
  description: 'Maintain the stability of the puppets.',
  icon: 'https://gallery.yopriceville.com/var/resizes/Free-Clipart-Pictures/Badges-and-Labels-PNG/Gold_Badge_Transparent_PNG_Image.png',
  seeds_coefficient_x10000: 10000,
  hypha_coefficient_x10000: 9000,
  hvoice_coefficient_x10000: 11000,
  husd_coefficient_x10000: 13000,
}

const getBadgeProposal = ( 
  { title, 
    description, 
    icon, 
    seeds_coefficient_x10000,
    hypha_coefficient_x10000,
    hvoice_coefficient_x10000,
    husd_coefficient_x10000
   } : BadgeProposal = masterOfPuppets): Document => DocumentBuilder
.builder()
.contentGroup(builder => {
  builder
  .groupLabel('details')
  .string('title', title)
  .string('description', description)
  .string('icon', icon)
  .int64('seeds_coefficient_x10000', seeds_coefficient_x10000)
  .int64('hypha_coefficient_x10000', hypha_coefficient_x10000)
  .int64('hvoice_coefficient_x10000', hvoice_coefficient_x10000)
  .int64('husd_coefficient_x10000', husd_coefficient_x10000)
})
.build();

export {
  getBadgeAssignmentProposal,
  getBadgeProposal,
  masterOfPuppets,
  masterOfPuppetsAssignment
}