## Member document (aka graph node)
Each member has their own document (graph node) that simply holds their account name.
``` json
{
      "id": 7,
      "hash": "d66e7c0b1bce8b23957a25bd1691bb387d70bb7c9e1a8694d4b7ae3bf6dbdcce",
      "creator": "dao1.hypha",
      "content_groups": [[{
            "label": "content_group_label",
            "value": [
              "string",
              "details"
            ]
          },{
            "label": "member",
            "value": [
              "name",
              "mem5.hypha"
            ]
          }
        ],[{
            "label": "content_group_label",
            "value": [
              "string",
              "system"
            ]
          },{
            "label": "type",
            "value": [
              "name",
              "member"
            ]
          },{
            "label": "node_label",
            "value": [
              "string",
              "mem5.hypha"
            ]
          }
        ]
      ],
      "certificates": [],
      "created_date": "2020-12-21T20:56:46.000",
      "contract": "dao1.hypha"
    }
```

There are two graph edges created when an account becomes a member.

1. Connect the DHO to the Member (edge=member)
2. Connect the Member to the DHO (edge=memberof)

These can be found in the edges table.  This member (johnnyhypha1) is the "from_node" and the "to_node" is the dao.hypha account. 

## DHO -> Member Edge
``` json
cleos -u https://test.telos.kitchen get table -l 1 --index 3 --key-type sha256 -L 71836b83d367ab992b58d3704efd7e9d4d36b28e90bd89ecee82415f7ca34528 dao.hypha dao.hypha edges
{
  "rows": [{
      "id": 1750427846,
      "from_node": "d4ec74355830056924c83f20ffb1a22ad0c5145a96daddf6301897a092de951e",
      "to_node": "71836b83d367ab992b58d3704efd7e9d4d36b28e90bd89ecee82415f7ca34528",
      "edge_name": "member",
      "created_date": "2020-10-15T20:48:18.000"
    }
  ],
  "more": true,
  "next_key": "7463fa7dda551b9c4bbd2ba17b793931c825cefff9eede14461fd1a5c9f07d15"
}
```

## Member -> DHO Edge
``` json
cleos -u https://test.telos.kitchen get table -l 1 --index 2 --key-type sha256 -L 71836b83d367ab992b58d3704efd7e9d4d36b28e90bd89ecee82415f7ca34528 dao.hypha dao.hypha edges
{
  "rows": [{
      "id": 3726847728,
      "from_node": "71836b83d367ab992b58d3704efd7e9d4d36b28e90bd89ecee82415f7ca34528",
      "to_node": "d4ec74355830056924c83f20ffb1a22ad0c5145a96daddf6301897a092de951e",
      "edge_name": "memberof",
      "created_date": "2020-10-15T20:48:18.000"
    }
  ],
  "more": true,
  "next_key": "7463fa7dda551b9c4bbd2ba17b793931c825cefff9eede14461fd1a5c9f07d15"
}
```

## Retrieve list of all members from blockchain
You can retrieve a list of all members using this query.
> NOTE: we may use ```member``` as the edge name for Circles or other purposes, so this shows all member edges, not just top level DHO members.
``` json
cleos -u https://test.telos.kitchen get table -l 100 --index 4 --key-type i64 -L member -U member dao.hypha dao.hypha edges
{
  "rows": [{
      "id": 129102575,
      "from_node": "d4ec74355830056924c83f20ffb1a22ad0c5145a96daddf6301897a092de951e",
      "to_node": "b0d9a2292ffaf9795b4ad5c4862a1ce2b4fef9ed4642be23695ed64f4dc3428e",
      "edge_name": "member",
      "created_date": "2020-10-15T20:48:18.000"
    },{
      "id": 169937459,
      "from_node": "d4ec74355830056924c83f20ffb1a22ad0c5145a96daddf6301897a092de951e",
      "to_node": "e0a97ca26aeb20f0b3ee25dcf6555dfebb7413e4a24a50cb4b940f6b03ae2ae8",
      "edge_name": "member",
      "created_date": "2020-10-15T20:48:18.000"
    },{
      "id": 689578194,
      "from_node": "d4ec74355830056924c83f20ffb1a22ad0c5145a96daddf6301897a092de951e",
      "to_node": "26912bf1e62120d1ed5ecf2c2bd1f62b0f63317eefadef43d7f59a834e0f064d",
      "edge_name": "member",
      "created_date": "2020-10-15T20:48:18.000"
    },{
      "id": 773358451,
      "from_node": "d4ec74355830056924c83f20ffb1a22ad0c5145a96daddf6301897a092de951e",
      "to_node": "89b7a9fcbc41164db6a74950667cc5e47687b6015061506b0dc710bfe2c19141",
      "edge_name": "member",
      "created_date": "2020-10-15T20:48:18.000"
    }

    <snip>
```

## Retrieve a document by hash
##### johnnyhypha1
```
cleos -u https://test.telos.kitchen get table -l 1 --index 2 --key-type sha256 -L 71836b83d367ab992b58d3704efd7e9d4d36b28e90bd89ecee82415f7ca34528 dao.hypha dao.hypha documents
```

##### root_node
``` json
cleos -u https://test.telos.kitchen get table -l 1 --index 2 --key-type sha256 -L 0f374e7a9d8ab17f172f8c478744cdd4016497e15229616f2ffd04d8002ef64a dao1.hypha dao1.hypha documents
{
      "id": 1,
      "hash": "0f374e7a9d8ab17f172f8c478744cdd4016497e15229616f2ffd04d8002ef64a",
      "creator": "dao1.hypha",
      "content_groups": [[{
            "label": "content_group_label",
            "value": [
              "string",
              "details"
            ]
          },{
            "label": "root_node",
            "value": [
              "name",
              "dao1.hypha"
            ]
          }
        ],[{
            "label": "content_group_label",
            "value": [
              "string",
              "system"
            ]
          },{
            "label": "type",
            "value": [
              "name",
              "dho"
            ]
          },{
            "label": "node_label",
            "value": [
              "string",
              "Hypha DHO Root"
            ]
          }
        ]
      ],
      "certificates": [],
      "created_date": "2020-12-21T20:56:35.500",
      "contract": "dao1.hypha"
    }
```
