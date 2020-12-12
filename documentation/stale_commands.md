# From telos-test.sh
NEW TEST NET: 

Private key: 5HwnoWBuuRmNdcqwBzd1LABFRKnTk2RY2kUMYKkZfF8tKodubtK
Public key: EOS5tEdJd32ANvoxSecRnY5ucr1jbzaVN2rQZegj6NxsevGU8JoaJ

dao.hypha  -- the primary account associated with the DAO
token.hypha  -- token account for HYPHA and PRESEED

Users:
johnnyhypha1
samanthahyph
jameshypha11
thomashypha1
haydenhypha1

Private key: 5KBBECPRDtgPJNsTVUerFfxXpya1Ce93avTye9rn4oButkCSbPZ
Public key: EOS7PJ3hWdozwGZPdTFvuZhAvPakuKTGy4SJfweLGB7Pgu9sU9aW7

ddd, eee, fff, ggg, hhh
iii, jjj, lll


eosc -u https://test.telos.kitchen --vault-file ../eosc-testnet-vault.json system newaccount hypha msig.hypha --stake-cpu "1.0000 TLOS" --stake-net "1.0000 TLOS" --transfer --auth-key EOS7PJ3hWdozwGZPdTFvuZhAvPakuKTGy4SJfweLGB7Pgu9sU9aW7
eosc -u https://test.telos.kitchen --vault-file ../../teloskitchen/tk-dev.json transfer teloskitchen msig.hypha "1000.0000 TLOS"
eosc -u https://test.telos.kitchen --vault-file ../eosc-testnet-vault.json system buyrambytes msig.hypha msig.hypha 10000

eosc -u https://test.telos.kitchen --vault-file ../eosc-testnet-vault.json system newaccount hypha test.hypha --stake-cpu "1.0000 TLOS" --stake-net "1.0000 TLOS" --transfer --auth-key EOS7PJ3hWdozwGZPdTFvuZhAvPakuKTGy4SJfweLGB7Pgu9sU9aW7
eosc -u https://test.telos.kitchen --vault-file ../../teloskitchen/tk-dev.json transfer teloskitchen test.hypha "1000.0000 TLOS"
eosc -u https://test.telos.kitchen --vault-file ../eosc-testnet-vault.json system buyrambytes test.hypha test.hypha 100000

eosc -u https://api.telos.kitchen --vault-file dao.hypha.json system setcontract publsh.hypha ../monitor/monitor/monitor.wasm ../monitor/monitor/monitor.abi


export ACCT=treasurermmm
cleos -u https://test.telos.kitchen system newaccount --stake-cpu "1.0000 TLOS" --stake-net "1.0000 TLOS" --buy-ram "1.0000 TLOS" dao.hypha $ACCT EOS7PJ3hWdozwGZPdTFvuZhAvPakuKTGy4SJfweLGB7Pgu9sU9aW7
cleos -u https://test.telos.kitchen push action trailservice regvoter '['"$ACCT"', "2,HVOICE", null]' -p $ACCT
cleos -u https://test.telos.kitchen push action trailservice mint '['"$ACCT"', "1.00 HVOICE", "original mint"]' -p dao.hypha
cleos -u https://test.telos.kitchen push action dao.hypha apply '['"$ACCT"', "Enroll me please"]' -p $ACCT
cleos -u https://test.telos.kitchen push action dao.hypha enroll '["johnnyhypha1", '"$ACCT"', "enrolled"]' -p johnnyhypha1


cleos -u https://test.telos.kitchen set contract dao.hypha hyphadao/hyphadao
cleos -u https://test.telos.kitchen set contract token.hypha ~/dev/token/token/

cleos -u https://test.telos.kitchen push action eosio.token transfer '["teloskitchen", "dao.hypha", "1000.0000 TLOS", "for testing"]' -p teloskitchen
cleos -u https://test.telos.kitchen push action eosio.token transfer '["dao.hypha", "trailservice", "1000.0000 TLOS", "deposit"]' -p dao.hypha
cleos -u https://test.telos.kitchen push action trailservice newtreasury '["dao.hypha", "1000000000.00 HVOICE", "public"]' -p dao.hypha
cleos -u https://test.telos.kitchen push action trailservice toggle '["2,HVOICE", "transferable"]' -p dao.hypha

## Enrolling a user in hypha DAO
cleos -u https://test.telos.kitchen push action trailservice regvoter '["dao.hypha", "2,HVOICE", null]' -p dao.hypha
cleos -u https://test.telos.kitchen push action trailservice mint '["dao.hypha", "1.00 HVOICE", "original mint"]' -p dao.hypha

