# lab-esp-and-aimbot #

Walkthrough of an ESP and aimbot cheat from scratch for the open-source and free game [Assault Cube](https://assault.cubers.net/).<br>
This is written solely for educational purposes. Please refer to the licenses of all software involved and act in accordance with your governing law.<br>
Reverse engineering and modification of software is an advanced subtopic of CompSci and videogames provide a great medium for learning and practicing these skills. Always hack responsibly.<br>

**Anything built alongside this lab should only be used offline against bots. Don't ruin the enjoyment of others.**<br>
The code provided by this lab will only work against bots, not against real players.

## About Rootkit Education ##

This lab and the accompanying guide were written in collaboration with RootKit, a non-profit organization dedicated to certifying and educating the next generation of aspiring developers in the Computer Science realm through advanced courses and exams. Check out [their github](https://github.com/RootKit-Org) or join [their discord](https://discord.gg/rootkit) to learn more.

## Prerequisites ##

This lab is intended to be the "final step" of the accompanied [intro to gamehacking guide](https://github.com/kotae4/intro-to-gamehacking) so if you've completed the guide you should meet all of the knowledge-based prerequisites already.<br>
It's also recommended that you go through the previous labs before attempting this one as they act as "building blocks" and further reinforce the concepts covered in the guide:
1. [lab-reversing-structures](https://github.com/kotae4/lab-reversing-structures)
2. [lab-hooking-testbed](https://github.com/kotae4/lab-hooking-testbed)

### Knowledge ###

1. Knowledge of C/C++
2. Knowledge of the Portable Executable format
3. Knowledge of what processes are and the role of the Windows Image Loader
2. Knowledge of at least one injection technique
3. Knowledge of at least one hooking technique
4. Basic knowledge of x86 assembly language
5. Conceptual knowledge of ESP and aimbot cheats
    * What it takes to draw boxes around players and make your player face a certain direction

### Software ###

1. A disassembler for static analysis (preferably one with a decompiler too)
    * This walkthrough will use IDA but [Ghidra](https://github.com/NationalSecurityAgency/ghidra) is a great free, open-source alternative.
2. A live memory viewer/editor (preferably w/ built-in disassembler and debugger)
    * This walkthrough will use [cheat engine](https://www.cheatengine.org/)
    * [ReClass.NET](https://github.com/ReClassNET/ReClass.NET) will also be used to better examine data structures
3. Visual Studio. [Community edition](https://visualstudio.microsoft.com/vs/community/) is fine.
4. [vcpkg](https://github.com/microsoft/vcpkg)
5. [Assault Cube official v1.3.0.2 binaries](https://github.com/assaultcube/AC/releases/tag/v1.3.0.2)
    * You only need to download `AssaultCube_v1.3.0.2_LockdownEdition.exe` and install
6. [Assault Cube v1.3.0.2 source code](https://github.com/assaultcube/AC/tree/v1.3.0.2)

Note: v1.3.0.2 is the latest version of assault cube at the time this was written, but it seems development has picked up again so there may be newer versions when you read this. If you want to follow the walkthrough portion of this lab in a one-to-one manner it's important that you download v1.3.0.2 even if it's older.

## Objectives ##

1. To reinforce many concepts covered in the [intro to gamehacking guide](https://github.com/kotae4/intro-to-gamehacking)
2. To gain practical experience with reverse engineering and development of modifications
3. To dispel some of the 'magic' surrounding the development of game cheats
4. To ignite your passion by building something that actually **does stuff**

## Setup Instructions ##

Besides installing and setting up all of the software listed in the prerequisites section, this project will also make use of the [minhook hooking library](https://github.com/TsudaKageyu/minhook) which can be installed globally via vcpkg.<br>
That process will be detailed here:
1. Install vcpkg ([official instructions](https://github.com/microsoft/vcpkg#quick-start-windows)).
2. (optional) Add vcpkg to your PATH environment variable
3. Integrate vcpkg with visual studio (`.\vcpkg\vcpkg integrate install` in elevated command prompt)
4. Install minhook via vcpkg
    * `.\vcpkg\vcpkg install minhook` or just `vcpkg install minhook` if added to PATH
    * The included project uses the x86-windows-static triplet, so the command would be `vcpkg install minhook:x86-windows-static`.

If you installed vcpkg to your system drive (eg: `C:\dev`) and integrated with visual studio correctly then you'll be able to simply `#include <MinHook.h>` in all current and future visual studio projects.

## Table of Contents ##

1. <a href="Chapter 1 - Defining Objectives %26 Gathering Information">Chapter 1 - Defining Our Objectives & Gathering Information</a>
2. <a href="Chapter 2 - Static Analysis - Exploring The Game's Executable File">Chapter 2 - Static Analysis - Exploring The Game's Executable File</a>
3. <a href="Chapter 3 - Dynamic Analysis - Confirming Our Findings">Chapter 3 - Dynamic Analysis - Confirming Our Findings</a>
4. <a href="Chapter 4 - Writing The ESP">Chapter 4 - Writing The ESP</a>
5. <a href="Chapter 5 - Writing The Aimbot">Chapter 5 - Writing The Aimbot</a>
6. <a href="Chapter 6 - Conclusion">Chapter 6 - Conclusion</a>