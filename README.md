# Dynex [DNX] - Next Generation Blockchain for Neuromorphic Computing

With the end of Moore’s law approaching and Dennard scaling ending, the computing community is increasingly looking at new technologies to enable continued performance improvements. A neuromorphic computer is a nonvon Neumann computer whose structure and function are inspired by biology and physics. Today, such systems can be built and operated using existing technology, even at scale, and are capable of outperforming current quantum computers.

Dynex is a next-generation platform for neuromorphic computing based on a new flexible blockchain protocol. It is designed for the development of software applications and algorithms that utilize neuromorphic hardware and are capable of accelerating computation. To accomplish this goal, the platform connects hosts that are running clusters of neuromorphic chips with users and applications that utilize this next-generation hardware. On the Dynex platform, computation time is exchanged for the Dynex native token. 

Dynex has also developed a proprietary circuit design, the Dynex Neuromorphic Chip, that complements the Dynex ecosystem and turns any modern field programmable gate array (FPGA) based chip into a neuromorphic computing chip that can perform orders of magnitude faster than classical or quantum methodologies for a wide range of applications. Due to the dominance of ASICs in the proof-of-work token mining industry, there is a large amount of dormant FPGA infrastructure available which can be converted into high performance next-generation neuromorphic computing clusters. The Dynex mainnet started on September 16th 2022.

For more information, read our [Dynex White Paper](https://github.com/dynexcoin/Dynex-Whitepaper)



## Using the Dynex Hosted Web Wallet to manage your DNX

Users who dont want to run a node or dont want to install anything can use the Dynex Mobile Web Wallet: 
https://dynexcoin.org/get-dnx/#wallets

## Using the Dynex-App to manage your DNX

Users who just want to use the Dynex wallet functionality to create wallets or send and receive DNX are recommended to use the convenient GUI based app. You can find it in the dedicated repository: https://github.com/dynexcoin/Dynex-Wallet-App 

## Mining DNX and managing DNX from the command line (precompiled binaries)

The easiest way to start minging DNX or to manage DNX wallet(s) from the command line in the terminal is by using our precompiled binaries. Download the version matching your operating system from our releases page:
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

After downloading the precompiled binary, unzip the executable on your machine. To run a full Node in the TuringX blockchain, run the main service (=daemon) with the following command and wait until your node is fully synchronised with the network:
```
./dynexd
```

![TuringX-Daemon](https://github.com/dynexcoin/Dynex/raw/main/turingxd.jpg)

From the command line, you can also create and manage your personal wallet to mine and transact your TRGX tokens (make sure you have the main service daemon running):

```
./simplewallet
```

![TuringX-Daemon](https://github.com/dynexcoin/Dynex/raw/main/simplewallet.jpg)

Then just follow the commands (use "O" to open an existing wallet or "G" to generate a new wallet for your TRGX).

## DynexSolve Mining Software

To run the Dynex Solve mining software, use the following command:

```
Linux based systems:
./dynexsolve -mining-address <WALLET ADDRESS> -no-cpu -mallob-endpoint https://dynex.dyndns.org/dynexmallob

Windows based systems:
dynexsolvevs -mining-address <WALLET ADDRESS> -no-cpu -mallob-endpoint https://dynex.dyndns.org/dynexmallob
```

Notes:
* DynexSolve binaries are currently available for Windows and Linux (built for Ubuntu 22)
* Currently supported GPUs: NVIDIA sm52 and better. AMD support coming soon
* Older Linux Versions (Ubuntu 18, Ubuntu 20): need to be built from source: https://github.com/dynexcoin/DynexSolve
* HiveOS: need to be built from source: https://github.com/dynexcoin/DynexSolve Here is a guide on how to build DynexSolve on HiveOS: https://discord.com/channels/1021103888503414834/1036973620938944612/1048610995456716871

Note that the miner output shows computation speed, number of chips which are simulated, etc. Information about mining rewards can be observed in your wallet. When you start the DynexSolve miner, it will by default the GPU with device ID zero (the first installed one). You can specify another GPU if you like by using the command line parameter “-deviceid <ID”. To query the installed and available devices, you can use the command line option “-devices” which will output all available GPUs of your system and the associated IDs. A list of all available commands can be retrieved with the option “-h”.

## Build Daemon, Simplewallet & WalletD from source

You can also entirely build all binaries from the source. First, clone the repository:
```
git clone https://github.com/dynexcoin/Dynex.git
```

### Linux

It is required to have the [Boost library](https://www.boost.org) (Version 1.74.0 or better) installed: 
```
sudo apt-get install libboost-all-dev (Ubuntu Linux)
```

To compile and build:
```
mkdir build 
cd build
cmake ..
make
```

### MacOS

It is required to have the [Boost library](https://www.boost.org) (Version 1.74.0 or better) installed: 
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

brew install boost
```

To compile and build:
```
mkdir build 
cd build
cmake ..
make
```

### Windows

1. It is required to have the [Boost library](https://www.boost.org) (Version 1.74.0 or better) installed: 
```
sudo apt-get install libboost-all-dev (Ubuntu Linux)
```

2. It is required to have [cmake](https://cmake.org/) for windows installed.
3. You also need [Microsoft Visual Studio](https://visualstudio.microsoft.com) (2017 or later) installed for the build process.

To compile and build:
```
mkdir build 
cd build
cmake ..
```
Then open Visual Studio. Make sure you have set Solution->Configuration to "Release". You also need to add "bcrypt.lib" to P2P->Properties->Configuration Properties->Librarian->Additional Dependencies. This is necessary to build boost's cryptographic randomizer functions. Once these settings are done, proceed a full build to generate your binaries.




