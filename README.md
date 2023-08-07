# Dynex [DNX] - Next Generation Blockchain for Neuromorphic Computing

The Dynex platform represents a cutting-edge neuromorphic computing platform, built upon a revolutionary adaptable blockchain system. Comprising of collaborating miners, this decentralized neuromorphic supercomputing network excels in executing calculations with unparalleled speed and efficiency, surpassing quantum computing limitations.

The Dynex platform is capable of performing quantum computing based algorithms without their limitations, significantly accelerating machine learning training, optimizing feature selection and improving model accuracy. The Dynex Neuromoprohic Computing platform connects mining pool hosts that are running clusters of Neuromorphic chips with users and applications that utilize this next-generation hardware. On the Dynex platform, computation time spent by individuals mining is exchanged for the Dynex native token.

DynexSolve is the proprietary mining algorithm that addresses real-life computational tasks by applying Proof-of-useful-Work (PoUW) throughout the mining process. Dynex turns any modern GPU into a simulated neuromorphic computing chip that can perform orders of magnitude more efficient than classical or quantum methodologies for a wide range of applications. Due to the dominance of ASICs in the proof-of-work token mining industry, there is a large amount of dormant GPU infrastructure available which can be converted into high performance next-generation Neuromorphic computing clusters. 

The Dynex mainnet started on September 16th 2022.

For more information, read our [Dynex Whitepapers](https://dynexcoin.org/learn/dynex-whitepapers)


## Receive, Send and Manage your DNX coins

There are multiple ways to receive, send and manage your Dynex coins. Since the coins are stored on the blockchain and not the wallet software, by saving your seed words you can use different options at any time. Each wallet has their own benefits and each user must evaluate what is best for their intended use and specific needs. Advanced users are encouraged to build from source using the documentation included in this repository. 

Users on a mobile phone may consider the Dynex Mobile Web Wallet: [https://dynexcoin.org/get-dnx/#wallets](https://dynexcoin.org/get-dnx#wallets)

Users with a Personal Computer can use the convenient GUI based Dynex Wallet App. You can find it in the dedicated repository: https://github.com/dynexcoin/Dynex-Wallet-App"

Building and running a full node is advised for any individual user seeking to contribute the most to the health and growth of the Dynex network. 

Remember to save your seed phrase offline in a secure location. It is good practice to send a test transaction first, before sending large amounts. There is no way to recover lost funds, so always use caution when handling decentralized cryptocurrencies.


## Earning Dynex with your GPU using commerical grade software

To maximize the efficiency of your computer's hardware, miners may consider using closed-source software for commerical grade preformance. Though these products are not open source (and therefor pripriotory) they are still valued for being actively developed and having increased hashrate compared to open-source.

NVIDIA: https://github.com/OneZeroMiner/onezerominer/releases
AMD: https://github.com/doktor83/SRBMiner-Multi/releases

## Open source mining DNX from a command line terminal (precompiled binaries)

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

![TuringX-Daemon](https://github.com/dynexcoin/Dynex/raw/main/turingxd.jpg)

From the command line, you can also create and manage your personal wallet to mine and transact your DNX tokens (make sure you have the main service daemon running):

```
./simplewallet
```

![TuringX-Daemon](https://github.com/dynexcoin/Dynex/raw/main/simplewallet.jpg)

Then just follow the commands (use "O" to open an existing wallet or "G" to generate a new wallet for your DNX).

## DynexSolve Mining Software

To run the Dynex Solve mining software, use the following command (check with your pool operator for the stratum details):

```
Linux based systems:
./dynexsolve -mining-address <WALLET ADDRESS> -no-cpu -stratum-url <POOL> -stratum-port <POOL> -stratum-password <POOL> (-multi-gpu)

Windows based systems:
dynexsolvevs -mining-address <WALLET ADDRESS> -no-cpu -stratum-url <POOL> -stratum-port <POOL> -stratum-password <POOL> (-multi-gpu)
```

Notes:
* DynexSolve binaries are currently available for Windows and Linux
* You can also build DynexSolve from source: https://github.com/dynexcoin/DynexSolve

Note that the miner output shows computation speed, number of chips which are simulated, etc. Information about mining rewards can be observed in your wallet. When you start the DynexSolve miner, it will by default the GPU with device ID zero (the first installed one). You can specify another GPU if you like by using the command line parameter “-deviceid <ID”. To query the installed and available devices, you can use the command line option “-devices” which will output all available GPUs of your system and the associated IDs. A list of all available commands can be retrieved with the option “-h”. If you are running on Linux, DynexSolve will also generate a json file with the most recent statistics. These can be used with any mining farm management tool. Here is an overview of all DynexSolve command line options:

```
usage: dynexsolve -mining-address <WALLET ADDR> [options]

-mining-address <WALLET ADDR>    wallet address to receive the rewards
-daemon-host <HOST>              RPC host address of dynexd (default: localhost)
-daemon-port <PORT>              RPC port of dynexd (default: 18333)
-stratum-url <HOST>              host of the stratum pool
-stratum-port <PORT>             port of the stratum pool
-stratum-paymentid <PAYMENT ID>  payment ID to add to wallet address
-stratum-password <PASSWORD>     stratum password (f.e. child@worker1)
-stratum-diff <DIFFICULTY>       stratum difficulty
-no-cpu                          run no Dynex chips on CPU
-no-gpu                          run no Dynex chips on GPU (WARNING: MINING NOT POSSIBLE)
-mallob-endpoint <IP>            set the endpoint for the Dynex Malleable Load Balancer
-devices                         show GPU devices on this system
-deviceid <GPU ID>               which GPU to use (default: 0 = first one) when using 1 GPU
-multi-gpu                       uses all GPUs in the system (default: off)
-disable-gpu <ID,ID,ID>          disable certain GPUs (check -devices for IDs) when using multi-gpu
-maximum-chips <JOBS>            set maximum number of parallel Dynex Chips to be run on GPU (default: INT_MAX)
-steps-per-batch <STEPS>         set number of steps per batch (default: 10000, min 10000)
-start-from-job <JOB_NUM>        set the starting job number (default: 0)
-cpu-chips <INT>                 set number of CPU Dynex-Chips to run (default: 4)
-alpha <DOUBLE>                  set alpha value of ODE
-beta <DOUBLE>                   set beta value of ODE
-gamma <DOUBLE>                  set gamma value of ODE
-delta <DOUBLE>                  set detla value of ODE
-epsilon <DOUBLE>                set epsilon value of ODE
-zeta <DOUBLE>                   set zeta value of ODE
-init_dt <DOUBLE>                set initial dt value of ODE
-debug                           enable debugging output
-test <INPUTFILE>                test Dynex Chips locally
-mallob-debug                    enables debugging of MPI
-adj <DOUBLE>                    adjust used mem amount (default: 1.5)
-sync                            use cuda streams sync (reduce cpu usage)
-skip                            skip GPU state (.BIN) save/restore
-h                               show help
```

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



