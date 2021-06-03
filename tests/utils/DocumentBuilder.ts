import { ContentGroup, ContentGroups, Document, makeAssetContent, makeContentGroup, makeInt64Content, makeNameContent, makeStringContent } from "../types/Document";

type BuilderFunction<T> = (builder: T) => void;

export class DocumentBuilder {

    private _contentGroups: ContentGroups;

    private constructor() {
        this._contentGroups = [];
    }

    public static builder(): DocumentBuilder {
        return new DocumentBuilder();
    }

    public contentGroup(builderFunction: BuilderFunction<ContentGroupBuilder>): DocumentBuilder {
        const builder = ContentGroupBuilder.builder();
        builderFunction(builder);
        this._contentGroups.push(builder.build());

        return this;
    }

    public build(): Document {
        return {
            content_groups: this._contentGroups
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
