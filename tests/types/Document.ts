
export interface Document {
    id: string;
    hash: string;
    creator: string;
    content_groups: ContentGroups;
    certificates: Array<unknown>,
    created_date: string;
    contract: string;
}

export type ContentGroups = Array<ContentGroup>;
export type ContentGroup = Array<Content>;

export enum ContentType {
    STRING = 'string',
    ASSET = 'asset',
    NAME = 'name',
    INT64 = 'int64',
    TIME_POINT = 'time_point'
}

type ContentValueType<T,V> = [T, V];
type ContentValueStringType<T> = ContentValueType<T, string>;
type ContentValueNumberType<T> = ContentValueType<T, number>;

type ContentValueString = ContentValueStringType<ContentType.STRING>;
type ContentValueAsset = ContentValueStringType<ContentType.ASSET>;
type ContentValueName = ContentValueStringType<ContentType.NAME>;
type ContentValueTimePoint = ContentValueStringType<ContentType.TIME_POINT>;
type ContentValueInt64 = ContentValueNumberType<ContentType.INT64>;

export type ContentValue = 
// string
ContentValueString | ContentValueAsset | ContentValueName | ContentValueTimePoint
// number
| ContentValueInt64;

export interface Content {
    label: string;
    value: ContentValue;
}

const makeContent = (label: string, value: ContentValue): Content => ({
    label,
    value
});

export const CONTENT_GROUP_LABEL = 'content_group_label';
export const SYSTEM_CONTENT_GROUP_LABEL = 'system';

export const makeStringContent = (label: string, value: string): Content  => makeContent(label, [ ContentType.STRING, value ]);
export const makeAssetContent = (label: string, value: string): Content  => makeContent(label, [ ContentType.ASSET, value ]);
export const makeNameContent = (label: string, value: string): Content  => makeContent(label, [ ContentType.NAME, value ]);
export const makeTimePointContent = (label: string, value: string): Content  => makeContent(label, [ ContentType.TIME_POINT, value ]);
export const makeInt64Content = (label: string, value: number): Content  => makeContent(label, [ ContentType.INT64, value ]);

export const makeContentGroup = (groupLabel: string | undefined, ...content: ContentGroup) : ContentGroup => {
    return [
        ...(groupLabel !== undefined ? [ makeStringContent(CONTENT_GROUP_LABEL, groupLabel) ] : []), 
        ...content
    ];
};
