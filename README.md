# Dynex [DNX] - Next Generation Blockchain for Neuromorphic Computing

With the end of Moore’s law approaching and Dennard scaling ending, the computing community is increasingly looking at new technologies to enable continued performance improvements. A neuromorphic computer is a nonvon Neumann computer whose structure and function are inspired by biology and physics. Today, such systems can be built and operated using existing technology, even at scale, and are capable of outperforming current quantum computers.

Dynex is a next-generation platform for neuromorphic computing based on a new flexible blockchain protocol. It consists of participating nodes that together constitute one enormous neuromorphic computing network. Consequently, the platform is capable of performing computations at unprecedented speeds and efficiency – even exceeding quantum computing. Everyone is welcome to participate, since the Dynex neuromorphic computing chip is capable of being simulated using almost any device, from regular laptops to desktop computers to GPUs, FPGAs and ASIC clusters. Users exchange computation time for Dynex’s native token DNX, thus enabling everyone to earn money on the platform.

All participating nodes together constitute one enormous neuromorphic computing network. Consequently, the platform is capable of performing computations at unprecedented speeds and efficiency – even exceeding quantum computing. Everyone is welcome to participate, since the Dynex neuromorphic computing chip is capable of being simulated using almost any device, from regular laptops to desktop computers to GPUs, FPGAs and ASIC clusters. Users exchange computation time for Dynex’s native token DNX, thus enabling everyone to earn money on the platform.

Dynex has developed a proprietary circuit design, the Dynex Neuromorphic Chip, that complements the Dynex ecosystem and turns any modern device into a neuromorphic computing chip that can perform orders of magnitude faster than classical or quantum methodologies for a wide range of applications. Especially due to the dominance of ASICs in the proof-of-work token mining industry, there is a large amount of dormant FPGA infrastructure available which can be converted into high performance next-generation neuromorphic computing clusters.

For more information, read our [Dynex White Paper](https://github.com/dynexcoin/Dynex-Whitepaper)

## The Dynex mainnet is live!
Starting from September 16th 2022, the Dynex mainnet is live. Everyone is welcome to join!

## Using the Dynex-App to manage your DNX

Users who just want to use the Dynex wallet functionality to create wallets or send and receive DNX are recommended to use the convenient GUI based app. You can find it in the dedicated repository: https://github.com/dynexcoin/Dynex-Wallet-App 

## Mining DNX and managing DNX from the command line (precompiled binaries)

The easiest way to start minging DNX or to manage DNX wallet(s) from the command line in the terminal is by using our precompiled binaries. Download the version matching your operating system:

Microsoft Windows: 
coming soon - please build from source as described below

Apple MacOS: 
https://github.com/dynexcoin/Dynex/raw/main/MacOS_executables.zip

Linux: 
https://github.com/dynexcoin/Dynex/raw/main/Ubuntu_22.04.1_executables.tar.xz

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

Note: For MacOS Silicon M1/M2 based machines only: 

You need x86-64 versions of the boost libraries, but Apple Silicon compiles for Arm. You can download and unzip our precompiled [boost x86-64 runtime libraries](https://github.com/dynexcoin/Dynex/raw/main/libboost_x86-64.zip) to run Dynex on Apple Silicon. Then you need to set the environment variable for the dynamic boost libraries with

```
export DYLD_LIBRARY_PATH=/path/to/the/library/files/:$DYLD_LIBRARY_PATH
```

After downloading the precompiled binary, unzip the executable on your machine. To run a full Node in the Dynex blockchain, run the main service (=daemon) with the following command and wait until your node is fully synchronised with the network:
```
./dynexd
```

![Dynex-Daemon](https://github.com/dynexcoin/Dynex/raw/main/turingxd.jpg)

From the command line, you can also create and manage your personal wallet to mine and transact your DNX tokens (make sure you have the main service daemon running):

```
./simplewallet
```

![Dynex-Daemon](https://github.com/dynexcoin/Dynex/raw/main/simplewallet.jpg)

Then just follow the commands (use "O" to open an existing wallet or "G" to generate a new wallet for your DNX).

You start minig directly from within your wallet: type the command "start_mining <number_of_threads>" to start mining DNX. 

``` 
start_mining 8
```

The command "stop_mining" will stop the mining procedure. 

```
stop_mining
```

You can follow your hashrate in the main service daemon with the command "show_hr".

```
show_hr
...
hide_hr
``` 

Typing "help" (both in the wallet and in the main service daemon) displays all available functions and features. Please make sure you exit these gracefully by typing in the command "exit".

``` 
exit
``` 

## Mining DNX and managing DNX from the command line (build from source)

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

You also need cmake:
```
brew install cmake
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




