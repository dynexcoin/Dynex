# TuringX - Next Generation Platform for Neuromorphic Computing

With the end of Moore’s law approaching and Dennard scaling ending, the computing community is increasingly looking at new technologies to enable continued performance improvements. A neuromorphic computer is a nonvon Neumann computer whose structure and function are inspired by biology and physics. Today, such systems can be built and operated using existing technology, even at scale, and are capable of outperforming current quantum computers.

TuringX is a next-generation platform for neuromorphic computing based on a new flexible blockchain protocol. It is designed for the development of software applications and algorithms that utilize neuromorphic hardware and are capable of accelerating computation. To accomplish this goal, the platform connects hosts that are running clusters of neuromorphic chips with users and applications that utilize this next-generation hardware. On the TuringX platform, computation time is exchanged for the TuringX native token. 

TuringX has also developed a proprietary circuit design, the TuringX Neuromorphic Chip, that complements the TuringX ecosystem and turns any modern field programmable gate array (FPGA) based chip into a neuromorphic computing chip that can perform orders of magnitude faster than classical or quantum methodologies for a wide range of applications. Due to the dominance of ASICs in the proof-of-work token mining industry, there is a large amount of dormant FPGA infrastructure available which can be converted into high performance next-generation neuromorphic computing clusters.

For more information, read our [TuringX White Paper](https://github.com/TuringXplatform/TuringX-Whitepaper)

## The TuringX mainnet is live!
We are working hard on releasing all repositories. In the meantime you can connect to the mainnet by using our pre-compiled releases:

You need to have the [Boost library](https://www.boost.org) (Version 1.74.0) installed: 
```
sudo apt-get install libboost-all-dev
```

Download and extract the executables from this repository: 
```
git clone https://github.com/TuringXplatform/TuringX.git
```

To run a full Node in the TuringX blockchain, run the main service (=daemon) with the following command and wait until your node is fully synchronised with the network:
```
./turingxd
```

From the command line, you can also create and manage your personal wallet to mine and transact your TRGX tokens (make sure you have the main service daemon running):

```
./simplewallet
```

Then just follow the commands (open an existing wallet or generate a new one).

You can also start minig from within the wallet. Type the command "start_mining <number_of_threads>" to start mining TRGX. The command "stop_mining" will stop the mining procedure. You can follow your hashrate in the main service daemon with the command "show_hr".

Typing "help" (both in the wallet and in the main service daemon) displays all available functions and features. Please make sure you exit these gracefully by typing in the command "exit".

There's also an App to manage your wallet and your transactions, you can find it [here](https://github.com/TuringXplatform/TuringX-Wallet-App).

