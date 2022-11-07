# DAO Contracts

EOSIO contracts for managing a decentralized autonomous organization on EOSIO chains.

![Status](https://github.com/hypha-dao/dao-contracts/actions/workflows/build.yaml/badge.svg?branch=develop)

## Getting Started

#### Dependencies
- eosio
- eosio.cdt 
- cmake 

```
git clone https://github.com/hypha-dao/dao-contracts
cd dao-contracts
git submodule update
#Use dev for testnet or prod for mainnet 
./setup.sh prod|dev
mkdir build && cd build
cmake ..
make
```

## CLIs and Testing

### daoctl
The repo at https://github.com/hypha-dao/daoctl features a CLI that can be used to interact with the contracts.

### dao.js - deprecated
The script at scripts/dao.js can interacts with the contracts.

### Go library and tests
Unit and some integrations tests at https://github.com/hypha-dao/dao-contracts/dao-go 