cleos -u https://test.telos.kitchen push action trailservice regvoter '["johnnyhypha1", "2,HVOICE", null]' -p johnnyhypha1
cleos -u https://test.telos.kitchen push action trailservice mint '["johnnyhypha1", "1.00 HVOICE", "original mint"]' -p dao.hypha

cleos -u https://test.telos.kitchen push action trailservice castvote '["haydenhypha1", "hypha1......2", ["abstain"]]' -p haydenhypha1

cleos -u https://test.telos.kitchen push action dao.hypha closeprop '["roles", 0'

cleos -u https://test.telos.kitchen push action dao.hypha proposerole '["johnnyhypha1", "Underwater Basketweaver", "Weave baskets at the bottom of the sea", "We make *great* baskets.", "11 HYPHA", "11.00000000 PRESEED", "11 HVOICE", 0, 10]' -p johnnyhypha1
cleos -u https://test.telos.kitchen push action trailservice castvote '["johnnyhypha1", "hypha1.....oa", ["pass"]]' -p johnnyhypha1
cleos -u https://test.telos.kitchen push action dao.hypha closeprop '["roles", 26]' -p haydenhypha1


# You can run these statements over and over because the commands end with the same state as the beginning
# The applicant must run these two actions (preferably as the same transaction)
cleos -u https://test.telos.kitchen push action trailservice regvoter '["hyphalondon2", "0,HVOICE", null]' -p hyphalondon2
cleos -u https://test.telos.kitchen push action dao.hypha apply '["hyphalondon2", "I met with Debbie at the regen conference and we talked about Hypha. I would like to join."]' -p hyphalondon2

cleos -u https://test.telos.kitchen push action dao.hypha enroll '["johnnyhypha1", "hyphalondon2", "Debbie confirmed she made this referral"]' -p johnnyhypha1

# The account can be unregistered if they transfer away their HVOICE and call unregvoter
cleos -u https://test.telos.kitchen push action dao.hypha removemember '["hyphalondon2"]' -p dao.hypha  
cleos -u https://test.telos.kitchen push action trailservice transfer '["hyphalondon2", "johnnyhypha1", "1 HVOICE", "memo"]' -p hyphalondon2
cleos -u https://test.telos.kitchen push action trailservice unregvoter '["hyphalondon2", "0,HVOICE"]' -p hyphalondon2


cleos -u https://test.telos.kitchen push action trailservice castvote '["haydenhypha1", "hypha1.....", ["pass"]]' -p haydenhypha1
cleos -u https://test.telos.kitchen push action dao.hypha closeprop '["payouts", 0]' -p haydenhypha1
cleos -u https://test.telos.kitchen push action dao.hypha setlastballt '["hypha1.....1f"]' -p dao.hypha


cleos -u https://test.telos.kitchen push action -sjd -x 86400 eosio.token transfer '["dao.hypha", "johnnyhypha1", "1.2345 TLOS", "testing native approval trx"]' -p dao.hypha > hypha_xfer_test.json
cleos -u https://test.telos.kitchen push action dao.hypha 
cleos -u https://api.eosnewyork.io multisig propose_trx unpause '[{"actor": "gftma.x", "permission": "active"}, {"actor": "danielflora3", "permission": "active"}]' ./gyftietokens_unpause.json gftma.x

cleos -u https://test.telos.kitchen push action token.hypha issue '["dao.hypha", "100 HYPHA", "memo"]' -p dao.hypha
cleos -u https://test.telos.kitchen push action token.hypha transfer '["dao.hypha", "haydenhypha1", "1 HYPHA", "memo"]' -p dao.hypha

## new format

cleos -u https://test.telos.kitchen push action dao.hypha mapit '{"key":"strings":"value":{"key":"tester":"value":"tester value"}}' -p dao.hypha

cleos -u https://test.telos.kitchen push action dao.hypha propose '{"proposer":"johnnyhypha1", 
                                                                        "proposal_type":"role", 
                                                                        "trx_action_name":"newrole", 
                                                                        "names":null, 
                                                                        "strings":{   
                                                                            "name":"Underwater Basketweaver", 
                                                                            "description":"Weave baskets at the bottom of the sea",
                                                                            "content":"We make *great* baskets."
                                                                        }, 
                                                                        "assets":{
                                                                            "hypha_amount":"11 HYPHA", 
                                                                            "seeds_amount":"11.00000000 SEEDS", 
                                                                            "hvoice_amount":"11 HVOICE"
                                                                        }, 
                                                                        "time_points":null, 
                                                                        "ints":{
                                                                            "start_period":"41", 
                                                                            "end_period":"51"
                                                                        }, 
                                                                        "floats":null,
                                                                        "trxs":null' -p johnnyhypha1



