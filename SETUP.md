
1. Create a token
2. 

SUFFIX=hypha
ACCOUNT=bm

eosc -u https://test.telos.kitchen --vault-file ../x/test-vault.json system setcontract bm.hypha dao/dao.wasm dao/dao.abi


eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create token.hypha create '{"issuer":"bm.hypha","maximum_supply":"-1.00 BM"}' -p token.hypha

eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create voice.hypha create '{"issuer":"bm.hypha","maximum_supply":"-1.00 BMV","decay_period":0,"decay_per_period_x10M":0}' -p voice.hypha

eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create husd.hypha create '{"issuer":"bm.hypha","maximum_supply":"-1.00 BUSD"}' -p husd.hypha

eosc -u https://test.telos.kitchen --vault-file ~/x/test-vault.json system setcontract bm.hypha ../dao-contracts/build/dao/dao.wasm ../dao-contracts/build/dao/dao.abi
eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create bm.hypha createdao '{"dao_name":"bm"}' -p bm.hypha


eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create bm.hypha setsetting '{"dao_name":"bm","key":"voting_duration_sec","value":["int64",3600]}' -p bm.hypha

eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create bm.hypha setsetting '{"dao_name":"bm","key":"peg_token_contract","value":["name","husd.hypha"]}' -p bm.hypha
eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create bm.hypha setsetting '{"dao_name":"bm","key":"peg_token_symbol","value":["asset","0.00 BUSD"]}' -p bm.hypha

eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create bm.hypha setsetting '{"dao_name":"bm","key":"reward_token_contract","value":["name","token.hypha"]}' -p bm.hypha
eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create bm.hypha setsetting '{"dao_name":"bm","key":"reward_token_symbol","value":["asset","0.00 BM"]}' -p bm.hypha

eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create bm.hypha setsetting '{"dao_name":"bm","key":"governance_token_contract","value":["name","voice.hypha"]}' -p bm.hypha
eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create bm.hypha setsetting '{"dao_name":"bm","key":"governance_token_symbol","value":["asset","0.00 BMV"]}' -p bm.hypha

eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create bm.hypha setsetting '{"dao_name":"bm","key":"seeds_token_contract","value":["name","token.seeds"]}' -p bm.hypha
eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create bm.hypha setsetting '{"dao_name":"bm","key":"treasury_contract","value":["name","bank.hypha"]}' -p bm.hypha

eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create bm.hypha setsetting '{"dao_name":"bm","key":"seeds_escrow_contract","value":["name","escrow.hypha"]}' -p bm.hypha

eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create bm.hypha apply '{"applicant":"johnnyhypha1","content":"let me in!"}' -p johnnyhypha1
eosc --vault-file ~/x/test-vault.json -u https://test.telos.kitchen tx create bm.hypha enroll '{"enroller":"bm","applicant":"johnnyhypha1","content":"welcome to bm!"}' -p johnnyhypha1



Testnet keypair: 
Private key: 5KQfzXZrmbWvLReGKSdq7GqLhQ8xohs1B18pyXeqtERmjyw1LEp
Public key: EOS8BGVN3Nq2aeC2qQwizoe3AsTVi5oP9pu834rwrqz7wKcz1VQEp