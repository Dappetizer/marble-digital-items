# Marble Starter Guide

Follow the steps below to get started creating a Marble NFT:

## 1. Setup

To begin, navigate to the project directory: `marble-standard/`

    mkdir build && mkdir build/marble

    chmod +x build.sh

    chmod +x deploy.sh

## 2. Build

    ./build.sh marble

## 3. Deploy

    ./deploy.sh marble { mainnet | testnet | local }

## 4. Initialize

The first action called on the contract should be the `init()` action. This will set the initial contract version and initial access method in the tokenconfigs.

`cleos push action testaccount1 init '[ ... ]' -p testaccount1`

## 5. Create a Group

Groups are sets of NFT's that are all related to each other in some way. NFT's can only belong to a single group at a time, and not all NFT's in a group will have sequential serial numbers.

Group Examples: `cards`, `pokemon`, `dragons`, `players`

`cleos push action testaccount1 newgroup '[ ... ]' -p testaccount1`

## 6. Mint an NFT

Once a group has been created, NFTs can be minted by the group manager.

`cleos push action testaccount1 newnft '[ ... ]' -p testaccount1`