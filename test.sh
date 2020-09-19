#! /bin/bash

#contract
if [[ "$1" == "marble" ]]; then
    contract=marble
else
    echo "need contract"
    exit 0
fi

echo ">>> Testing $contract contract..."

#copy build to tests folder
# cp build/todo/todo.wasm tests/contracts/todo/
# cp build/todo/todo.abi tests/contracts/todo/

#start nodeos
eoslime nodeos start

#run test suite
mocha tests/marbleTests.js

#stop nodeos
eoslime nodeos stop