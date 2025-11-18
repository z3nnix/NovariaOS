// Include standard C libraries for memory allocation, input/output, string operations, and threading
#include <stdio.h>    // For standard input/output functions like printf
#include <stdlib.h>   // For memory allocation functions like malloc, free
#include <string.h>   // For string manipulation functions like memset
#include <pthread.h>  // For thread synchronization primitives like mutexes

// Define constants for memory management configuration
#define MEMORY_POOL_SIZE (1024 * 1024)  // Total size of our memory pool: 1 megabyte
#define MIN_BLOCK_SIZE 16               // Minimum block size we'll manage (16 bytes)
#define MAX_CLASSES 20                  // Maximum number of size classes for SLAB allocation

// Define a structure to represent a memory block in our free list
// This structure is used to track allocated and free memory regions
typedef struct mem_block {
    size_t size;          // Size of this memory block in bytes
    int free;             // Flag indicating if the block is free (1) or allocated (0)
    struct mem_block *next; // Pointer to the next memory block in the linked list
} mem_block;

// Define a structure to represent a SLAB (SLab Uniform Block)
// A SLAB is a memory area that contains multiple objects of the same size
typedef struct slab {
    void *objects;        // Pointer to the actual memory area containing the objects
    int free_count;       // Number of currently free objects in this slab
    void *free_list;      // Linked list of free object pointers within this slab
    struct slab *next;    // Pointer to the next slab in the list for this size class
} slab;

// Define a structure to represent a size class for SLAB allocation
// Each size class manages objects of a specific size range
typedef struct size_class {
    size_t obj_size;      // Size of objects in this size class
    slab *slabs;          // Head of the linked list of slabs for this size class
    int free_objects;     // Total count of free objects across all slabs in this class
} size_class;

// Define the main memory manager structure
// This structure contains all components of our memory management system
typedef struct memory_manager {
    void *memory_pool;    // Pointer to the main memory pool we're managing
    size_t total_size;    // Total size of the memory pool in bytes
    mem_block *free_list; // Head of the linked list of free memory blocks (for large allocations)
    size_class classes[MAX_CLASSES]; // Array of size classes for SLAB allocation
    pthread_mutex_t lock;  // Mutex for thread synchronization
} memory_manager;

// Function to initialize the memory manager
// This sets up the initial state of the memory management system
memory_manager* init_memory_manager() {
    // Allocate memory for the memory manager structure itself
    memory_manager *mm = (memory_manager*)malloc(sizeof(memory_manager));
    if (!mm) return NULL;  // Return NULL if allocation failed
    
    // Allocate the main memory pool that we'll be managing
    mm->memory_pool = malloc(MEMORY_POOL_SIZE);
    if (!mm->memory_pool) {
        free(mm);  // Clean up the memory manager structure if pool allocation failed
        return NULL;
    }
    
    // Set the total size of the memory pool
    mm->total_size = MEMORY_POOL_SIZE;
    
    // Initialize the free list to NULL (empty)
    mm->free_list = NULL;
    
    // Initialize all size classes
    for (int i = 0; i < MAX_CLASSES; i++) {
        // Set the object size for this class: minimum block size multiplied by class index + 1
        mm->classes[i].obj_size = MIN_BLOCK_SIZE * (i + 1);
        
        // Initialize the slabs list to NULL (empty)
        mm->classes[i].slabs = NULL;
        
        // Initialize the free objects count to 0
        mm->classes[i].free_objects = 0;
    }
    
    // Initialize the mutex for thread synchronization
    pthread_mutex_init(&mm->lock, NULL);
    
    // Create the initial free block that covers the entire memory pool
    // This block is marked as free and has no next block
    mem_block *initial_block = (mem_block*)mm->memory_pool;
    initial_block->size = MEMORY_POOL_SIZE;  // Size is the entire memory pool
    initial_block->free = 1;                 // Mark as free
    initial_block->next = NULL;             // No next block yet
    mm->free_list = initial_block;           // Set this as the head of the free list
    
    // Return the initialized memory manager
    return mm;
}

