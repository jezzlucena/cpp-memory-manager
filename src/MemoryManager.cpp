#include "MemoryManager.h"

// Memory Manager class
// Implemented by Jezz Lucena
// November 19, 2015
namespace MemoryManager
{
    // IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT
    //
    // This is the only static memory that you may use, no other global variables may be
    // created, if you need to save data make it fit in MM_pool
    //
    // IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT
    
    
    const int MM_POOL_SIZE = 65536;
    char MM_pool[MM_POOL_SIZE];
    
    //--
    //-- additional routines, implemented by Jezz Lucena
    //--
    
    // Returns an integer that represents the size of the memory block that starts
    // on a given index
    int getBlockSize(int index) {
        int blockSize = 0;
        
        blockSize += (MM_pool[index+1] << 8) & 0b1111111100000000;
        blockSize += (MM_pool[index+2]) & 0b11111111;
        
        return blockSize;
    }
    
    // Returns an boolean that represents if the memory block that starts
    // on a given index is free or allocated. This is done by checking
    // The last bit on the first byte of the memory block. For example,
    // if the byte is xxxxxxx1, this means the block is allocated.
    // If the byte is xxxxxxx0, this means the block is free to be used.
    int isBlockFree(int index) {
        bool isFree = (MM_pool[index] & 0b00000001) == 0b0;
        
       
        return isFree;
    }
    
    // Returns an integer that represents the index of the memory block
    // that is free for allocation, and best fits the size passed as a
    // parameter. The "best fit" block is chosen by analyzing which of
    // the available free blocks have the smallest remaining size if it
    // is possibly chosen to be allocated by an "aSize" chunk of memory.
    int bestFitIndex(int aSize) {
        int result = -1;
        int bestFitValue = MM_POOL_SIZE;
        
        int index = 0;
        while (index < MM_POOL_SIZE) {
            int blockSize = getBlockSize(index);
            bool isFree = isBlockFree(index);
            
            if (isFree) {
                int remainingSize = blockSize - (aSize+3);
                
                if (remainingSize >= 0 && remainingSize < bestFitValue) {
                    bestFitValue = remainingSize;
                    result = index;
                }
            }
            
            index += blockSize + 3;
        }
        
        return result;
    }
    
    // Returns an integer that represents the index of the memory block
    // that has its data section, starting at a given pointer's address.
    int findBlock(void* aPointer) {
        int index = 0;
        
        while (index < MM_POOL_SIZE) {
            int blockSize = getBlockSize(index);
            
            if (aPointer == &MM_pool[index+3]) {
                return index;
            }
            
            index += blockSize + 3;
        }
        
        return -1;
    }
    
    // Returns an integer that represents the index of the memory block
    // that is imediately before the block that is on the index provided
    int prevBlock(int index) {
        int lastIndex = -1;
        int currentIndex = 0;
        
        while (currentIndex != index) {
            int blockSize = getBlockSize(currentIndex);
            
            lastIndex = currentIndex;
            currentIndex += blockSize + 3;
        }
        
        return lastIndex;
    }
    
    // Returns an integer that represents the index of the memory block
    // that is imediately after the block that is on the index provided
    int nextBlock(int index) {
        int result = index;
        int blockSize = getBlockSize(index);
            
        result = index+3 + blockSize;
        
        if (result >= MM_POOL_SIZE) {
            result = -1;
        }
        
        return result;
    }
    
    // Initialize set up any data needed to manage the memory pool
    void initializeMemoryManager(void)
    {
        // Since this protocol uses 1 control byte and 2 addressing bytes,
        // we initialize the memory with 3 bytes less than the total available.
        int freeSpace = MM_POOL_SIZE - 3;
        
        // Setting the last control bit of the first byte to '0', which means "is free"
        // The rest of the first byte's bits are unused in this example
        MM_pool[0] = 0b00000000;
        
        // Saving the first 8 bits of the freeSpace variable to the first addressing byte
        // and the remaining 8 bits to the second addressing byte.
        MM_pool[1] = freeSpace >> 8;
        MM_pool[2] = freeSpace;
    }
    
    // return a pointer inside the memory pool
    // If no chunk can accommodate aSize call onOutOfMemory()
    void* allocate(int aSize)
    {
        // Define the index of the best fitting memory block for
        // an allocation of size "aSize"
        int index = bestFitIndex(aSize);
        
        // If there is any fitting block
        if (index >= 0) {
            // Get the size of the free block and calculate how many bytes will remain
            // after the allocation
            int previousSize = getBlockSize(index);
            int remainingSize = previousSize - aSize;
            int finalSize = aSize;
            
            // If there is more than 3 bytes, this means there will be enough free
            // space to home the control and addressing bytes and create an extra
            // free memory block
            if (remainingSize > 3) {
                int remainingBlockIndex = index+3 + aSize;
                
                MM_pool[remainingBlockIndex] &= 0b11111110;
                MM_pool[remainingBlockIndex+1] = (remainingSize-3) >> 8;
                MM_pool[remainingBlockIndex+2] = (remainingSize-3);
                
            // If there isn't enough space to create an extra memory block, then
            // give the remaining bytes to the original block being allocated.
            // This is done to avoid the loss of bytes with utilization.
            } else if (remainingSize > 0) {
                finalSize += remainingSize;
            }
            
            // Set the control bytes for the newly allocated memory block
            MM_pool[index] |= 0b00000001;
            MM_pool[index+1] = finalSize >> 8;
            MM_pool[index+2] = finalSize;
            
        // If there isn't any possible block that fits an allocation with an "aSize"
        // size, then call the "onOutOfMemory()" error handler.
        } else {
            onOutOfMemory();
        }
        
        // Return a pointer to the address of the data section of the newly allocated
        // memory block.
        return &MM_pool[index + 3];
    }
    
