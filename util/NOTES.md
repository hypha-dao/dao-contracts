## Publishing large contracts

1 - Size cannot be above 512Kb which is 512 * 1024 = 524,288 bytes!

2 - if the size of the transaction is too big, cleos throws strange errors

3 - wasm-opt version 112 and 114 produce invalid opcodes, use v105

Note the size is limited by the maximum transaction size, which is 512KB. Contract size could in theory be much bigger. 


## examples - set contract fails with strange error - set code works because it's < 512KB

```
❯ cleos -u https://jungle4.cryptolions.io  set contract daoxhypha111 . dao_gery_v105.wasm dao_gery.abi
Reading WASM from /Users/elohim/play/hypha/dao-contracts/build/dao/dao_gery_v105.wasm...
Publishing contract...
Failed to connect to keosd at unix:///Users/elohim/eosio-wallet/keosd.sock; is keosd running?
[ => false error message, transaction was just too big!]

~/play/hypha/dao-contracts/build/dao antelope_use_dunes_2* 10s
❯ cleos -u https://jungle4.cryptolions.io  set code daoxhypha111 dao_gery_v105.wasm              
Reading WASM from /Users/elohim/play/hypha/dao-contracts/build/dao/dao_gery_v105.wasm...
Setting Code...
executed transaction: 036de61902d5ebfd4b67a6e75c56342873fd0fe1180771b2f14391a1ef3d460e  169760 bytes  11083 us
#         eosio <= eosio::setcode               {"account":"daoxhypha111","vmtype":0,"vmversion":0,"code":"0061736d0100000001e2034260027f7f0060037f7...
warning: transaction executed locally, but may not be confirmed by the network yet         ] 

❯ cleos -u https://jungle4.cryptolions.io  set abi daoxhypha111 dao_gery.abi      
Setting ABI...
executed transaction: 2cbb325242bc93a1866eab9fe29c5c8cc76c54ecb89a8254d00663a061cdac24  2288 bytes  189 us
#         eosio <= eosio::setabi                {"account":"daoxhypha111","abi":"0e656f73696f3a3a6162692f312e32050c436f6e74656e7447726f757009436f6e7...
```

## Some endpoints reject the large transaction
cleos --print-request -u http://eos.greymass.com set code --compression 1 daoxhypha111 dao_gery_v105.wasm -p daoxhypha111@active

## Others accept it
cleos --print-request -u https://eos.eosusa.io set code --compression 1 dao.hypha dao_gery_v105.wasm -p dao.hypha@active


https://eos.eosusa.io/
http://eos.eosusa.io/

cleos -u https://eos.eosusa.io get actions -j dao.hypha

# Powerup Usage

```
cleoseosmain get table eosio 0 powup.state
```

```
cleoseosmain push action eosio powerup '{
            "payer": "dao.hypha",
            "receiver": "dao.hypha",
            "days": 1,
            "net_frac": "39884497",
            "cpu_frac": "304276603",
            "max_payment": "0.0022 EOS"
}' -p dao.hypha@active
```

cleoseosmain push action eosio powerup '{
            "payer": "illum1nation",
            "receiver": "illum1nation",
            "days": 1,
            "net_frac": "19884497",
            "cpu_frac": "104276603",
            "max_payment": "0.0001 EOS"
}' -p illum1nation@active