// Helper function to find the appropriate size class for a requested size
// Returns the index of the size class that can accommodate the requested size
int find_size_class(size_t size) {
    // Iterate through all size classes
    for (int i = 0; i < MAX_CLASSES; i++) {
        // If the requested size is less than or equal to the object size of this class
        if (size <= mm->classes[i].obj_size) {
            return i;  // Return this class index
        }
    }
    return -1;  // Return -1 if no suitable class was found (size too large)
}

// Function to create a new slab for a given size class
// A slab is a memory area that contains multiple objects of the same size
slab* create_slab(size_class *sc) {
    // Allocate memory for the slab metadata structure
    slab *new_slab = (slab*)malloc(sizeof(slab));
    if (!new_slab) return NULL;  // Return NULL if allocation failed
    
    // Calculate the size of the slab: we'll use 1/10 of the total memory pool per slab
    size_t slab_size = MEMORY_POOL_SIZE / 10;
    
    // Calculate how many objects of this size class will fit in the slab
    int obj_count = slab_size / sc->obj_size;
    
    // Allocate memory for the actual objects from the system heap
    new_slab->objects = malloc(slab_size);
    if (!new_slab->objects) {
        free(new_slab);  // Clean up the slab metadata if object allocation failed
        return NULL;
    }
    
    // Initialize the free count for this slab
    new_slab->free_count = obj_count;
    
    // Initialize the free list to NULL (empty)
    new_slab->free_list = NULL;
    
    // Build the free list by linking all objects together
    void *current = new_slab->objects;
    for (int i = 0; i < obj_count; i++) {
        // Calculate the pointer to the next object
        void *next = (char*)current + sc->obj_size;
        
        // Store the next pointer in the current object's memory
        void **ptr = (void**)current;
        *ptr = next;
        
        // Move to the next object
        current = next;
    }
    
    // The last object should point to NULL (end of free list)
    void **last_ptr = (void*)((char*)new_slab->objects + (obj_count - 1) * sc->obj_size);
    *last_ptr = NULL;
    
    // Link the new slab to the front of the size class's slab list
    new_slab->next = sc->slabs;
    sc->slabs = new_slab;
    
    // Update the total free objects count for this size class
    sc->free_objects += obj_count;
    
    // Return the newly created slab
    return new_slab;
}

// SLUB (SLab Uniform Block) allocation function
// This function allocates memory for small objects using SLAB allocation
void* slub(memory_manager *mm, size_t size) {
    // Lock the mutex to ensure thread-safe operation
    pthread_mutex_lock(&mm->lock);
    
    // Find the appropriate size class for the requested size
    int class_idx = find_size_class(size);
    if (class_idx == -1) {
        // No suitable size class found (size is too large for SLAB allocation)
        pthread_mutex_unlock(&mm->lock);  // Unlock before returning
        return NULL;  // Return NULL to indicate allocation failure
    }
    
    // Get a pointer to the selected size class
    size_class *sc = &mm->classes[class_idx];
    
    // Check if there are any free objects in existing slabs for this size class
    if (sc->free_objects > 0) {
        // Iterate through the slabs to find one with free objects
        slab *current_slab = sc->slabs;
        while (current_slab) {
            // If this slab has free objects
            if (current_slab->free_count > 0) {
                // Get the first free object from the free list
                void *obj = current_slab->free_list;
                
                // Update the free list to point to the next free object
                void **ptr = (void**)obj;
                current_slab->free_list = *ptr;
                
                // Decrement the free count for this slab
                current_slab->free_count--;
                
                // Decrement the total free objects count for this size class
                sc->free_objects--;
                
                // Unlock the mutex before returning
                pthread_mutex_unlock(&mm->lock);
                
                // Return the pointer to the allocated object
                return obj;
            }
            current_slab = current_slab->next;
        }
    }
    
    // No free objects found in existing slabs, create a new slab
    slab *new_slab = create_slab(sc);
    if (!new_slab) {
        // Failed to create a new slab
        pthread_mutex_unlock(&mm->lock);  // Unlock before returning
        return NULL;  // Return NULL to indicate allocation failure
    }
    
    // Get the first free object from the new slab's free list
    void *obj = new_slab->free_list;
    
    // Update the free list to point to the next free object
    void **ptr = (void**)obj;
    new_slab->free_list = *ptr;
    
    // Decrement the free count for this slab
    new_slab->free_count--;
    
    // Decrement the total free objects count for this size class
    sc->free_objects--;
    
    // Unlock the mutex before returning
    pthread_mutex_unlock(&mm->lock);
    
    // Return the pointer to the allocated object
    return obj;
}

