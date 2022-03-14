import { DaoBlockchain } from "../dao/DaoBlockchain";
import { last } from "./Arrays";
import { Document } from '../types/Document';
import { getContent, getContentGroupByLabel, getDocumentsByType } from "./Dao";
import { setDate, fromUTC } from "./Date";
import {Dao} from "../dao/Dao";

export const closeProposal =
async (dao: Dao, proposal: Document, type: string, environment: DaoBlockchain): Promise<Document> => {
  const expiration = getContent(getContentGroupByLabel(proposal, "ballot"), "expiration");

  //Expiration comes in UTC so we have to convert it to the local machite's timezone
  let next = fromUTC(new Date(expiration.value[1]));
  //Add 1 second
  next.setSeconds(next.getSeconds() + 1);
  setDate(environment, next);

  await environment.daoContract.contract.closedocprop({
    proposal_id: proposal.id
  }, dao.members[0].getPermissions());

  return last(getDocumentsByType(
    environment.getDaoDocuments(),
    type
  ));
}

export const passProposal =
async (dao: Dao, proposal: Document, type: string, environment: DaoBlockchain): Promise<Document> => {

    await environment.sendTransaction({
        actions: dao.members.map(m => environment.buildAction(environment.daoContract, 'vote', {
            voter: m.account.accountName,
            proposal_id: proposal.id,
            vote: 'pass',
            notes: 'votes pass'
        }))
    })

  return await closeProposal(dao, proposal, type, environment);
}

export const createProposal =
async (dao: Dao, proposal: Document, type: string, environment: DaoBlockchain): Promise<Document> => {

    await environment.daoContract.contract.propose({
        dao_id: dao.getId(),
        proposer: dao.members[0].account.accountName,
        proposal_type: type,
        content_groups: proposal.content_groups
    });

    let propDoc = last(getDocumentsByType(
        environment.getDaoDocuments(),
        type
    ));

    return propDoc;
}

export const proposeAndPass =
async (dao: Dao, proposal: Document, type: string, environment: DaoBlockchain): Promise<Document> => {

    await environment.daoContract.contract.propose({
        dao_id: dao.getId(),
        proposer: dao.members[0].account.accountName,
        proposal_type: type,
        publish: true,
        content_groups: proposal.content_groups
    });

    let propDoc = last(getDocumentsByType(
        environment.getDaoDocuments(),
        type
    ));

    return await passProposal(dao, propDoc, type, environment);
}
