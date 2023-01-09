# Dynex [DNX] - Next Generation Blockchain for Neuromorphic Computing

With the end of Moore’s law approaching and Dennard scaling ending, the computing community is increasingly looking at new technologies to enable continued performance improvements. A neuromorphic computer is a nonvon Neumann computer whose structure and function are inspired by biology and physics. Today, such systems can be built and operated using existing technology, even at scale, and are capable of outperforming current quantum computers.

Dynex is a next-generation platform for neuromorphic computing based on a new flexible blockchain protocol. It is designed for the development of software applications and algorithms that utilize neuromorphic hardware and are capable of accelerating computation. To accomplish this goal, the platform connects hosts that are running clusters of neuromorphic chips with users and applications that utilize this next-generation hardware. On the Dynex platform, computation time is exchanged for the Dynex native token. 

Dynex has also developed a proprietary circuit design, the Dynex Neuromorphic Chip, that complements the Dynex ecosystem and turns any modern GPU into a simulated neuromorphic computing chip that can perform orders of magnitude more efficient than classical or quantum methodologies for a wide range of applications. Due to the dominance of ASICs in the proof-of-work token mining industry, there is a large amount of dormant GPU infrastructure available which can be converted into high performance next-generation neuromorphic computing clusters. The Dynex mainnet started on September 16th 2022.

For more information, read our [Dynex White Paper](https://github.com/dynexcoin/Dynex-Whitepaper)



## Using the Dynex Hosted Web Wallet to manage your DNX

Users who don't want to run a node or don't want to install anything can use the Dynex Mobile Web Wallet: 
https://dynexcoin.org/get-dnx/#wallets

## Using the Dynex-App to manage your DNX

Users who just want to use the Dynex wallet functionality to create wallets or send and receive DNX are recommended to use the convenient GUI based app. You can find it in the dedicated repository: https://github.com/dynexcoin/Dynex-Wallet-App 

## Mining DNX and managing DNX from the command line (precompiled binaries)

The easiest way to start mining DNX or to manage DNX wallet(s) from the command line in the terminal is by using our precompiled binaries. Download the version matching your operating system from our releases page:
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

After downloading the precompiled binary, unzip the executable on your machine. To run a full Node in the Dynex blockchain, run the main service (=daemon) with the following command and wait until your node is fully synchronised with the network:
```
./dynexd
```

Please note that your node requires ports 17336 and SSL to be open.

![TuringX-Daemon](https://github.com/dynexcoin/Dynex/raw/main/turingxd.jpg)

From the command line, you can also create and manage your personal wallet to mine and transact your TRGX tokens (make sure you have the main service daemon running):

```
./simplewallet
```

![TuringX-Daemon](https://github.com/dynexcoin/Dynex/raw/main/simplewallet.jpg)

Then just follow the commands (use "O" to open an existing wallet or "G" to generate a new wallet for your TRGX).

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
* Currently supported GPUs: NVIDIA sm52 and better. AMD support coming soon
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



