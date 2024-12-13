# <img src="https://github.com/amar454/floppaos/blob/main/C999A980-DD33-41B2-8C9E-941D8E16A5A7.jpeg?raw=true" width="100">
# floppaOS 

### Copyright Amar Djulovic 2024

floppaOS is a free and open source 32-bit operating system made in C. Everything is coded by me from scratch besides the GNU GRUB bootloader I'm using.

This project is intended to be the "magnum opus" of my programming portfolio. I am making it for job opportunities and personal interest in low level programming.

Obviously, this will not be a production or professional operating system with the current way I'm doing things. I wished to avoid confirming to UNIX standards, which is common for operating system development. This is because I am focusing on a uniqueness factor. In my view, GNU/Linux already exists and that's the peak of UNIX operating system. Why would I try to beat something that has already been done better than I ever could? I wish to seperate my operating system from UNIX standards for that exact reason.

Operating system programming is really difficult and takes lots of time. For that reason, updates and new versions will unfortunately be inconsistent especially while the operating system is in the alpha stage of development, which it is now.

Help and contributions would be highly appreciated. This is a time consuming project and I obviously have my own career and life as well, so please feel free to fork, pull request, and message me on my discord account @amarat.

Thank you for checking out floppaOS, and reading my little note (if you did)


## Compiling instructions:
1. To compile from source, clone the reprository by typing `git clone https://github.com/amar454/floppaos.git`.

2. Next, open the reprository directory in a terminal and type `make all`.

* <i>This will make the ISO image which you can then attach to your virtual machine or USB device.</i>

3. Optionally, after typing `make all`, you can type `make clean` to get rid of all binaries. 

4. Additionally `make cleanobj` keeps the ISO binary but gets rid of C and asm object files. (recommended action after compilation)


## Feature Overview:
* Simple and lightweight command line operating system
* Custom command line scripting language (fshell)
* Memory handler
* VGA text mode and graphics (graphics in progress)
* Task handler
* Basic file system

## Goals:
- [x] Run on low hardware requirements
- [x] Make completely from scratch
- [ ] Uniqueness factor
- [ ] Thorough documentation
- [ ] Focus on security through cryptography
- [x] Low operating system size (as of 12/13/2024, floppaOS is only 27.65MB!!)
- [ ] Unique file system with permissions (not POSIX)
