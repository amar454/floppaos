#ifndef FILESYSTEM_H
#define FILESYSTEM_H

// Structure representing a file
struct File {
    char name[50]; // Example file name
   
};

// Structure representing the file system
struct FileSystem {
    struct File files[100]; // Array of files
    int file_count;          // Count of files in the system
};

// Function declarations
void list_files(struct FileSystem *fs, int colored); // Updated declaration
void create_file(struct FileSystem *fs, char *filename); // Complete declaration
void remove_file(struct FileSystem *fs, char *filename); // Complete declaration

#endif // FILESYSTEM_H
