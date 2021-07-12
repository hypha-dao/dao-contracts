import Account from '@klevoya/hydra/lib/main/account';
import { getAccountPermission } from '../utils/Permissions';
import { Document } from './Document';

export class Period {
    readonly startTime: Date;
    readonly label: string;
    readonly doc: Document;

    constructor(startTime: Date, label: string, doc: Document) {
        this.startTime = startTime;
        this.label = label;
        this.doc = doc;
    }
}
