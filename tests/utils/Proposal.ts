import { DaoBlockchain } from "../dao/DaoBlockchain";
import { last } from "./Arrays";
import { Document } from '../types/Document';
import { getContent, getContentGroupByLabel, getDocumentsByType } from "./Dao";
import { setDate, fromUTC } from "./Date";

export const closeProposal =
async (proposal: Document, type: string, environment: DaoBlockchain): Promise<Document> => {
  const expiration = getContent(getContentGroupByLabel(proposal, "ballot"), "expiration");

  //Expiration comes in UTC so we have to convert it to the local machite's timezone
  let next = fromUTC(new Date(expiration.value[1]));
  //Add 1 second 
  next.setSeconds(next.getSeconds() + 1);
  setDate(environment, next);

  await environment.dao.contract.closedocprop({
    proposal_hash: proposal.hash
  }, environment.members[0].getPermissions());

  return last(getDocumentsByType(
    environment.getDaoDocuments(),
    type
  ));
} 

export const passProposal = 
async (proposal: Document, type: string, environment: DaoBlockchain): Promise<Document> => {

  const voteTx = environment.members.map(m => environment.buildAction(
    environment.dao,
    'vote',
    {
      voter: m.account.accountName,
      proposal_hash: proposal.hash,
      vote: 'pass',
      notes: 'votes pass'
    }
  ));

  await environment.sendTransaction({
    actions: voteTx
  });

  return await closeProposal(proposal, type, environment);
  
}

export const createProposal = 
async (proposal: Document, type: string, environment: DaoBlockchain): Promise<Document> => {
  
    await environment.dao.contract.propose({
        proposer: environment.members[0].account.accountName,
        proposal_type: type,
        ...proposal
    });

    let propDoc = last(getDocumentsByType(
        environment.getDaoDocuments(),
        type
    ));

    return propDoc;
}

export const proposeAndPass = 
async (proposal: Document, type: string, environment: DaoBlockchain): Promise<Document> => {
  
    await environment.dao.contract.propose({
        proposer: environment.members[0].account.accountName,
        proposal_type: type,
        publish: true,
        ...proposal
    });

    let propDoc = last(getDocumentsByType(
        environment.getDaoDocuments(),
        type
    ));

    return await passProposal(propDoc, type, environment);
}
