## Quick Start
Currently, this repo is only used for unit and integration testing the DAO contracts.  

### Dependencies

#### TODO
TODO: integrate contracts more cleanly as linked repositories or otherwise

Recommended setup:
```
mkdir ~/dev/hypha && cd ~/dev/hypha
git clone https://github.com/hypha-dao/eosio-contracts
cd eosio-contracts
cmake .
make
cd ..

mkdir ~/dev/telosnetwork && cd ~/dev/telosnetwork
git clone https://github.com/telosnetwork/telos-decide
cd telos-decide/contracts
cmake .
make
cd ../..

git clone https://github.com/digital-scarcity/token
cd token
cmake .
make 
cd ..

git clone https://github.com/hypha-dao/treasury-contracts
cd treasury-contracts
cmake .
make
cd ..

git clone https://github.com/hypha-dao/monitor
cd monitor
cmake .
make
cd ..

git clone https://github.com/hypha-dao/dao-go
cd dao-go
go test -v -timeout 0
```

> NOTE: check lines 101-119 of ```environment_test.go``` to ensure paths seems accurate. This will be updated to be more developer friendly soon.

Update line 20 
- Go 1.14+
- eosio
- Contracts: 
    - https://github.com/hyphda-dao/eosio-contracts
    - https://github.com/hypha-dao/treasury-contracts
    - https://github.com/telosnetwork/telos-decide
    - https://github.com/digital-scarcity/token
    - https://github.com/hypha-dao/monitor


### Run tests

To run tests: 
```
go test -v -timeout 0
```

> NOTE: The test harness will start (and restart) your local instance of nodeos with the correct parameters as long as nodeos is in your path.  This is used on macOS - see the nodeos.sh script and you may need to adapt for Windows or other environments.