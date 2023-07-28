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

### Troubleshooting

In case of cache errors - run this cmake command after deleting the build folder

```cmake -S . -B ./build```

## Unit Tests

Run unit tests with Hydra framework like this

```
yarn install
yarn test
```

## Deployment

Example deploys to "dao.hypha"

```
cd build
cleos set contract dao.hypha dao dao.wasm dao.abi
```

### Deployment size issues

Some EOSIO chains have size limite for uploading WASM files - seems around 512KB. The error messages when uploading larger WASM files are misleading and confusing. (http error...). 

If there is an unexpected problem uploading, check the size of the WASM file. 

It is possible to shrink the file prior to upload using wasm-opt

```
# Optimize for size.
wasm-opt -Os -o output.wasm input.wasm

# Optimize aggressively for size.
wasm-opt -Oz -o output.wasm input.wasm

# Optimize for speed.
wasm-opt -O -o output.wasm input.wasm

# Optimize aggressively for speed.
wasm-opt -O3 -o output.wasm input.wasm
```

## Permissions
```dao.hypha@eosio.code``` permission must be added to both active and owner

## CLIs and Testing (outdated)

### daoctl
The repo at https://github.com/hypha-dao/daoctl features a CLI that can be used to interact with the contracts.

### dao.js - deprecated
The script at scripts/dao.js can interacts with the contracts.

### Go library and tests
Unit and some integrations tests at https://github.com/hypha-dao/dao-contracts/dao-go 

## Building on Mac OS X

Every OS update breaks the build. Use this command to clean directory when there are build errors: 

```
git clean -xdf
```

