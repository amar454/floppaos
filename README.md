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

FloppaOS, or the Flopperating System is a lightweight, multitasking, and simplicity focused os.
It features a monolithic kernel with a self-documented, diagram oriented, and easy to read code style
is intended to be easy to approach and study for beginners. 
 
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

✅ Memory allocation (pmm, vmm, paging, heap)
✅ Task scheduling (smp) 
✅ Multiboot 1   
✅ Tmpfs
✅ Obviously an IDT and GDT :^)
✅ Full graphical support 
✅ Flanterm 

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