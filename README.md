<div style="text-align: center;">  
  <img src="https://github.com/amar454/floppaos/blob/main/floppaOS_logo.jpeg?raw=true" alt="floppaOS Logo" width="100">  
</div>  

# **floppaOS**  

```
  __ _                          ___  ____  
 / _| | ___  _ __  _ __   __ _ / _ \/ ___| 
| |_| |/ _ \| '_ \| '_ \ / _` | | | \___ \ 
|  _| | (_) | |_) | |_) | (_| | |_| |___) |
|_| |_|\___/| .__/| .__/ \__,_|\___/|____/ 
            |_|   |_|                             
```  

### **Copyright © Amar Djulovic 2024, 2025**  

**floppaOS** is a free and open-source 32-bit operating system written in C, built entirely from scratch—except for the GNU GRUB bootloader.  

This project is meant to be the **magnum opus** of my programming portfolio, created both for job opportunities and my deep interest in low-level programming.  

I chose **not** to conform to UNIX standards, which are common in OS development. My reasoning? **GNU/Linux already exists and represents the peak of UNIX operating systems.** Rather than competing with that, I aim to build something distinct from the usual UNIX-based systems.  

Obviously, **floppaOS** is not intended for production use in its current state. OS development is **time-consuming**, and updates will be **inconsistent**, especially during this alpha stage.  

**Contributions and help are always welcome!** If you're interested, feel free to fork the project, submit pull requests, or reach out on Discord: **@amarat**.  

Thank you for checking out **floppaOS**—and if you read all this, I appreciate it!  

---

## **Compiling Instructions**  

> *Note: If you're on Windows, you'll need a UNIX-like environment such as Cygwin to compile the source code.*  

### **1. Clone the Repository**  
Run the following command in your terminal:  
```bash
git clone https://github.com/amar454/floppaos.git
```

### **2. Build the OS**  
Navigate to the repository directory and compile with:  
```bash
make all
```  
This will generate an **ISO image**, which can be attached to a **virtual machine** or written to a **USB device**.  

### **2a. Build and Run in QEMU** *(Recommended)*  
To compile and immediately launch the OS in **QEMU**, use:  
```bash
make qemu
```  
**QEMU** is the only vm floppaos is compatible with, due to issues with multiboot mmap on anything else... sorry

**3. Clean Up**  
To remove compiled binaries:  
```bash
make clean
```  

### **4. Clean Object Files** *(Without Deleting the ISO)*  
To remove C and assembly object files while keeping the ISO:  
```bash
make cleanobj
```  
*(This is the recommended step after compilation.)*  

---

## **Feature Overview**  

✅ Simple and lightweight command-line OS  
✅ Custom scripting language (**fshell**)  
✅ Memory handler  
✅ VGA text mode & graphics *(graphics WIP)*  
✅ Task handler  
✅ Basic file system  
✅ Command-line scientific calculator *(some functions WIP, wrapper not yet complete)*  

---

## **Project Goals**  

- [x] **Runs on minimal hardware**  
- [x] **Completely built from scratch**  
- [ ] **Uniqueness factor** *(Still in progress)*  
- [ ] **Thorough documentation**  
- [ ] **Focus on security through cryptography**  
- [x] **Small OS size** *(As of 12/13/2024, floppaOS is only **27.65MB**!)*  
- [ ] **Unique file system with permissions** *(Not POSIX-compliant)*  

---