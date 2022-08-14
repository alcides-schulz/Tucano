# Tucano
Tucano Chess Engine ![alt text](image/tucano.bmp "Tucano")

This is my chess engine Tucano (Toucan in english). It is the name of a colorful bird with an oversized bill, and is popular in my country Brazil. 
I'm an IT professional that likes chess and programming, so I combined both with the development of the development of a chess engine. 
I used the information available on the internet, specially, other engines source code, such as Fruit, Crafty, Stockfish, Rodent, Olithink, Sungorus, Tscp, Ethereal, Demolito and Xiphos.
Thanks to all developers that made this knowledge available.
The main source of information for development of chess engines can is the Chess Programming Wiki Pages (https://www.chessprogramming.org/Main_Page).
In case of any question please send me an email (alcides_schulz@hotmail.com) or stop by talkchess.com.

Tucano can be downloaded from github: https://github.com/alcides-schulz/Tucano

Neural Network Evaluation
-------------------------
Starting with version 10 release, Tucano uses a neural network evaluation, which increases the engine strength. This network architecture is based on stockfish neural network.
From version 10.06 and later, the network has been changed and use a new structure of 768x128x1. Also includes its own network trainer. The weights are embeeded in the file eval_nn.h.

It is recommend to run with the neural network evaluation, othwerwise the performance will not be much better than previous version 9.

Terms of use
------------
Tucano is free, and distributed under the GNU General Public License (GPL). Essentially, this means that you are free to do almost exactly what you want with the program, including distributing it among your friends, making it available for download from your web site, selling it (either by itself or as part of some bigger software package), or using it as the starting point for a software project of your own.

The only real limitation is that whenever you distribute Tucano in some way, you must always include the full source code, or a pointer to where the source code can be found. If you make any changes to the source code, these changes must also be made available under the GPL.

For full details, read the copy of the GPL found in the file named copying.txt.

Notice: this is free software, there is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

Have Fun.

Alcides Schulz.

Running Tucano
--------------
Version 8.0 and earlier supports XBoard protocol.
Starting from version 9.00 it supports UCI protocol, which is the recommended protocol to use.

Syzygy endgame tablebases
-------------------------
Starting on version 8.00, tucano supports syzygy (option SyzygyPath). It leverages Fathom from Jon Dart.

    - Syzygy: endgame tables bases by Ronald de Man
    
    - Fathom: syzygy probing tool by Jon Dart.
    
    
UCI Protocol options
--------------------
Tucano supports the following uci options:

    option name Hash type spin default 64 min 8 max 65536
    option name Threads type spin default 1 min 1 max 256
    option name SyzygyPath type string default <empty>
    option name Ponder type check default false
    option name EvalFile type string default <empty>
    
Command Line options
--------------------
tucano -hash N -threads N -syzygy_path F

    -hash indicates the size of hash table, default = 64 MB, minimum: 8 MB, maximum: 65536 MB.
    -threads indicates how many threads to use during search, minimum: 1, maximum: 256. Depends on how many cores your computer have.
    -syzygy_path indicates the folder of syzygy endgame tablebase.
   
Signature
---------
If you compile tucano you can use the command "bench" to get a signature. Just start tucano and type "bench". 
Signature is a number generated after searching a couple of positions to indicate you have the correct compilation. 
If you don't get the correct signature it means that something is wrong with the compilation process and the program may not perform correctly.

    10.00:  5734637 (with nn eval file loaded)
     9.00: 21898211
     8.00: 32406478

 Executable
 ----------
The release will come with executables for different plataforms, try to use the one that providers the most nodes per seconds. 
Start tucano and type the command "speed", you should see the following report:

        Tucano chess engine by Alcides Schulz - 10.00 (type 'help' for information)

           hash table: 64 MB, threads: 1

        speed
        running speed test 1 of 5...
        running speed test 2 of 5...
        running speed test 3 of 5...
        running speed test 4 of 5...
        running speed test 5 of 5...

        Speed test: nodes per second average = 1376k


Compilation
-----------
The main platform used for development is Windows. 
I try to use standard functions and make tucano portable, so it can be compiled on other platforms, but there's no warranty it will work on all of them.
You can report issues with other platforms and I will try to address as possible.

Here are the commands used for compilation:

Windows (compiled using mingW version 7.2.0)

        AVX2
        gcc -o tucano_avx2.exe -DEGTB_SYZYGY -DTNNAVX2 -O3 -Isrc -flto -m64 -mtune=generic -s -static -Wall -Wfatal-errors -mavx2 -msse4.1 -mssse3 -msse2 -msse src\*.c src\fathom\tbprobe.c
        
        SSE4.1
        gcc -o tucano_sse4.exe -DEGTB_SYZYGY -DTNNSSE4 -O3 -Isrc -flto -m64 -mtune=generic -s -static -Wall -Wfatal-errors -msse4.1 -mssse3 -msse2 -msse src\*.c src\fathom\tbprobe.c
        
        OLD
        gcc -o tucano_old.exe -DEGTB_SYZYGY -O3 -Isrc -flto -m64 -mtune=generic -s -static -Wall -Wfatal-errors src\*.c src\fathom\tbprobe.c


Linux and ARM V8 (using src/makefile):
    
    cd src
    make <architeture>
          <architecture>: avx2, sse4, old

Note: It is recommend to use AVX2 or SSE in order to have a good performance with neural network evaluation. 

//END
