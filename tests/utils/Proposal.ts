import { DaoBlockchain } from "../dao/DaoBlockchain";
import { last } from "./Arrays";
import { Document } from '../types/Document';
import { getContent, getContentGroupByLabel, getDocumentsByType } from "./Dao";
import { setDate } from "./Date";

export const proposeAndPass = 
async (proposal: Document, type: string, date: Date, environment: DaoBlockchain): Promise<Document> => {
  
  await environment.dao.contract.propose({
      proposer: environment.members[0].account.accountName,
      proposal_type: type,
      ...proposal
  });

  let propDoc = last(getDocumentsByType(
      environment.getDaoDocuments(),
      type
  ));

  for (let i = 0; i < environment.members.length; ++i) {
    await environment.dao.contract.vote({
      voter: environment.members[i].account.accountName,
      proposal_hash: propDoc.hash,
      vote: 'pass',
      notes: 'votes pass'
    });
  }

  const expiration = getContent(getContentGroupByLabel(propDoc, "ballot"), "expiration");

  date = setDate(environment, new Date(expiration.value[1]));

  await environment.dao.contract.closedocprop({
    proposal_hash: propDoc.hash
  }, environment.members[0].getPermissions());

  return last(getDocumentsByType(
    environment.getDaoDocuments(),
    type
  ));
}