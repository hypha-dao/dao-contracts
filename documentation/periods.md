# Time Periods

Each DHO can have its own document for time periods. The format is below. There's a content_group for each time period. Content groups are sequenced in C++. Although they are not sequenced in the DGraph query results, the index number is included as an explicit item in the content group and content item data.

There is one document per time period.  In the graph, the time period will link and be linked to where appropriate, such as for payments and assignments.

There is also a need to know the sequence of periods. For example, when determining if 


The graph spec is:
```
    dho/root_node   ---->   periods     ---->
```

``` yaml
{
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
                "label": "period_label",
                "value": [
                    "string",
                    "Full Moon"
                ]
            },
            {
                "label": "start_time",
                "value": [
                    "time_point",
                    "2020-12-05T11:41:00.750"
                ]
            },
            {
                "label": "end_time",
                "value": [
                    "time_point",
                    "2020-12-12T19:51:21.750"
                ]
            }
        ]
    ]
}
```

##### Passed attestations
When the proposal is passed, there is now a graph edge from the DHO root node to the attestation document.

```
root_node       ----- attestation ----->    attestation document
```

#### Edit attestation
Now, let's propose an edit to the above attestation.  Let's decrease the threshold from 50 to 25 million Seeds. 

> NOTE: we are using ```ballot_title``` and ```ballot_description``` so that we provide descriptors to put on the ballot and proposal card, but they will not overwrite the original title and description.

``` yaml
{
    "content_groups": [
        [{
                "label": "content_group_label",
                "value": [
                    "string",
                    "details"
                ]
            },
            {
                "label": "original_document",
                "value": [
                    "checksum256",
                    "3f84d079a9a002b36c7f02e50ca8f45629533eccebc804de0d8d1d072608aa19"
                ]
            },
            {
                "label": "ballot_title",
                "value": [
                    "string",
                    "Update hardware threshold"
                ]
            },
            {
                "label": "ballot_description",
                "value": [
                    "string",
                    "This proposal updates the hardware requirement threshold from 50 million to 25 million"
                ]
            },
            {
                "label": "clause_hardware",
                "value": [
                    "string",
                    "Hardware wallets should be used for any treasury over the value of 25 million Seeds"
                ]
            },
            {
                "label": "clause_hardware_threshold",
                "value": [
                    "asset",
                    "25000000.0000 SEEDS"
                ]
            }
        ]
    ]
}
```

### Post Edit
After the Edit proposal is passed, the resulting document will be the merged content and all existing graph edges will be updated to point to the latest document.

``` yaml
{
    "content_groups": [
        [{
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
                    "Security specs for Hypha treasuries"
                ]
            },
            {
                "label": "description",
                "value": [
                    "string",
                    "This proposal specifies some key policies for Hypha treasuries"
                ]
            },
            {
                "label": "clause_multisig",
                "value": [
                    "string",
                    "Hypha treasuries should all use at least 3 of 5 multisig"
                ]
            },
            {
                "label": "original_document",
                "value": [
                    "checksum256",
                    "3f84d079a9a002b36c7f02e50ca8f45629533eccebc804de0d8d1d072608aa19"
                ]
            },
            {
                "label": "ballot_title",
                "value": [
                    "string",
                    "Update hardware threshold"
                ]
            },
            {
                "label": "ballot_description",
                "value": [
                    "string",
                    "This proposal updates the hardware requirement threshold from 50 million to 25 million"
                ]
            },
            {
                "label": "clause_hardware",
                "value": [
                    "string",
                    "Hardware wallets should be used for any treasury over the value of 25 million Seeds"
                ]
            },
            {
                "label": "clause_hardware_threshold",
                "value": [
                    "asset",
                    "25000000.0000 SEEDS"
                ]
            }
        ]
    ]
}
```

> TODO: Still need to review how the 'version tracking' capabilities should work?  All changes are in the blockchain log, albeit buried.  Should we provide the option to maintain historical versions on-chain?  Could be a setting on the edit proposal, perhaps?  During the merge, should we remove the ```original_document``` content item?  E.g. ...