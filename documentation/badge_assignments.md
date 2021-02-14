# Badge Assignments
## Retrieve list of active badge assignments from chain
To retrieve badge assignments directly from the blockchain, you can query the 'assignbadge' edge name, which is reserved for badge assignments.  To be sure, each record should have the from_node as the root node, but we cannot verify this in the query because we only have one index to use.

``` json
cleos -u https://test.telos.kitchen get table -l 100 --index 4 --key-type i64 -L assignbadge -U assignbadge dao.hypha dao.hypha edges
```

> NOTE: the preferred query for data will come from DGraph of course

## Badge Assignment Lifecycle
### Step 1: Propose a new badge assignment
``` json
eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha propose '
{                                                                                                          
    "proposer": "johnnyhypha1",
    "proposal_type": "assignbadge",
    "content_groups": [
        [
            {
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "title",
                "value": [
                    "string",
                    "Healer assignment"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "Assign Healer badge to johnnyhypha1"
                ]
            },
            {
                "label": "assignee",
                "value": [
                    "name",
                    "johnnyhypha1"
                ]
            },
            {
                "label": "badge",
                "value": [
                    "checksum256",
                    "f66712bc5bbcb11c00a6547e646b18b49d00e83897224982fb2d8e55e7a85256"
                ]
            },
            {
                "label": "start_period",
                "value": [
                    "int64",
                    "61"
                ]
            },
            {
                "label": "end_period",
                "value": [
                    "int64",
                    "74"
                ]
            }
        ]
    ]
}' -p johnnyhypha1
```

#### Step 1.1 - Check proposal document
To verify that the proposal document was created, retrieve the last created document (which would be the proposal created above):
``` json
cleos -u https://test.telos.kitchen get table -r -l 1 dao.hypha dao.hypha documents
{
  "rows": [{
      "id": 45,
      "hash": "132bec60bec672fb8c1a5efda9e8289348d5ff64ae7acb4c59727a9da8541ae8",
      "creator": "johnnyhypha1",
      "content_groups": [[{
            "label": "content_group_label",
            "value": [
              "string",

    ...snip...
```
The hash above (132bec60bec672fb8c1a5efda9e8289348d5ff64ae7acb4c59727a9da8541ae8) is the unique identifier for this newly created **badge assignment proposal**.  

> NOTE: your hash will be different. The document contains the ballot_id, which changes with each proposal, so the hash is always different.

Take note of the ```ballot_id``` because this is required to vote on the proposal.

#### Step 1.2 - Check proposal edges
When a proposal is created, it will create 3 edges:
1. from the DHO root node to the proposal (edge_name=proposal)
2. from the proposer to the proposal (edge_name=owns)
3. from the proposal back to the proposer (edge_name=owned_by)

You can check edges #1 and #2 by using the index 3 ("to_node") on the edges table and the proposal document hash:
``` json
cleos -u https://test.telos.kitchen get table -r -l 2 --index 3 --key-type sha256 -L 132bec60bec672fb8c1a5efda9e8289348d5ff64ae7acb4c59727a9da8541ae8 -U 132bec60bec672fb8c1a5efda9e8289348d5ff64ae7acb4c59727a9da8541ae8 dao.hypha dao.hypha edges
{
  "rows": [{
      "id": 1856858742,
      "from_node": "71836b83d367ab992b58d3704efd7e9d4d36b28e90bd89ecee82415f7ca34528",
      "to_node": "132bec60bec672fb8c1a5efda9e8289348d5ff64ae7acb4c59727a9da8541ae8",
      "edge_name": "owns",
      "created_date": "2020-10-16T16:56:42.000"
    },{
      "id": 594031207,
      "from_node": "d4ec74355830056924c83f20ffb1a22ad0c5145a96daddf6301897a092de951e",
      "to_node": "132bec60bec672fb8c1a5efda9e8289348d5ff64ae7acb4c59727a9da8541ae8",
      "edge_name": "proposal",
      "created_date": "2020-10-16T16:56:42.000"
    }
  ],
  "more": false,
  "next_key": ""
}
```

You can check edge #3 by using index #2 ("from_node") on the document hash:
``` json
❯ cleos -u https://test.telos.kitchen get table -r -l 2 --index 2 --key-type sha256 -L 132bec60bec672fb8c1a5efda9e8289348d5ff64ae7acb4c59727a9da8541ae8 -U 132bec60bec672fb8c1a5efda9e8289348d5ff64ae7acb4c59727a9da8541ae8 dao.hypha dao.hypha edges                     
{
  "rows": [{
      "id": 2467891235,
      "from_node": "132bec60bec672fb8c1a5efda9e8289348d5ff64ae7acb4c59727a9da8541ae8",
      "to_node": "71836b83d367ab992b58d3704efd7e9d4d36b28e90bd89ecee82415f7ca34528",
      "edge_name": "ownedby",
      "created_date": "2020-10-16T16:56:42.000"
    }
  ],
  "more": false,
  "next_key": ""
}
```

### Step 2: Vote on the badge proposal
Vote for the proposal to pass using the ```ballot_id``` recorded above.
```
cleos -u https://test.telos.kitchen push action trailservice castvote '["johnnyhypha1", "hypha1....15", ["pass"]]' -p johnnyhypha1
```

