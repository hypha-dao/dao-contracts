import Account from '@klevoya/hydra/lib/main/account';
import { getAccountPermission } from '../utils/Permissions';
import { Document } from './Document';

export class Period {
    readonly startTime: Date;
    readonly label: string;
    readonly doc: Document;

    constructor(doc: Document) {
        this.doc = doc;
        this.startTime = new Date(doc.content_groups[0][1].value[1]);
        this.label = doc.content_groups[0][2].value[1] as string;
    }
}
