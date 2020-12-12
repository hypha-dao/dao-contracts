# Attestation Proposals
Attestations represent any general purpose statement, policy, or assertion made by the HVOICE token holders. It creates a document on chain containing the attestation, for historical record keeping.  

Attestations can be edited, and the new content is merged into the existing document, adding new elements and overwriting existing ones.  

## Example: Hypha Policy
### Treasurer Hardware Wallet Threshold Policy
As an example, here's a proposed policy to set the threshold for treasurers to use hardware wallets as a value of 50 million SEEDS. Of course, multisignature is used for all treasuries, but hardware signers should be required for high values. In other words, any specific treasury (crypto wallet) holding an equivalent value of this many SEEDS should use hardware. Then, the example edits this value as a subsequent policy update.

#### Original
An attestation can be any number of variant-type ```label-value``` pairs in any number of ```content_groups```.  

The only required fields are:
- ```content_group_label``` indicating the ```details``` group
- ```title```, specifically, this is used for the ballot and proposal card 
- ```description```, used for ballot and proposal card

In this example, adding the specific field for ```clause_hardware_threshold``` is certainly not required. However, it's potentially helpful if there are later APIs that want to read this policy and take some action, such as our treasury wallet or apps. 

``` json
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
                "label": "clause_hardware",
                "value": [
                    "string",
                    "Hardware wallets should be used for any treasury over the value of 50 million Seeds"
                ]
            },
            {
                "label": "clause_hardware_threshold",
                "value": [
                    "asset",
                    "50000000.0000 SEEDS"
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

``` json
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

``` json
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