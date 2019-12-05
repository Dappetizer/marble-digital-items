#! /bin/bash

#contract
if [[ "$1" == "marble" ]]; then
    contract=marble
    
else
    echo "need contract"
    exit 0
fi

#account
account=$2

#network
if [[ "$3" == "mainnet" ]]; then
    network=Mainnet
    url=http://api.tlos.goodblock.io #Telos Mainnet
elif [[ "$3" == "testnet" ]]; then
    network=Testnet
    url=https://testnet.telosusa.io/ #Telos Testnet (Basho)
elif [[ "$3" == "local" ]]; then
    network=Local
    url=http://127.0.0.1:8888
else
    echo "need network"
    exit 0
fi

echo ">>> Deploying $contract to $account on Telos $network..."

#eosio v1.8.4
cleos -u $url set contract $account ./build/$contract/ $contract.wasm $contract.abi -p $account