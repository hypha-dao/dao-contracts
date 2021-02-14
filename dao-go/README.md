## Quick Start
This folder/Go module is used for testing the DAO smart contracts and also serves as a primitive Go library for interacting with the DAO.

Recommended setup:
```
mkdir ~/dev/hypha && cd ~/dev/hypha
git clone https://github.com/hypha-dao/dao-contracts
cd dao-contracts
mkdir build
cmake ..
make -j8s
cd ../dao-go
go test -v -timeout 0
```
This file is used to start nodeos when running the Go tests. It should work if nodeos is in the path, but you should modify this to your environment.

```dao-go/nodeos.sh``` : current contents

```
#!/bin/sh
nodeos -e -p eosio --plugin eosio::producer_plugin  --max-transaction-time 300 --plugin eosio::producer_api_plugin --plugin eosio::chain_api_plugin --plugin eosio::http_plugin --plugin eosio::history_plugin --plugin eosio::history_api_plugin --filter-on='*' --access-control-allow-origin='*' --contracts-console --http-validate-host=false --verbose-http-errors --delete-all-blocks &> nodeos.log
```

### Run tests

To run tests: 
```
go test -v -timeout 0
```

> NOTE: The test harness will start (and restart) your local instance of nodeos with the correct parameters as long as nodeos is in your path.  This is used on macOS - see the nodeos.sh script and you may need to adapt for Windows or other environments.