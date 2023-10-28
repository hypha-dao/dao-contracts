cd build
wasm-opt -O3 dao/dao.wasm -o dao/dao_O3.wasm
cleos set contract dao.hypha dao dao_O3.wasm dao.abi