##
cleos -u https://test.telos.kitchen push action dao.hypha setconfig '["token.hypha", "trailservice"]' -p dao.hypha

cleos -u https://test.telos.kitchen push action token.hypha create '["dao.hypha", "-1.00 HYPHA"]' -p token.hypha
cleos -u https://test.telos.kitchen push action token.hypha create '["dao.hypha", "-1.00 HUSD"]' -p token.hypha



cleos -u https://test.telos.kitchen push action token.hypha issue '["dao.hypha", "1.00 HYPHA", "memo"]' -p dao.hypha
cleos -u https://test.telos.kitchen push action token.hypha transfer '["dao.hypha", "johnnyhypha1", "1.00000000 PRESEED", "memo"]' -p dao.hypha

cleos -u https://test.telos.kitchen push action dao.hypha addmember '["johnnyhypha1"]' -p dao.hypha
cleos -u https://test.telos.kitchen push action dao.hypha addmember '["samanthahyph"]' -p dao.hypha
cleos -u https://test.telos.kitchen push action dao.hypha addmember '["jameshypha11"]' -p dao.hypha
cleos -u https://test.telos.kitchen push action dao.hypha addmember '["thomashypha1"]' -p dao.hypha
cleos -u https://test.telos.kitchen push action dao.hypha addmember '["haydenhypha1"]' -p dao.hypha


cleos -u https://test.telos.kitchen push action token.hypha issue '["dao.hypha", "1 HYPHA", "memo"]' -p dao.hypha
cleos -u https://test.telos.kitchen push action token.hypha transfer '["dao.hypha", "johnnyhypha1", "1 HYPHA", "memo"]' -p dao.hypha

# cleos -u https://test.telos.kitchen push action token.hypha issue '["samanthahyph", "1 HYPHA", "memo"]' -p dao.hypha
# cleos -u https://test.telos.kitchen push action token.hypha issue '["jameshypha11", "1 HYPHA", "memo"]' -p dao.hypha
# cleos -u https://test.telos.kitchen push action token.hypha issue '["thomashypha1", "1 HYPHA", "memo"]' -p dao.hypha
# cleos -u https://test.telos.kitchen push action token.hypha issue '["haydenhypha1", "1 HYPHA", "memo"]' -p dao.hypha

cleos -u https://test.telos.kitchen push action eosio.trail regtoken '["1000000000000 HVOIC", "dao.hypha", "https://dao.hypha.earth"]' -p dao.hypha
cleos -u https://test.telos.kitchen push action eosio.trail issuetoken '["dao.hypha", "johnnyhypha1", "1 HVOIC", false]' -p dao.hypha

# cleos -u https://test.telos.kitchen push action eosio.trail issuetoken '["dao.hypha", "hyphamember2", "1 HVOICE", false]' -p dao.hypha
# cleos -u https://test.telos.kitchen push action eosio.trail issuetoken '["dao.hypha", "hyphamember3", "1 HVOICE", false]' -p dao.hypha
# cleos -u https://test.telos.kitchen push action eosio.trail issuetoken '["dao.hypha", "hyphamember4", "1 HVOICE", false]' -p dao.hypha
# cleos -u https://test.telos.kitchen push action eosio.trail issuetoken '["dao.hypha", "hyphamember5", "1 HVOICE", false]' -p dao.hypha


cleos -u https://test.telos.kitchen push action dao.hypha proposerole '["johnnyhypha1", "Underwater Basketweaver", "Weave baskets at the bottom of the sea", "We make *great* baskets.", "11 HYPHA", "11.00000000 PRESEED", "11 HVOICE", 0, 10]' -p johnnyhypha1
cleos -u https://test.telos.kitchen push action dao.hypha propassign '["johnnyhypha1", "johnnyhypha1", 0, "https://joinseeds.com", "I am a professional basket maker and scuba diver", 0, 6, 1.000000000]' -p johnnyhypha1

cleos -u https://test.telos.kitchen push action eosio.trail castvote '["johnnyhypha1", 1, 1]' -p johnnyhypha1

# cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember2", 66, 1]' -p hyphamember2
# cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember3", 66, 1]' -p hyphamember3
# cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember4", 60, 0]' -p hyphamember4

cleos -u https://test.telos.kitchen push action dao.hypha closeprop '[0]' -p hyphamember1

