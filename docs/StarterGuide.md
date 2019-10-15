# Nifty Starter Guide

Follow the steps below to 

## 1. Setup

To begin, navigate to the project directory: `nifty-standard/`

    mkdir build && mkdir build/nifty

    chmod +x build.sh

    chmod +x deploy.sh

## 2. Build

    ./build.sh nifty

## 3. Deploy

    ./deploy.sh nifty { mainnet | testnet | local }

## 4. Initialize

The first action called on the contract should be the `init()` action. This will set the initial contract version and initial access method in the tokenconfigs.

`cleos push action testaccount1 init '[ ... ]' -p testaccount1`

## 5. Create a Set

Sets are collections of NFT's that are all related to each other in some way. NFT's can only belong to a single set at a time, and not all NFT's in a set will have sequential serial numbers.

`cleos push action testaccount1 newset '[ ... ]' -p testaccount1`

## 6. Mint an NFT

Once a set has been created, NFTs can be minted by the set manager.

`cleos push action testaccount1 newnft '[ ... ]' -p testaccount1`