#! /bin/bash

#contract
if [[ "$1" == "nifty" ]]; then
    contract=nifty
    account=testaccount1
else
    echo "need contract"
    exit 0
fi

#network
if [[ "$2" == "mainnet" ]]; then
    url=http://api.tlos.goodblock.io #Telos Mainnet
elif [[ "$2" == "testnet" ]]; then
    url=https://api-test.tlos.goodblock.io/ #Telos Testnet (Basho)
elif [[ "$2" == "local" ]]; then
    url=http://127.0.0.1:8888
else
    echo "need network"
    exit 0
fi

echo ">>> Deploying $contract to $account on $2..."

#eosio v1.8.4
cleos -u $url set contract $account ./build/$contract/ $contract.wasm $contract.abi -p $account