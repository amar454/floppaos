<div style="text-align: center;"> <img src="https://github.com/amar454/floppaos/blob/main/floppaOS_logo.jpeg?raw=true" alt="floppaOS Logo" width="100"> </div>

# floppaOS 

```
 ____     ___                                        _____    ____      
/\  _`\  /\_ \                                      /\  __`\ /\  _`\     
\ \ \L\_\\//\ \      ___    _____    _____      __  \ \ \/\ \\ \,\L\_\   
 \ \  _\/  \ \ \    / __`\ /\ '__`\ /\ '__`\  /'__`\ \ \ \ \ \\/_\__ \   
  \ \ \/    \_\ \_ /\ \L\ \\ \ \L\ \\ \ \L\ \/\ \L\.\_\ \ \_\ \ /\ \L\ \ 
   \ \_\    /\____\\ \____/ \ \ ,__/ \ \ ,__/\ \__/.\_\\ \_____\\ `\____\
    \/_/    \/____/ \/___/   \ \ \/   \ \ \/  \/__/\/_/ \/_____/ \/_____/
                              \ \_\    \ \_\                             
                               \/_/     \/_/                             
```

### Copyright Amar Djulovic 2024

**floppaOS** is a free and open-source 32-bit operating system made in C. Everything is coded by me from scratch, aside from the GNU GRUB bootloader.

This project is intended to be the "magnum opus" of my programming portfolio. I am making it for both job opportunities and personal interest in low-level programming.

Obviously, this will not be a production or professional operating system in its current state. I chose not to conform to UNIX standards, which are common in OS development, as I am focusing on creating something unique. In my view, GNU/Linux already exists and represents the peak of UNIX operating systems, so I don’t see the need to compete with that. Instead, I want to create something that stands apart from the typical UNIX-based systems.

Operating system programming is a tough and time-consuming endeavor. As a result, updates and new versions will be inconsistent, especially while the operating system is still in its alpha stage.

Help and contributions are highly appreciated! This is a time-consuming project, and I have my own career and life as well. Feel free to fork the project, submit pull requests, or contact me on my Discord at **@amarat**.

Thank you for checking out **floppaOS**, and for reading my little note (if you did).

---

## Compiling Instructions:

1. **Clone the Repository**  
   To compile from source, clone the repository by running the following command in your terminal:  
   ```bash
   git clone https://github.com/amar454/floppaos.git
   ```

2. **Build the OS**  
   Navigate to the repository directory and type:  
   ```bash
   make all
   ```  
   *This will generate the ISO image, which you can then attach to a virtual machine or write to a USB device.*

3. **Clean Up**  
   After building, you can optionally clean up the binaries by running:  
   ```bash
   make clean
   ```  
   
4. **Clean Object Files**  
   If you want to keep the ISO binary but remove C and assembly object files, run:  
   ```bash
   make cleanobj
   ```  
   *(This is the recommended action after compilation.)*

---

## Feature Overview:

- Simple and lightweight command-line operating system
- Custom command-line scripting language (fshell)
- Memory handler
- VGA text mode and graphics (graphics work in progress)
- Task handler
- Basic file system
- Command-line scientific calculator (some functions in progress, wrapper not yet complete)

---

## Goals:

- [x] **Run on low hardware requirements**
- [x] **Completely built from scratch**
- [ ] **Uniqueness factor**  
- [ ] **Thorough documentation**
- [ ] **Focus on security through cryptography**
- [x] **Small OS size** (As of 12/13/2024, floppaOS is only **27.65MB**!)
- [ ] **Unique file system with permissions** (Not POSIX)

---