cleos -u https://test.telos.kitchen push action dao.hypha payassign '[0, 0]' -p hyphamember3
cleos -u https://test.telos.kitchen push action dao.hypha resetperiods '[]' -p dao.hypha
cleos -u https://test.telos.kitchen push action dao.hypha assign '[2]' -p dao.hypha











doesnt work: 3fbdca985be8751bcfd3917bdbae80ea9e33770a
does work: 


62b2e3dd38533cd6f3ced50c3f53c76f40e98651







# Trail experimentation

cleos -u https://test.telos.kitchen get table dao.hypha dao.hypha applicants


cleos -u https://test.telos.kitchen get table dao.hypha dao.hypha config
cleos -u https://test.telos.kitchen get table dao.hypha dao.hypha nominees
cleos -u https://test.telos.kitchen get table dao.hypha dao.hypha boardmembers

cleos -u https://test.telos.kitchen get table dao.hypha dao.hypha proposals
cleos -u https://test.telos.kitchen get table dao.hypha dao.hypha roles
cleos -u https://test.telos.kitchen get table dao.hypha dao.hypha assignments
cleos -u https://test.telos.kitchen get table dao.hypha dao.hypha roleprops

cleos -u https://test.telos.kitchen get table eosio.trail eosio.trail registries
cleos -u https://test.telos.kitchen get table -lower 50  eosio.trail eosio.trail ballots
cleos -u https://test.telos.kitchen get table eosio.trail eosio.trail elections
cleos -u https://test.telos.kitchen get table eosio.trail eosio.trail leaderboards --lower 4
cleos -u https://test.telos.kitchen get table eosio.trail eosio.trail proposals

cleos -u https://test.telos.kitchen push action hyphadaobali reset '[]' -p hyphadaobali
cleos -u https://test.telos.kitchen push action hyphadaobal1 inithvoice '["https://joinseeds.com"]' -p hyphadaobal1
cleos -u https://test.telos.kitchen push action hyphadaobali initsteward '["https://joinseeds.com"]' -p hyphaboard11

cleos -u https://test.telos.kitchen push action eosio.trail issuetoken '["hyphadaobal1", "hyphamember1", "1 HYVO", false]' -p hyphadaobal1
cleos -u https://test.telos.kitchen push action eosio.trail issuetoken '["hyphadaobal1", "hyphamember2", "1 HYVO", false]' -p hyphadaobal1
cleos -u https://test.telos.kitchen push action eosio.trail issuetoken '["hyphadaobal1", "hyphamember3", "1 HYVO", false]' -p hyphadaobal1
cleos -u https://test.telos.kitchen push action eosio.trail issuetoken '["hyphadaobal1", "hyphamember4", "1 HYVO", false]' -p hyphadaobal1
cleos -u https://test.telos.kitchen push action eosio.trail issuetoken '["hyphadaobal1", "hyphamember5", "1 HYVO", false]' -p hyphadaobal1

cleos -u https://test.telos.kitchen push action hyphaboard11 nominate '["hyphamember1", "hyphamember1"]' -p hyphamember1
cleos -u https://test.telos.kitchen push action hyphaboard11 makeelection '["hyphamember1", "https://joinseeds.com"]' -p hyphamember1

cleos -u https://test.telos.kitchen push action hyphaboard11 addcand '["hyphamember1", "https://joinseeds.com"]' -p hyphamember1


cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember1", 19, 0]' -p hyphamember1
cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember2", 19, 1]' -p hyphamember2
cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember3", 19, 1]' -p hyphamember3
cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember4", 19, 1]' -p hyphamember4

cleos -u https://test.telos.kitchen push action hyphaboard11 endelection '["hyphamember1"]' -p hyphamember1


##### Propose a role
cleos -u https://test.telos.kitchen push action hyphadaobal1 proposerole '["hyphamember1", "Strawberry Gatherer", "https://joinseeds.com", "Farmer growing food", "12 HYPHA", "9 PRESEED", "15 HYVOICE"]' -p hyphamember1
cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember4", 41, 1]' -p hyphamember4
cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember2", 41, 1]' -p hyphamember2
cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember3", 41, 1]' -p hyphamember3
cleos -u https://test.telos.kitchen push action hyphadaobal1 closeprop '["hyphamember1", 0]' -p hyphamember1

#####  Propose an assignment
cleos -u https://test.telos.kitchen push action hyphadaobal1 propassign '["hyphamember1", "hyphamember1", 0, "https://joinseeds.com", "I would like this job", 0, 1.000000000]' -p hyphamember1
cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember1", 34, 1]' -p hyphamember1
cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember2", 34, 1]' -p hyphamember2
cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember3", 34, 1]' -p hyphamember3
cleos -u https://test.telos.kitchen push action hyphadaobal1 closeprop '[1]' -p hyphamember1

