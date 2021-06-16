import { DaoBlockchain } from "../DaoBlockchain";
import { Document } from "../types/Document";
import { last } from "./Arrays";
import { getEdgesByFilter } from "./Dao";

export class Expect {

    readonly dao: DaoBlockchain;

    constructor(dao: DaoBlockchain) {
        this.dao = dao;
    }

    toHaveEdge(from: Document, to: Document, edgeName: string) {
        const edges = this.dao.getDaoEdges();
        const filter = {
            edge_name: edgeName,
            from_node: from.hash,
            to_node: to.hash
        };
        const filtered = getEdgesByFilter(edges, filter);
        if (filtered.length === 0) {
            // Ok so we got an error. Lets make it easier to the test implementers to find out what's wrong
            // Spit the last 5 of each combination (from, to), (from, name), (to, name)
            const fromTo = getEdgesByFilter(edges, { from_node: from.hash, to_node: to.hash });
            const fromName = getEdgesByFilter(edges, { from_node: from.hash, edge_name: edgeName });
            const toName = getEdgesByFilter(edges, { to_node: to.hash, edge_name: edgeName });

            const message: Array<string> = [];
            message.push(`${filter.from_node} ---${filter.edge_name}---> ${filter.to_node} not found.`);
            if (fromTo.length > 0 || fromName.length > 0 || toName.length > 0) {
                message.push('Showing similar combinations:');
                if (fromTo.length > 0) {
                    message.push(`${filter.from_node} ---?---> ${filter.to_node}`);
                    message.push(JSON.stringify(fromTo, null, 2));
                }
                if (fromName.length > 0) {
                    message.push(`${filter.from_node} ---${filter.edge_name}---> ?`);
                    message.push(JSON.stringify(fromName, null, 2));
                }
                if (toName.length > 0) {
                    message.push(`? ---${filter.edge_name}---> ${filter.to_node}`);
                    message.push(JSON.stringify(toName, null, 2));
                }
            } else {
                message.push('No valid combinations found');
            }
            
            throw new Error(message.join('\n'));
        }
        expect(filtered.length).toEqual(1);
        expect(filtered[0]).toMatchObject(filter);
    }
}

// This is a cheap version of the actual work we need to do to have a custom matcher.
// Custom matchers are more user friendly but they are more work.
export const getDaoExpect = (dao: DaoBlockchain) => new Expect(dao);