// Function to free a SLAB allocated object
// This function returns a previously allocated SLAB object to the free list
void slab_free(memory_manager *mm, void *ptr, size_t size) {
    // Lock the mutex to ensure thread-safe operation
    pthread_mutex_lock(&mm->lock);
    
    // Find the appropriate size class for this object
    int class_idx = find_size_class(size);
    if (class_idx == -1) {
        // Invalid size for SLAB allocation (shouldn't happen if allocated properly)
        pthread_mutex_unlock(&mm->lock);  // Unlock before returning
        return;  // Simply return, nothing to free
    }
    
    // Get a pointer to the selected size class
    size_class *sc = &mm->classes[class_idx];
    
    // Iterate through the slabs to find which one contains this object
    slab *current_slab = sc->slabs;
    while (current_slab) {
        // Check if the pointer is within this slab's object memory area
        if (ptr >= current_slab->objects && 
            ptr < (char*)current_slab->objects + MEMORY_POOL_SIZE / 10) {
            
            // Add the object back to the free list
            // Store the current free list head in the object's memory
            void **ptr_to_ptr = (void**)ptr;
            *ptr_to_ptr = current_slab->free_list;
            
            // Update the free list head to point to this object
            current_slab->free_list = ptr;
            
            // Increment the free count for this slab
            current_slab->free_count++;
            
            // Increment the total free objects count for this size class
            sc->free_objects++;
            
            // Unlock the mutex before returning
            pthread_mutex_unlock(&mm->lock);
            
            // Return, as we've successfully freed the object
            return;
        }
        current_slab = current_slab->next;
    }
    
    // Object not found in any slab (this shouldn't happen if allocated properly)
    // Unlock the mutex before returning
    pthread_mutex_unlock(&mm->lock);
}

// Function for general memory allocation (for large objects that don't fit in SLABs)
void* general_alloc(memory_manager *mm, size_t size) {
    // Lock the mutex to ensure thread-safe operation
    pthread_mutex_lock(&mm->lock);
    
    // Round up the requested size to align to 8-byte boundary
    // This ensures proper memory alignment for most architectures
    size = (size + 7) & ~7;
    
    // Initialize pointers for traversing the free list
    mem_block *prev = NULL;
    mem_block *current = mm->free_list;
    
    // Iterate through the free list to find a suitable block
    while (current) {
        // If this block is free and large enough to satisfy the request
        if (current->free && current->size >= size) {
            // Check if we should split this block
            // We split if the remaining space can fit another block header plus minimum data
            if (current->size >= size + sizeof(mem_block) + MIN_BLOCK_SIZE) {
                // Create a new block from the remaining space
                mem_block *new_block = (mem_block*)((char*)current + sizeof(mem_block) + size);
                new_block->size = current->size - sizeof(mem_block) - size;
                new_block->free = 1;
                new_block->next = current->next;
                
                // Update the current block's size and next pointer
                current->size = size;
                current->next = new_block;
            }
            
            // Mark the block as allocated
            current->free = 0;
            
            // Unlock the mutex before returning
            pthread_mutex_unlock(&mm->lock);
            
            // Return the memory area after the block header
            return (void*)((char*)current + sizeof(mem_block));
        }
        prev = current;
        current = current->next;
    }
    
    // No suitable block was found in the free list
    pthread_mutex_unlock(&mm->lock);
    return NULL;  // Return NULL to indicate allocation failure
}