##### Propose a contribution
cleos -u https://test.telos.kitchen set contract hyphadaobal1 hyphadao/hyphadao
cleos -u https://test.telos.kitchen push action hyphadaobal1 proppayout '["hyphamember2", "hyphamember2", "Investment", "https://joinseeds.com", "2000 HHH", "10000.00000000 PPP", "5 HYVO", "2019-07-05T05:49:01.500"]' -p hyphamember2
cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember1", 50, 1]' -p hyphamember1
cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember2", 50, 1]' -p hyphamember2
cleos -u https://test.telos.kitchen push action eosio.trail castvote '["hyphamember3", 50, 1]' -p hyphamember3
cleos -u https://test.telos.kitchen push action hyphadaobal1 closeprop '["hyphamember2", 9]' -p hyphamember2

cleos -u https://test.telos.kitchen get table token.hypha hyphamember2 accounts

cleos -u https://test.telos.kitchen push action hyphadaobal1 makepayout '[2]' -p hyphadaobal1
cleos -u https://test.telos.kitchen push action hyphadaobal1 reset '[]' -p hyphadaobal1





cleos -u https://test.telos.kitchen push action hyphadaobali setconfig '["token.hypha", "token.hypha"]' -p hyphadaobali


cleos -u https://api.telos.kitchen push action eosio updateauth '{
    "account": "dao.hypha",
    "permission": "enrollers",
    "parent": "active",
    "auth": {
        "keys": [],
        "threshold": 1,
        "accounts": [
            {
                "permission": {
                    "actor": "cometogether",
                    "permission": "active"
                },
                "weight": 1
            },
             {
                "permission": {
                    "actor": "hyphanewyork",
                    "permission": "active"
                },
                "weight": 1
            },
             {
                "permission": {
                    "actor": "joachimstroh",
                    "permission": "active"
                },
                "weight": 1
            },
             {
                "permission": {
                    "actor": "illumination",
                    "permission": "active"
                },
                "weight": 1
            },
             {
                "permission": {
                    "actor": "thealchemist",
                    "permission": "active"
                },
                "weight": 1
            }
        ],
        "waits": []
    }
}' -p bank.hypha@owner

cleos -u https://test.telos.kitchen push action eosio updateauth '{
    "account": "varexrounds2",
    "permission": "active",
    "parent": "owner",
    "auth": {
        "keys": [
            {
                "key": "EOS8aoFkEXL9QDcBUvMzHLGgn5Epzz3tQ74q5aCs4CDvw3saccPTh",
                "weight": 1
            }
        ],
        "threshold": 1,
        "accounts": [
            {
                "permission": {
                    "actor": "varexrounds2",
                    "permission": "eosio.code"
                },
                "weight": 1
            }
        ],
        "waits": []
    }
}' -p varexrounds2@owner




------------------------------------------------------------------------------------------------
OLD TEST NET: 

Private key: 5J1gYLAc4GUo7EXNAXyaZTgo3m3SxtxDygdVUsNL4Par5Swfy1q
Public key: EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn

hyphadaotest
hyphatoken11
hyphaboard11
hyphaboard11
hyphamember1
hyphamember2
hyphamember3
hyphamember4
hyphamember5
hyphadaobali
token.hypha