### Step 3: Close the proposal
Close the proposal using the ```closedocprop``` action.

> NOTE: this action is different than ```closeprop```, which is used for object proposals (to be replaced)

``` json
eosc -u https://test.telos.kitchen --vault-file ../eosc-testnet-vault.json tx create dao.hypha closedocprop '{"proposal_hash":"132bec60bec672fb8c1a5efda9e8289348d5ff64ae7acb4c59727a9da8541ae8"}' -p johnnyhypha1
```

### Step 3.1: Re-check the graph edges
After the proposal passes, the following edges will be created:
1. from the badge document to the badge assignee document (edge_name=heldby)
2. from the badge assignee to the badge (edge_name=owns)

Edges #1, pointing from the badge (document starting ```f66...```) to the assignee (```7183...```)
``` json
cleos -u https://test.telos.kitchen get table --index 2 --key-type sha256 -L f66712bc5bbcb11c00a6547e646b18b49d00e83897224982fb2d8e55e7a85256 -U f66712bc5bbcb11c00a6547e646b18b49d00e83897224982fb2d8e55e7a85256 dao.hypha dao.hypha edges 
{
  "rows": [{
      "id": 4069812146,
      "from_node": "f66712bc5bbcb11c00a6547e646b18b49d00e83897224982fb2d8e55e7a85256",
      "to_node": "71836b83d367ab992b58d3704efd7e9d4d36b28e90bd89ecee82415f7ca34528",
      "edge_name": "heldby",
      "created_date": "2020-10-16T16:58:48.500"
    }

    ...snip...
}
```

Edge #2 pointing from the badge holder (assignee) to the badge document.
``` json
❯ cleos -u https://test.telos.kitchen get table --index 2 --key-type sha256 -L f66712bc5bbcb11c00a6547e646b18b49d00e83897224982fb2d8e55e7a85256 -U f66712bc5bbcb11c00a6547e646b18b49d00e83897224982fb2d8e55e7a85256 dao.hypha dao.hypha edges                     
{
  "rows": [{
      "id": 304393774,
      "from_node": "71836b83d367ab992b58d3704efd7e9d4d36b28e90bd89ecee82415f7ca34528",
      "to_node": "f66712bc5bbcb11c00a6547e646b18b49d00e83897224982fb2d8e55e7a85256",
      "edge_name": "holdsbadge",
      "created_date": "2020-10-16T16:58:48.500"
    }

    ...snip...
}
```

# Step 4: Retrieve the badge assignment

# TODO: NEEDS UPDATING
Of course, reading from the DHO edges named ```badge```, we can then use the hash to retrieve the details of this badge.

``` json
cleos -u https://test.telos.kitchen get table -r -l 2 --index 2 --key-type sha256 -L 84df671b9d033e407630fd00575542e8ab78dd4eb525d88a90c7c9bb12339472 -U 84df671b9d033e407630fd00575542e8ab78dd4eb525d88a90c7c9bb12339472 dao.hypha dao.hypha documents
{
  "rows": [{
      "id": 44,
      "hash": "84df671b9d033e407630fd00575542e8ab78dd4eb525d88a90c7c9bb12339472",
      "creator": "johnnyhypha1",
      "content_groups": [[{
            "label": "content_group_label",
            "value": [
              "string",
              "details"
            ]
          },{
            "label": "title",
            "value": [
              "string",
              "Healer"
            ]
          },{
            "label": "description",
            "value": [
              "string",
              "Holder of indigenous wisdom ready to transfer the knowledge to others willing to receive"
            ]
          },{
            "label": "icon",
            "value": [
              "string",
              "https://assets.hypha.earth/badges/badge_explorer.png?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=minioadmin%2F20201007%2F%2Fs3%2Faws4_request&X-Amz-Date=20201007T153744Z&X-Amz-Expires=432000&X-Amz-SignedHeaders=host&X-Amz-Signature=78194bf2d97352305f9dd4f1214da5cba13e39289965143b44f31325f228992d"
            ]
          },{
            "label": "seeds_coefficient_x10000",
            "value": [
              "int64",
              10010
            ]
          },{
            "label": "hypha_coefficient_x10000",
            "value": [
              "int64",
              10015
            ]
          },{
            "label": "hvoice_coefficient_x10000",
            "value": [
              "int64",
              10000
            ]
          },{
            "label": "husd_coefficient_x10000",
            "value": [
              "int64",
              10100
            ]
          }
        ],[{
            "label": "content_group_label",
            "value": [
              "string",
              "system"
            ]
          },{
            "label": "client_version",
            "value": [
              "string",
              "1.0.13 0c81dde6"
            ]
          },{
            "label": "contract_version",
            "value": [
              "string",
              "1.0.1 366e8dfe"
            ]
          },{
            "label": "ballot_id",
            "value": [
              "name",
              "hypha1....151"
            ]
          },{
            "label": "proposer",
            "value": [
              "name",
              "johnnyhypha1"
            ]
          },{
            "label": "type",
            "value": [
              "name",
              "badge"
            ]
          }
        ]
      ],
      "certificates": [],
      "created_date": "2020-10-16T14:42:34.000"
    }
  ],
  "more": false,
  "next_key": ""
}
```

Now that the badge is created, you can move to creating a proposal to assign this badge to a member: [!badge_assignments.md]