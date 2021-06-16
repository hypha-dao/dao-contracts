import { ContentGroup, ContentGroups, Document, makeAssetContent, makeContentGroup, makeInt64Content, makeNameContent, makeStringContent, makeTimePointContent } from "../types/Document";

type BuilderFunction<T> = (builder: T) => void;

export class DocumentBuilder {

    private _contentGroups: ContentGroups;
    private _id: string;
    private _hash: string;
    private _creator: string;
    private _certificates: Array<unknown>;
    private _created_date: string;
    private _contract: string;

    private constructor() {
        this._contentGroups = [];
        this._id = '';
        this._hash = '';
        this._creator = '';
        this._certificates = [];
        this._created_date = '';
        this._contract = '';
    }

    public static builder(): DocumentBuilder {
        return new DocumentBuilder();
    }

    public id(id: string): DocumentBuilder {
        this._id = id;
        return this;
    }

    public hash(hash: string): DocumentBuilder {
        this._hash = hash;
        return this;
    }

    public contract(contract: string): DocumentBuilder {
        this._contract = contract;
        return this;
    }

    public contentGroup(builderFunction: BuilderFunction<ContentGroupBuilder>): DocumentBuilder {
        const builder = ContentGroupBuilder.builder();
        builderFunction(builder);
        this._contentGroups.push(builder.build());

        return this;
    }

    public created_date(created_date: string): DocumentBuilder {
        this._created_date = created_date;
        return this;
    }

    public creator(creator: string): DocumentBuilder {
        this._creator = creator;
        return this;
    }

    public build(): Document {
        return {
            id: this._id,
            hash: this._hash,
            creator: this._creator,
            content_groups: this._contentGroups,
            certificates: this._certificates,
            created_date: this._created_date,
            contract: this._contract
        };
    }

}

class ContentGroupBuilder {

    private _contentGroup: ContentGroup;
    private _groupLabel?: string;

    private constructor() {
        this._contentGroup = [];
    }

    public static builder(): ContentGroupBuilder {
        return new ContentGroupBuilder();
    }

    public groupLabel(groupLabel: string): ContentGroupBuilder {
        this._groupLabel = groupLabel;
        return this;
    }

    public string(label: string, value: string): ContentGroupBuilder {
        this._contentGroup.push(
            makeStringContent(label, value)
        );
        return this;
    }

    public asset(label: string, value: string): ContentGroupBuilder {
        this._contentGroup.push(
            makeAssetContent(label, value)
        );
        return this;
    }

    public name(label: string, value: string): ContentGroupBuilder {
        this._contentGroup.push(
            makeNameContent(label, value)
        );
        return this;
    }

    public timePoint(label: string, value: string): ContentGroupBuilder {
        this._contentGroup.push(
            makeTimePointContent(label, value)
        );
        return this;
    }

    public int64(label: string, value: number): ContentGroupBuilder {
        this._contentGroup.push(
            makeInt64Content(label, value)
        );
        return this;
    }

    public build() {
        return makeContentGroup(this._groupLabel, ...this._contentGroup);
    }

}