[{"key":"note number 1", "value":" Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam at vehicula nunc, ac porttitor diam. Morbi semper sem eget tristique bibendum. Quisque viverra blandit leo, non luctus arcu molestie vel. Nullam nec odio venenatis, viverra lacus quis, consectetur quam. Maecenas vel elit lectus. Praesent mauris urna, commodo eu suscipit quis, imperdiet vitae neque. Nunc tincidunt, velit eu egestas convallis, erat diam fermentum ex, et pharetra enim magna varius purus. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Nulla egestas viverra elit, a maximus justo iaculis eget. Suspendisse potenti. Sed arcu nulla, suscipit quis semper in, accumsan ut justo. Nullam eget ligula eget nisl scelerisque vehicula."},{"key":"note number 1", "value":"Sed fermentum, nisl vitae convallis ultrices, velit lectus porta arcu, id pulvinar tellus risus condimentum urna. Aenean elementum justo eu erat imperdiet, quis mollis ipsum viverra. Vivamus lectus neque, ullamcorper a rhoncus ac, facilisis sit amet ligula. Curabitur interdum, dui nec commodo cursus, metus nunc scelerisque eros, in imperdiet dui elit sed mi. Aliquam viverra, augue at ultrices sodales, risus nunc efficitur elit, nec scelerisque ipsum ipsum a est. Nullam dictum tempor est id accumsan. Praesent vestibulum quis metus id fermentum. Nam semper venenatis velit, eu mattis nunc interdum a. Sed efficitur ligula vel maximus molestie. Etiam lacinia consequat dui vitae mattis. Integer eros tortor, vestibulum non turpis placerat, faucibus feugiat libero. Quisque porta semper consectetur. Etiam ut rutrum tellus. Cras auctor, nisi eu euismod porta, ex ligula interdum ipsum, ut fringilla tortor sem a turpis. Nam eleifend consectetur odio, vel accumsan mauris efficitur at."},{"key":"value 3", "value":"Etiam odio quam, tempus vel gravida ut, scelerisque sit amet augue. Aenean tristique eleifend leo blandit porta. Nunc vel rhoncus leo. Nulla nec tellus cursus, eleifend leo ac, vestibulum purus. Nunc ornare nunc quis lacus sagittis imperdiet et ut turpis. Morbi porttitor feugiat sagittis. Ut dignissim odio ligula, quis blandit sapien efficitur ut. Phasellus vitae turpis congue, molestie sem quis, tempus massa. Phasellus eu consectetur lorem. Praesent blandit, magna gravida pulvinar tempor, ipsum arcu lacinia leo, ut iaculis libero urna sit amet nulla. Nam diam massa, imperdiet at metus vel, iaculis vehicula dui. Suspendisse cursus tortor tincidunt mauris rutrum, in placerat arcu pellentesque."},{"key":"number 5", "value":" Curabitur at lobortis dolor. Pellentesque non eros in massa aliquet posuere non vitae felis. Vivamus eget viverra velit. Duis orci leo, efficitur sit amet nisi eu, egestas vestibulum ex. Proin iaculis tortor non eleifend pharetra. In hac habitasse platea dictumst. Nulla ut dui laoreet, ultrices libero non, fermentum mauris. Nunc egestas commodo massa in tincidunt. Aenean bibendum eu sapien id efficitur. In hac habitasse platea dictumst. Nunc nec urna vel mauris dapibus auctor a varius massa. Proin dui nibh, lacinia a tortor ut, auctor maximus erat. Suspendisse eu dui dui. Suspendisse scelerisque sapien odio, at ullamcorper felis finibus eget. Cras sed molestie justo, feugiat luctus enim. Aliquam nec purus eu leo faucibus egestas vitae ac enim. Donec imperdiet leo lorem, nec egestas metus fermentum eu. Phasellus massa enim, porttitor sed neque posuere, iaculis bibendum leo. Proin id euismod tortor, facilisis luctus quam. Nam ante libero, consequat non neque et, molestie tempor mi. Maecenas facilisis egestas malesuada. "},{"key":"value 8", "value":"Aliquam nec facilisis ligula, nec fermentum elit. Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis egestas. Vivamus in sapien eros. Curabitur in tellus faucibus, dictum mauris sit amet, porta nunc. Donec sed elit dapibus, rutrum nisi at, ullamcorper ipsum. Vivamus commodo nunc at arcu aliquam iaculis. Fusce aliquam dolor eu dolor efficitur, ut tincidunt augue lacinia. Curabitur scelerisque blandit ipsum, vitae tempus arcu blandit dapibus. Aenean gravida est at dui viverra fermentum. Nunc pellentesque magna nec fermentum pharetra. Praesent enim justo, dignissim nec sollicitudin in, volutpat vitae urna. Sed auctor elit libero, vitae tempor sem aliquam vitae. Nunc tempor quam ut lacus pharetra, id sollicitudin ipsum luctus. In ac facilisis dolor, sit amet scelerisque leo. Quisque ac augue sit amet orci sollicitudin tempus laoreet sit amet velit. Quisque rutrum nisl vel augue accumsan iaculis. "}]

