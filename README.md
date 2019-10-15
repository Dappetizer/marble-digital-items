# Nifty Standard
A lightweight NFT standard for EOSIO software.

### Features

`Simple Contract Interface`

The Nifty Standard is meant to be straightforward and easy to understand - no complex designs to slow you down. Follow the 6 Steps in the [Nifty Starter Guide](docs/StarterGuide.md) to easily deploy a Nifty NFT.

`Low Resource Consumption`

Nifty NFT's are lightweight and take up very few resources, and additional features are designed to be modular and opt-in.

`Verifiable Offchain Content`

Nifty Contracts allow individual NFTs to set links pointing to external content. Optional fields are available for setting content checksums and algorithms, adding to the easy auditability of stored NFT content.

Off-chain content could be anything from raw JSON or Markdown, to dStor/IPFS cids.

`Generic Attribute System`

Every Nifty NFT can optionally have one or more named attributes assigned to it, each with an associated point value. This generic pattern allows developers the freedom to design and interpret attributes specific to their application use case.

Some example attributes are: `level`, `strength`, `experience`, `friends`, `miles`, `bananas`, etc.

## Nifty Documentation

| Name | Description |
| --- | --- |
| [Starter Guide](docs/StarterGuide.md) | Easy to follow setup, build, and deployment instructions. |

## Contributing to Nifty Standard

Nifty is an open-source token standard where contributions and improvements are welcomed by the community.

To make a contribution, simply fork this repo and submit a PR for your changes. In order to avoid duplication of work, it's best to engage with the community first to determine what would be an acceptable addition to the standard.