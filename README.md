# Dynex [DNX] - Next Generation Blockchain for Neuromorphic Computing

The Dynex platform represents a cutting-edge neuromorphic computing platform, built upon a revolutionary adaptable blockchain system. Comprising of collaborating miners, this decentralized neuromorphic supercomputing network excels in executing calculations with unparalleled speed and efficiency, surpassing quantum computing limitations.

The Dynex platform is capable of performing quantum computing based algorithms without their limitations, significantly accelerating machine learning training, optimizing feature selection and improving model accuracy. The Dynex Neuromoprohic Computing platform connects mining pool hosts that are running clusters of Neuromorphic chips with users and applications that utilize this next-generation hardware. On the Dynex platform, computation time spent by individuals mining is exchanged for the Dynex native token.

DynexSolve is the proprietary mining algorithm that addresses real-life computational tasks by applying Proof-of-useful-Work (PoUW) throughout the mining process. Dynex turns any modern GPU into a simulated neuromorphic computing chip that can perform orders of magnitude more efficient than classical or quantum methodologies for a wide range of applications. Due to the dominance of ASICs in the proof-of-work token mining industry, there is a large amount of dormant GPU infrastructure available which can be converted into high performance next-generation Neuromorphic computing clusters. 

The Dynex mainnet started on September 16th 2022.

For more information, read our [Dynex Whitepapers](https://dynexcoin.org/learn/dynex-whitepapers)


## Receive, Send and Manage your DNX coins

There are multiple ways to receive, send and manage your Dynex coins. Since the coins are stored on the blockchain and not the wallet software, by saving your seed words you can use different options at any time. Each wallet has their own benefits and each user must evaluate what is best for their intended use and specific needs. Advanced users are encouraged to build from source using the documentation included in this repository. Here are the available wallets:

1) [Dynex Mobile Web Wallet (beta)](https://wallet.dynexcoin.org)
2) CLI Wallet (command line wallet): src/simplewallet
3) GUI Wallet (graphical user interface): /src/WalletGui/dynexwallet

Building and running a full node is advised for any individual user seeking to contribute the most to the health and growth of the Dynex network. 

Remember to save your seed phrase offline in a secure location. It is good practice to send a test transaction first, before sending large amounts. There is no way to recover lost funds, so always use caution when handling decentralized cryptocurrencies.


## Earning Dynex with your GPU using commerical grade software

To maximize the efficiency of your computer's hardware, miners may consider using closed-source software for commerical grade preformance. Though these products are not open source (and therefor pripriotory) they are still valued for being actively developed and having increased hashrate compared to open-source.

https://dynexcoin.org/get-dnx#mining


## Running a Dynex node (precompiled binaries)

The open source Dynex miner can be built from source, but the easiest way to start mining DNX or to manage DNX wallet(s) from the command line in the command line terminal is by using our precompiled binaries. Download the version matching your operating system from our releases page:
https://github.com/dynexcoin/Dynex/releases