[{"key":"note","value":"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris in purus vitae neque faucibus porttitor. Pellentesque metus augue, suscipit et augue et, pretium iaculis erat. Duis pharetra felis quam, ac dictum tellus tincidunt nec. Maecenas risus dui, euismod vitae pellentesque et, ullamcorper eu risus. Curabitur ultrices sit amet eros in facilisis. Vestibulum molestie, velit bibendum eleifend aliquet, justo nibh sollicitudin dolor, vel consequat dui odio non mauris. Etiam id posuere eros, ut dictum nunc. Phasellus vel cursus est, ac dapibus neque. Vestibulum ac suscipit orci, eget rutrum arcu. Fusce commodo commodo risus sit amet rhoncus. Proin ut tempor nisl. Maecenas vitae auctor diam. Sed tortor turpis, semper non diam et, ultricies venenatis urna.Phasellus eu purus dolor. Curabitur interdum consectetur nisi et porttitor. Donec justo lorem, imperdiet et sollicitudin eu, facilisis nec nisl. Nullam vel nisi eget lorem malesuada aliquet et ac orci. Nunc semper orci imperdiet magna varius placerat. Donec vel metus purus. Phasellus venenatis neque sit amet erat suscipit dictum id in augue. Suspendisse egestas ipsum sed felis porttitor congue. Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Vivamus egestas purus velit, suscipit vehicula nunc aliquet sit amet.Aenean vehicula at arcu non consequat. Quisque posuere gravida neque, ut hendrerit nibh placerat non. Mauris ornare neque sed erat dapibus sollicitudin. Sed posuere maximus orci at mollis. In hac habitasse platea dictumst. Quisque blandit nunc ligula, a lacinia leo consequat at. Phasellus accumsan vel ex in tincidunt. Mauris tincidunt tellus efficitur mauris dictum, eu volutpat erat volutpat. Sed posuere mauris velit, a venenatis metus faucibus a. Nulla nec sapien ut quam porttitor consectetur. Nullam molestie tortor orci, sed volutpat velit viverra sit amet. Donec tempus tempor suscipit. Ut finibus laoreet congue.Sed eget ullamcorper massa. Maecenas porta, ipsum non facilisis convallis, ligula metus finibus massa, in malesuada orci nisi at lacus. Maecenas id consequat libero. Suspendisse gravida ornare urna quis venenatis. Nullam rhoncus hendrerit orci, ac mattis lectus sagittis non. Suspendisse bibendum leo id urna dictum tempus. Suspendisse nulla odio, porta vel interdum eget, egestas sit amet ligula. Pellentesque lobortis aliquam sodales. Quisque vel velit lobortis, congue dolor eu, tristique turpis. Duis egestas maximus maximus. Pellentesque erat sem, efficitur quis urna suscipit, imperdiet varius purus.Etiam rutrum eget nibh a consectetur. Praesent in euismod dolor. Interdum et malesuada fames ac ante ipsum primis in faucibus. Pellentesque id malesuada quam. Nunc dolor augue, euismod nec est eu, malesuada rutrum turpis. Nullam rutrum rhoncus turpis, sed cursus odio rutrum et. Fusce egestas massa risus. Morbi porta molestie condimentum. Vestibulum ante ipsum primis in faucibus orci luctus et ultrices posuere cubilia curae; Aliquam erat volutpat biam. "}]




eosc -u https://test.telos.kitchen --vault-file ../gba-vault-test.json tx create dao.gba setconfig '{
    "names": [
      {
        "key": "telos_decide_contract",
        "value": "telos.decide"
      },
      {
        "key":"reward_token_contract",
        "value":"token.gba"
      }
    ],
    "strings": [
      {
        "key": "client_version",
        "value": "pre-alpha"
      },
      {
        "key":"contract_version",
        "value": "pre-alpha"
      }
    ],
    "assets": [],
    "time_points": [],
    "ints": [
      {
        "key": "approve_threshold_perc_x100",
        "value": 50
      },
      {
        "key": "paused",
        "value": 0
      },
      {
        "key": "quorum_perc_x100",
        "value": 20
      },
      {
        "key": "voting_duration_sec",
        "value": 60
      }
    ],
    "floats": [],
    "trxs": []
}' -p dao.gba


# From telos.sh

dao.hypha
token.hypha


# create HYPHA token
cleos -u https://api.telos.kitchen push action token.hypha create '["dao.hypha", "-1.00 HYPHA"]' -p token.hypha
cleos -u https://api.telos.kitchen push action token.hypha issue '["hyphanewyork", "1.00 HYPHA", "Welcome to Hypha!"]' -p dao.hypha

# set config
# cleos -u https://api.telos.kitchen push action dao.hypha setconfig '["token.hypha", "trailservice"]' -p dao.hypha

# fund the trailservice
cleos -u https://api.telos.kitchen push action eosio.token transfer '["dappbuilders", "dao.hypha", "1000.0000 TLOS", "for HVOICE setup"]' -p teloskitchen
cleos -u https://api.telos.kitchen push action eosio.token transfer '["dao.hypha", "trailservice", "1000.0000 TLOS", "deposit"]' -p dao.hypha

