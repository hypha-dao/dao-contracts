import { ContentGroup, ContentType, CONTENT_GROUP_LABEL, Document, SYSTEM_CONTENT_GROUP_LABEL } from "../types/Document"
import { Edge } from "../types/Edge";

const TYPE_LABEL = 'type';

export const getContentGroupByLabel = (document: Document, groupLabel: string): ContentGroup | undefined => {
    return document.content_groups.find(
        group => group.find(
            content => content.label === CONTENT_GROUP_LABEL && 
            content.value[0] === ContentType.STRING && 
            content.value[1] === groupLabel
        )
    );
}

export const getSystemContentGroup = (document: Document): ContentGroup | undefined => {
    return getContentGroupByLabel(document, SYSTEM_CONTENT_GROUP_LABEL);
}

export const getDocumentsByType = (documents: Array<Document>, documentType: string): Array<Document> => {
    return documents.filter(document => {
        const system = getSystemContentGroup(document);
        if (system) {
            return system.find(
                content => content.label === TYPE_LABEL &&
                (content.value[0] === ContentType.STRING || content.value[0] === ContentType.NAME) &&
                content.value[1] === documentType
            );
        }

        return false;
    });
}

export const getDocumentByHash = (documents: Array<Document>, hash: string): Document => {
    return documents.find(document => document.hash.toUpperCase() === hash.toUpperCase());
}

type EdgeFilter = Partial<Edge>;

const keys: Array<keyof Edge> = [
    'contract', 'from_node', 'to_node', 'created_date', 'creator', 'edge_name'
];

export const getEdgesByFilter = (edges: Array<Edge>, filter: EdgeFilter): Array<Edge> => {
    return edges.filter(edge => {
        let matchOne = false;
        let matchAll = true;

        for (const key of keys) {
            if (filter[key]) {
                if (edge[key] === filter[key]) {
                    matchOne = true;
                } else {
                    matchAll = false;
                }
            }
        }

        return matchOne && matchAll;
    });
};
