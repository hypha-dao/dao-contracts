project(seedsexchg)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( seedsexchg seedsexchg seedsexchg.cpp )
target_include_directories( seedsexchg PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( seedsexchg ${CMAKE_SOURCE_DIR}/../ricardian )