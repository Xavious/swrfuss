# New Document# SWR FUSS (Star Wars Reality) (Fixed Up Smaug Source)

## Introduction
SWR is a Star Wars themed MUD (Multi User Dungeon) codebase that  descends from SMAUG, which in turn descends from ROM and DIKU. The original source code was found on the [Smaug Muds](https://www.smaugmuds.org) website. To the uninitiated, MUDs are text based games traditionally played via telnet. The DIKU codebase is where the majority of these games are descended from; a now ancient codebase that was originally written in C.

This repository will be the staging area for me while I attempt to explore and modify this code. I welcome collaboration from anyone interested in dabbling with SWR, adding new features, or even refactoring this code to be more efficient and/or concise.

## Installation
I'm currently running this code on Arch Linux, but I've run it on several other distributions in the past. The currently staged code is actually compiled already, but generally speaking running the server requires the following steps:
- Clone this GitHub repository somewhere on your local file system.
- Go to the src directory and compile the code with the command "make", or "make clean" then "make"
- Debug any depency issues you might have with your distribution
  -  Packages: make, g++, libz-dev

### Required Packages
Testing compiling on the Linux Subsystem for Windows (Ubuntu) required the following packages to be installed in order to recompile using "make"

- apt-get install make
- apt-get install g++
- apt-get install libz-dev

## Start Server
- Move into the /area directory
- Excute the command: "../src/swreality"

The server defaults to port 4000. You should now be able to access the server by going to localhost port 4000 or, if you're using VirtualBox and a router with your local network, you will need to set up port forwarding for whatever IP address is assigned to your virtual machine through your routers interface.. whatever that may be.

## Change Log
- 2018-02-25
  - Added depdency list for Linux Subsystem for Windows (Ubuntu)
- 2017-11-12
  - Explored how skills work by adding a new level 100 combat skill called "flurry" that executes a barrage of attacks mid combat.
  - Changed the colors and outline of the character score sheet.
- 2017-11-03
  - Changed flags in the makefile to get the code to compile and ignore certain errors and warnings. Made the first Commit to GitHub
