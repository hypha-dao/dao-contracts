import Account from '@klevoya/hydra/lib/main/account';
import { getAccountPermission } from '../utils/Permissions';
import { Document } from './Document';

export class Member {
    readonly account: Account;
    readonly doc: Document;

    constructor(account: Account, doc: Document) {
        this.account = account;
        this.doc = doc;
    }

    getPermissions() {
        return getAccountPermission(this.account);
    }

}