# create new treasury
cleos -u https://api.telos.kitchen push action trailservice newtreasury '["dao.hypha", "1000000000.00 HVOICE", "public"]' -p dao.hypha
# cleos -u https://api.telos.kitchen push action trailservice toggle '["0,HVOICE", "transferable"]' -p dao.hypha

# register to vote
cleos -u https://api.telos.kitchen push action trailservice regvoter '["dao.hypha", "2,HVOICE", null]' -p dao.hypha
cleos -u https://api.telos.kitchen push action trailservice mint '["hyphanewyork", "5.00 HVOICE", "mint"]' -p dao.hypha

# apply to the DAO
cleos -u https://api.telos.kitchen push action dao.hypha apply '["hyphanewyork", "The Times 03/Jan/2009 Chancellor on the brink of second bailout for banks"]' -p hyphanewyork

# propose a 1 HYPHA payout to test







export HOST=https://test.telos.kitchen 

cleos -u $HOST set contract dao.hypha hyphadao/
cleos -u $HOST push action dao.hypha resetperiods '[]' -p dao.hypha
cleos -u $HOST push action dao.hypha resetbankcfg '[]' -p dao.hypha

# update table structures
- add coeffecients to time periods
- remove bankconfig table
- add config table structure to bank class

# set config
node dao.js -f proposals/config.json -h $HOST --config
node loadPhases.js


# add inline actions to pay SEEDS to escrow
- transfer
- create # what is vesting period of each payment


# From hypha.sh
cleos create account eosio hyphadao EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn
cleos create account eosio member1 EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn
cleos create account eosio member2 EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn
cleos create account eosio member3 EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn
cleos create account eosio token EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn
cleos create account eosio member4 EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn
cleos create account eosio member5 EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn
cleos create account eosio eosio.trail EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn EOS6vvAofsMC5RJyY6fRHcyiQLNjDGukX6tRUoF1WEc63idQ3BqJn

source perm.json

cleos set contract hyphadao hyphadao/hyphadao
cleos set contract token hyphadao/eosiotoken
cleos set contract eosio.trail ../telos.contracts/eosio.trail

cleos push action hyphadao setconfig '["token", "eosio.trail"]' -p hyphadao
cleos push action hyphadao init '[]' -p hyphadao
cleos push action eosio.trail issuetoken '["hyphadao", "member1", "1 HVOICE", 0]' -p hyphadao
cleos push action eosio.trail issuetoken '["hyphadao", "member2", "1 HVOICE", 0]' -p hyphadao
cleos push action eosio.trail issuetoken '["hyphadao", "member3", "1 HVOICE", 0]' -p hyphadao
cleos push action eosio.trail issuetoken '["hyphadao", "member4", "1 HVOICE", 0]' -p hyphadao
cleos push action eosio.trail issuetoken '["hyphadao", "member5", "1 HVOICE", 0]' -p hyphadao

cleos push action token create '["hyphadao", "1000000000 HYPHA"]' -p token
cleos push action token create '["hyphadao", "1000000000.00000000 PRESEED"]' -p token


cleos push action hyphadao proposerole '["member1", "blockdev", "https://joinseeds.com", "develop cool shit", "10 HYPHA", "500.00000000 PRESEED", "10 HVOICE"]' -p member1
cleos push action hyphadao proposerole '["member2", "webdev", "https://joinseeds.com", "dev website", "14 HYPHA", "450.00000000 PRESEED", "10 HVOICE"]' -p member2
cleos push action eosio.trail castvote '["member1", 0, 1]' -p member1
cleos push action eosio.trail castvote '["member2", 0, 1]' -p member2
cleos push action eosio.trail castvote '["member3", 0, 1]' -p member3

cleos push action hyphadao closeprop '["member1", 0]' -p member1 

cleos push action hyphadao propassign '["member1", "member1", 0, "https://joinseeds.com", "testing assignment", 0, 1.00000000]' -p member1
cleos push action hyphadao propassign '["member2", "member2", 1, "https://joinseeds.com", "testing assignment", 0, 0.50000000]' -p member2

cleos push action hyphadao closeprop '["member1", 1]' -p member1 



cleos get table hyphadao hyphadao proposals
cleos push action hyphadao eraseprop '[0]' -p member1 

# proposals
cleos get table hyphadao hyphadao proposals
cleos get table hyphadao hyphadao payoutprops
cleos get table hyphadao hyphadao roleprops
cleos get table hyphadao hyphadao assprops

# holocracy
cleos get table hyphadao hyphadao roles
cleos get table hyphadao hyphadao assignments


