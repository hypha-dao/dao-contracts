# Root node
The root node is the DHO document itself.  It is a document in the document table with content that identifies the contract (get_self()) as the root_node.  This creates a hash, which is used in the edges table to connect the DHO to all of its data.

The hash of the root node is saved in the config table of dao.hypha.

To generate the hash in the config table, call "createroot".

```
 eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha createroot '{"notes":"creating the root node"}' -p dao.hypha
```

### Check root node document
Retrieve the hash from the config table.
```
cleos -u https://test.telos.kitchen get table dao.hypha dao.hypha config
```

### Check the document contents
```
cleos -u https://test.telos.kitchen get table -l 1 --index 2 --key-type sha256 -L d4ec74355830056924c83f20ffb1a22ad0c5145a96daddf6301897a092de951e dao.hypha dao.hypha documents
```

Or use the JS utility:
```
node ../document/js/index.js --get --hash d4ec74355830056924c83f20ffb1a22ad0c5145a96daddf6301897a092de951e --contract dao.hypha
```

# Member nodes
If the DHO has existing members in the members table, there needs to be a document created for each one.  Run "makememdocs".

```
 eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha makememdocs '{"notes":"making mem docs"}' -p dao.hypha
```

## Checking 

# Erasing docs
In test environments, there is an action to remove the entire graph, including all documents and edges.
``` json
 eosc -u https://test.telos.kitchen --vault-file ../eosc-testnet-vault.json tx create dao.hypha erasealldocs '{"notes":"erasing all docs"}' -p dao.hypha
```

You can also erase docs one by one via hash:
```
eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha erasedochash '{"doc":"28609e9521b876774db84753c37cca088b4015b21153666e0720d6b9043d0fca"}' -p dao.hypha
```

# Reset graph
Combine erasing the graph with creating the root and member docs by combining the 3 commands:
```
eosc -u https://test.telos.kitchen --vault-file ../eosc-testnet-vault.json tx create dao.hypha erasealldocs '{"notes":"erasing all docs"}' -p dao.hypha

eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha createroot '{"notes":"creating the root node"}' -p dao.hypha

eosc -u https://testnet.telos.caleos.io --vault-file ../eosc-testnet-vault.json tx create dao.hypha makememdocs '{"notes":"making mem docs"}' -p dao.hypha

```