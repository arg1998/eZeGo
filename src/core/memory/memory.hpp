#include "core/definitions.hpp"



/// @brief allocates memeory using the underlying platform allocator
/// @param size number of bytes to allocate
/// @return a block of freshly allocated memory
EZ_NO_DISCARD void* ezAllocate(u64 size);

/// @brief allocates aligned memeory using the underlying platform allocator
/// @param size number of bytes to allocate
/// @param alignment alignment value, typically is a power of 2 and greated than 1
/// @return a block of freshly allocated aligned memory
EZ_NO_DISCARD void* ezAllocateAligned(u64 size, u16 alignment);


/// @brief free the memory using the platform deallocator
/// @param buffer the memeory region that is being freed
void ezFree(void* buffer);


/// @brief sets and overwrites a memeory region with a value
/// @param buffer the memory region that is being overwritten
/// @param size size of the memory region
/// @param value value that is being set for the provided memory region
void ezSetMemory(void* buffer, u64 size, s32 value);

/// @brief sets and overwrites a memeory region with 0 (zero)
/// @param buffer the memory region that is being overwritten
/// @param size size of the memory region
void ezZeroMemory(void* buffer, u64 size);


/// @brief copies values in the memeory from origin to desitnation
/// @param orig the origin memeory region
/// @param dest the destination memory region
/// @param size number of bytes that is being copied from `orig` to `dest`
void ezCopyMemmory(void *orig, void* dest, u64 size);