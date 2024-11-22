#include <ramfs.h>

// Structure definition for a file in RAM filesystem
typedef struct {
    char name[MAX_NAME_LENGTH];  // Name of the file
    uint8_t data[MAX_FILE_SIZE]; // Actual content of the file
    size_t size;                 // Size of the file content
    bool is_directory;           // Flag to indicate if it's a directory
} RamFile;

// Array to store all files in the RAM filesystem
RamFile ramfs[MAX_FILES];
// Counter to keep track of the number of files
int file_count = 0;

// Initialize the RAM filesystem
void init_ramfs() {
    file_count = 0;  // Reset the file count to 0
}

// Create a new file or directory in the RAM filesystem
int ramfs_create(const char* name, bool is_directory) {
    // Check if the filesystem is full
    if (file_count >= MAX_FILES) {
        return -1;  // Return error if no more files can be created
    }
    
    // Get a pointer to the new file structure
    RamFile* file = &ramfs[file_count];
    strncpy(file->name, name, MAX_NAME_LENGTH);  // Copy the file name
    file->size = 0;  // Initialize size to 0
    file->is_directory = is_directory;  // Set directory flag
    
    return file_count++;  // Return the file ID and increment the count
}

// Write data to a file in the RAM filesystem
int ramfs_write(int file_id, const void* buffer, size_t size) {
    // Check if the file ID is valid and it's not a directory
    if (file_id < 0 || file_id >= file_count || ramfs[file_id].is_directory) {
        return -1;  // Return error if invalid
    }
    
    RamFile* file = &ramfs[file_id];  // Get pointer to the file
    // Determine how much data can be written
    size_t write_size = size > MAX_FILE_SIZE ? MAX_FILE_SIZE : size;
    memcpy(file->data, buffer, write_size);  // Copy data to file
    file->size = write_size;  // Update file size
    
    return write_size;  // Return the number of bytes written
}

// Read data from a file in the RAM filesystem
int ramfs_read(int file_id, void* buffer, size_t size) {
    // Check if the file ID is valid and it's not a directory
    if (file_id < 0 || file_id >= file_count || ramfs[file_id].is_directory) {
        return -1;  // Return error if invalid
    }
    
    RamFile* file = &ramfs[file_id];  // Get pointer to the file
    // Determine how much data can be read
    size_t read_size = size > file->size ? file->size : size;
    memcpy(buffer, file->data, read_size);  // Copy data from file to buffer
    
    return read_size;  // Return the number of bytes read
}

// Print information about a file
void print_file_info(int file_id) {
    // Check if the file ID is valid
    if (file_id < 0 || file_id >= file_count) {
        println("Invalid file ID", 15);
        return;
    }
    
    RamFile* file = &ramfs[file_id];  // Get pointer to the file
    // Print file details
    print("File: ", 15);
    println(file->name, 15);
    print(" Size: ", 15);
    char size_str[10];
    itoa(file->size, size_str, 10);  // Convert size to string
    println(size_str, 15);
    print(" Content: ", 15);
    println((char*)file->data, 15);
}

// Test function for the RAM filesystem
void test_ramfs() {
    init_ramfs();  // Initialize the filesystem
    println(":: RamFS Test Start", 15);
    
    // Create a file
    println(":: Creating file 'test.txt'...", 15);
    int file_id = ramfs_create("test.txt", false);
    if (file_id < 0) {
        println(":: Failed to create file", 15);
        return;
    }
    println(":: File created successfully", 15);
    print_file_info(file_id);
    
    // Write data to the file
    println(":: Writing data to file...", 15);
    const char* test_data = "RamFS test";
    int write_size = ramfs_write(file_id, test_data, strlen(test_data));
    if (write_size < 0) {
        println(":: Failed to write to file", 15);
        return;
    }
    println(":: Data written successfully", 15);
    print_file_info(file_id);
    
    // Read data from the file
    println(":: Reading data from file...", 15);
    char buffer[MAX_FILE_SIZE];
    int read_size = ramfs_read(file_id, buffer, MAX_FILE_SIZE);
    if (read_size < 0) {
        println(":: Failed to read from file", 15);
        return;
    }
    buffer[read_size] = '\0';  // Null-terminate the string
    
    println(":: Data read successfully", 15);
    print(":: Read content: ", 15);
    println(buffer, 15);
    
    // Verify the read data matches the written data
    if (strcmp(buffer, test_data) == 0) {
        println(":: RamFS test passed successfully!", 15);
    } else {
        println(":: RamFS test failed!", 15);
    }
    
    // Create another file
    println("::Creating another file 'example.txt'...", 15);
    int another_file_id = ramfs_create("example.txt", false);
    if (another_file_id < 0) {
        println(":: Failed to create second file", 15);
        return;
    }
    const char* another_data = "This is another file in RamFS";
    ramfs_write(another_file_id, another_data, strlen(another_data));
    println(":: Second file created and written", 15);
    print_file_info(another_file_id);
    
    // Display a list of all files
    println(":: List of all files in RamFS:", 15);
    for (int i = 0; i < file_count; i++) {
        print_file_info(i);
        println("", 15);  // Blank line for separation
    }
    
    println(":: RamFS Test End", 15);
}