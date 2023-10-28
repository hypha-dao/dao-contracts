cd build
make -j10
wasm-opt -O3 dao/dao.wasm -o dao/dao_O3.wasm
