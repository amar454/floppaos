
# **The Flopperating System | floppaOS**  

<img width="1011" height="145" alt="image" src="https://github.com/user-attachments/assets/7b9eacfb-147e-490f-ab99-579e631d7ba6" />

### **Copyright © Amar Djulovic 2024, 2025**  

#### The Flopperating System, or (formally) floppaOS is a lightweight, monolithic, and multitasking hobby operating system. It targets the i386 platform. It is all designed by me and all code was made by me. My goal is simplicity, performance, ease of use, code readabiliy, and nice namespacing. This project is my prized possession and will most likely be a thesis for college.

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

- [x] Memory allocation (pmm, vmm, paging, heap)
- [x] Task scheduling (smp) 
- [x] Multiboot 1   
- [x] Tmpfs 
- [x] vfs
- [x] Full graphical support 
- [x] Flanterm 

- [ ] if youre wondering if i have a gdt, you're in the wrong place

---