    // Free up a chunk previously allocated
    void deallocate(void* aPointer)
    {
        // Find the index of the allocated block
        int index = findBlock(aPointer);
        
        // If the block can't be found, or if the memory block is marked as "free",
        // then call the "onIllegalOperation(...)" error handler.
        if (index < 0) {
            onIllegalOperation("ILLEGAL OPERATION: Pointer on Deallocate Request can not be found.");
        } else if (isBlockFree(index)) {
            onIllegalOperation("ILLEGAL OPERATION: Pointer on Deallocate Request has been previously deallocated.");
        }
        
        // If the memory block is valid for deallocation, then find it's previous
        // and next blocks, for a possible merging.
        int prevBlockIndex = prevBlock(index);
        int nextBlockIndex = nextBlock(index);
        
        // Initiate a variable that will calculate the total size of the block after
        // deallocation.
        int totalSize = getBlockSize(index);
        
        // If a next block exists, and is not allocated, then sum its size to
        // the final block
        if (nextBlockIndex >= 0
            && isBlockFree(nextBlockIndex)) {
            totalSize += getBlockSize(nextBlockIndex) + 3;
        }
        
        // If a previous block exists, and is not allocated, then sum its size to
        // the total size and set its control and addressing bytes to reflect its
        // new size and status
        if (prevBlockIndex >= 0
            && isBlockFree(prevBlockIndex)) {
            totalSize += getBlockSize(prevBlockIndex) + 3;
            
            MM_pool[prevBlockIndex] &= 0b11111110;
            MM_pool[prevBlockIndex+1] = totalSize >> 8;
            MM_pool[prevBlockIndex+2] = totalSize;
            
        // If a previous block does not exist, or is not allocated, then set the
        // original block's control and addressing bytes to reflect its new size
        // and status
        }else{
            MM_pool[index] &= 0b11111110;
            MM_pool[index+1] = totalSize >> 8;
            MM_pool[index+2] = totalSize;
        }
    }
    
    //---
    //--- support routines
    //---
    
    // Will scan the memory pool and return the total free space remaining
    int freeRemaining(void)
    {
        // Instantiate variables for the process of iterating through the memory
        int result = 0;
        int index = 0;
        
        // While the index is still within the range of the memory array
        while (index < MM_POOL_SIZE) {
            // Calculate block's size and check if it is allocated
            int blockSize = getBlockSize(index);
            bool isFree = isBlockFree(index);
            
            // If the block is not allocated, then sum it's size to the total
            // free size
            if (isFree) {
                result += blockSize;
            }
            
            // Follow to the next block, ignoring the 3 control and addressing
            // bytes.
            index += blockSize + 3;
        }
        
        // Return the total sum of free bytes.
        return result;
    }
    
    // Will scan the memory pool and return the largest free space remaining
    int largestFree(void)
    {
        // Instantiate variables for the process of iterating through the memory
        int largestFree = -1;
        int index = 0;
        
        // While the index is still within the range of the memory array
        while (index < MM_POOL_SIZE) {
            // Calculate block's size and check if it is allocated
            int blockSize = getBlockSize(index);
            bool isFree = isBlockFree(index);
            
            // If the block is not allocated and it's size is bigger than the
            // previously registered largest free space, then save it's size
            if (isFree && blockSize > largestFree) {
                largestFree = blockSize;
            }
            
            // Follow to the next block, ignoring the 3 control and addressing
            // bytes.
            index += blockSize + 3;
        }
        
        // Return the size of the largest free memory block.
        return largestFree;
    }
    
    // will scan the memory pool and return the smallest free space remaining
    int smallestFree(void)
    {
        // Instantiate variables for the process of iterating through the memory
        int smallestFree = MM_POOL_SIZE;
        int index = 0;
        
        // While the index is still within the range of the memory array
        while (index < MM_POOL_SIZE) {
            // Calculate block's size and check if it is allocated
            int blockSize = getBlockSize(index);
            bool isFree = isBlockFree(index);
            
            // If the block is not allocated and it's size is smaller than the
            // previously registered smallest free space, then save it's size
            if (isFree && blockSize < smallestFree) {
                smallestFree = blockSize;
            }
            
            // Follow to the next block, ignoring the 3 control and addressing
            // bytes.
            index += blockSize + 3;
        }
        
        // If no free memory block was registered, reset the smallestFree variable
        if (smallestFree == MM_POOL_SIZE) {
            smallestFree = -1;
        }
        
        // Return the size of the smallest free memory block.
        return smallestFree;
    }
}