// Function to free memory allocated by general_alloc
void general_free(memory_manager *mm, void *ptr) {
    // Lock the mutex to ensure thread-safe operation
    pthread_mutex_lock(&mm->lock);
    
    // Get a pointer to the block header (located before the actual data)
    mem_block *block = (mem_block*)((char*)ptr - sizeof(mem_block));
    
    // Mark the block as free
    block->free = 1;
    
    // Merge adjacent free blocks to reduce fragmentation
    // First, try to merge with the next block
    mem_block *current = mm->free_list;
    while (current) {
        // If the current block is free and points to our block
        if (current->free && current->next == block) {
            // Merge current and block
            current->size += sizeof(mem_block) + block->size;
            current->next = block->next;
            block = current;  // Update block to point to the merged block
        } 
        // If our block is free and points to the current block
        else if (block->free && block->next == current) {
            // Merge block and current
            block->size += sizeof(mem_block) + current->size;
            block->next = current->next;
        }
        current = current->next;
    }
    
    // Ensure the free list is properly sorted by address
    // If our block should be at the front of the list
    if (!mm->free_list || block < mm->free_list) {
        block->next = mm->free_list;
        mm->free_list = block;
    } else {
        // Find the correct position in the free list
        mem_block *prev = mm->free_list;
        while (prev->next && prev->next < block) {
            prev = prev->next;
        }
        // Insert block into the list
        block->next = prev->next;
        prev->next = block;
    }
    
    // Unlock the mutex before returning
    pthread_mutex_unlock(&mm->lock);
}

// Unified allocation function that chooses between SLAB and general allocation based on size
void* memory_alloc(memory_manager *mm, size_t size) {
    // For small objects (that fit in our largest size class), use SLAB allocation
    if (size <= mm->classes[MAX_CLASSES - 1].obj_size) {
        return slub(mm, size);
    }
    
    // For large objects, use general allocation
    return general_alloc(mm, size);
}

// Unified free function that determines the allocation type and calls the appropriate free function
void memory_free(memory_manager *mm, void *ptr, size_t size) {
    // If size is 0 or the object is too large for SLAB allocation, assume it's a general allocation
    if (size == 0 || size > mm->classes[MAX_CLASSES - 1].obj_size) {
        general_free(mm, ptr);
        return;
    }
    
    // Try to free as a SLAB allocation
    slab_free(mm, ptr, size);
    
    // Note: In a more robust implementation, we might check if slab_free actually freed the object
    // If not (e.g., it wasn't a SLAB allocation), we would fall back to general_free
    // For simplicity, we're not implementing that check here
}

// Function to clean up and free all resources used by the memory manager
void cleanup_memory_manager(memory_manager *mm) {
    // Check if the memory manager is NULL
    if (!mm) return;
    
    // Free all slabs for each size class
    for (int i = 0; i < MAX_CLASSES; i++) {
        // Get a pointer to the current size class
        size_class *sc = &mm->classes[i];
        
        // Iterate through all slabs in this size class
        slab *current = sc->slabs;
        while (current) {
            // Get a pointer to the next slab before freeing the current one
            slab *next = current->next;
            
            // Free the memory area containing the objects
            free(current->objects);
            
            // Free the slab metadata structure
            free(current);
            
            // Move to the next slab
            current = next;
        }
    }
    
    // Free the main memory pool
    free(mm->memory_pool);
    
    // Destroy the mutex to clean up synchronization resources
    pthread_mutex_destroy(&mm->lock);
    
    // Free the memory manager structure itself
    free(mm);
}

// Example usage of the memory manager
int main() {
    // Initialize the memory manager
    memory_manager *mm = init_memory_manager();
    if (!mm) {
        // Print an error message if initialization failed
        printf("Failed to initialize memory manager\n");
        return 1;  // Return with error code
    }
    
    // Allocate some small objects using SLAB allocation
    void *obj1 = memory_alloc(mm, 16);  // Should use SLAB allocation
    void *obj2 = memory_alloc(mm, 24);  // Should use SLAB allocation
    void *obj3 = memory_alloc(mm, 32);  // Should use SLAB allocation
    
    // Allocate a large object using general allocation
    void *large_obj = memory_alloc(mm, 4096);  // Should use general allocation
    
    // In a real application, we would use these objects here...
    
    // Free the allocated objects
    // For SLAB objects, we need to specify the original size
    memory_free(mm, obj1, 16);
    memory_free(mm, obj2, 24);
    memory_free(mm, obj3, 32);
    
    // For the large object, we specify 0 to indicate general allocation
    memory_free(mm, large_obj, 0);
    
    // Clean up the memory manager and all its resources
    cleanup_memory_manager(mm);
    
    // Return success
    return 0;
}