Please note that Linux and MacOS users are required to have the [Boost library](https://www.boost.org) (Version 1.74.0 or better) installed: 

Linux:
```
sudo apt-get install libboost-all-dev 
```

MacOS:
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

brew install boost
```

After downloading the precompiled binary, unzip the executable on your machine. To run a full Node in the Dynex blockchain, run the main service (=daemon) with the following command and wait until your node is fully synchronized with the network:
```
./dynexd
```

Please note that your node requires ports 17336 and SSL to be open.


## Running the Dynex CLI (command line interface) wallet

From the command line, you can also create and manage your personal wallet to mine and transact your DNX tokens (make sure you have the main service daemon running):

```
./simplewallet
```

## Running the Dynex GUI (graphical user interface) wallet

With the Dynex Wallet App users can use the Dynex wallet functionality to create wallets, send and receive DNX and manage transactions and recipients. It is not necessary to run a separate Dynex node, everything is built into the GUI wallet.

```
WalletGui/dynexwallet
```

Then just follow the instructions on screen.



## Build Daemon, Simplewallet & WalletD from source

You can also entirely build all binaries from the source. First, clone the repository:
```
git clone https://github.com/dynexcoin/Dynex.git
```

### Linux

It is required to have the [Boost library](https://www.boost.org) (Version 1.74.0 or better) and libcurl installed: 
```
sudo apt-get install libboost-all-dev (Ubuntu)
sudo apt-get -y install libcurl4-openssl-dev (libcurl Ubuntu)
```

To compile and build the Dynex node:
```
mkdir build 
cd build
cmake ..
make Daemon -j 8
```

To compile and build the Dynex CLI wallet:
```
mkdir build 
cd build
cmake ..
make SimpleWallet -j 8
```

To compile and build the Dynex GUI wallet:
```
sudo apt-get install libboost-all-dev 
sudo apt-get install libcurl-dev
sudo apt install qtcreator qtbase5-dev qt5-qmake cmake

mkdir build 
cd build
cmake .. -DGUI=True
make WalletGui -j 8
```

To compile and build the Dynex wallet service:
```
mkdir build 
cd build
cmake ..
make PaymentGateService -j 8
```


### MacOS

It is required to have the [Boost library](https://www.boost.org) (Version 1.74.0 or better) installed: 
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

brew install boost
```

To compile and build the Dynex node:
```
mkdir build 
cd build
cmake ..
make Daemon -j 8
```

To compile and build the Dynex CLI wallet:
```
mkdir build 
cd build
cmake ..
make SimpleWallet -j 8
```

To compile and build the Dynex GUI wallet:
```
sudo apt-get install libboost-all-dev 
sudo apt-get install libcurl-dev
sudo apt install qtcreator qtbase5-dev qt5-qmake cmake

mkdir build 
cd build
cmake .. -DGUI=True
make WalletGui -j 8
```

To compile and build the Dynex wallet service:
```
mkdir build 
cd build
cmake ..
make PaymentGateService -j 8
```

### Windows

1. It is required to have the [Boost library](https://www.boost.org) (Version 1.74.0 or better) installed: 
https://www.boost.org/users/download/

2. It is required to have [cmake](https://cmake.org/) for windows installed.
3. You also need [Microsoft Visual Studio](https://visualstudio.microsoft.com) (2017 or later) installed for the build process.

To compile and build:
```
mkdir build 
cd build
cmake ..
```
Then open Visual Studio: 
a) Make sure you have set Solution->Configuration to "Release". 
b) You also need to add "bcrypt.lib" to P2P->Properties->Configuration Properties->Librarian->Additional Dependencies. This is necessary to build boost's cryptographic randomizer functions. 
c) Add the LibCurl include files to CryptoNoteCore:
VC++ Directories -> Include Directories -> YOUR_CURL_PATH\libcurl-vc-x64-release-dll-ssl-dll-zlib-dll-ipv6-sspi\include
d) Add the LibCurl library directories to CryptoNoteCore:
VC++ Directories -> Library Directories -> YOUR_CURL_PATH\libcurl-vc-x64-release-dll-ssl-dll-zlib-dll-ipv6-sspi\lib
and
VC++ Directories -> Library Directories -> YOUR_ZLIB_PATH\zlib\lib

Once these settings are done, proceed a full build to generate your binaries.





# Release Notes

## Release Notes Core Update v2.2.2-20231121 (#non-privacy) - MANDATORY DEADLINE 15/12/2023

- turned DNX from privacy to non-privacy token
- transparent input/output wallet addresses
- transparent input/output amounts
- added wallet input/output data & amounts to node RPC request "gettransaction" (and derivates)
- added node RPC request "gettransactionsbyaddress" (returns json with all transactions belonging to an address)
- added node RPC request "getbalanceofaddress" (returns balances as well as wallet type legacy/non-private)
- added node RPC request "validatetransaction" (returns json with all addresses and amounts = transaction proof)
- updated CLI wallet to support non-privacy transactions
- updated GUI wallet to support non-privacy transactions
- updated wallet service to support non-privacy transactions
- updated Dynex blockchain explorer to support search for wallets
- updated Dynex blockchain explorer to include input/output wallets in transactions and wallet lookup
- [Node RPC functions](https://dynexcoin.org/learn/guide-dynex-node-rpc)
- [walletd RPC functions](https://dynexcoin.org/learn/guide-dynex-walletd-rpc)

## Release Notes Version 2.2.2(rev.B)

- added mnemonic phrase import, export, new wallet creation;
- added private keys export and import;
- added connection to remote node;
- added connection configuration menu with ip binding and ports selection;
- added wallet rescan option;
- added transaction fee option;
- added payment id support to address book;
- added address book edit options;
- added address book labels to wallets in transactions;
- added hash column to transactions;
- added payment id to transactions info;
- added right-click menus to address book and transactions;
- added autobackup;
- added open recent wallets option;
- added open log option;
- added node info msgs on splash screen (builtin node mode)
- improved startup time;
- improved help and about windows;
- updated copyright info;



