/*
 * This code is distributed under the terms of the GNU General Public License.
 * For more information, please refer to the LICENSE file in the root directory.
 * -------------------------------------------------
 * Copyright (C) 2025 Rodrigo R.
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
*/

#ifndef FLUENT_LIBC_ARENA_LIBRARY_H
#define FLUENT_LIBC_ARENA_LIBRARY_H

// ============= FLUENT LIB C =============
// Arena Memory Allocator
// ----------------------------------------
// Fast, linear memory allocator using arena-style chunked allocation.
// Avoids repeated `malloc` calls for small, fixed-size allocations.
//
// Ideal for:
// - High-performance scenarios with lots of short-lived allocations
// - Game engines, scripting systems, AST construction, etc.
// - Avoiding fragmentation & malloc overhead
//
// Types Provided:
// ----------------------------------------
// - `arena_t`
//   Represents a raw memory chunk tracked by the allocator.
//
// - `arena_allocator_t`
//   Manages multiple `arena_t` chunks via a vector backend.
//
// Functions:
// ----------------------------------------
// arena_allocator_t *arena_new(size_t chunk_els, size_t el_size);
//   - Initializes an arena allocator with element/chunk size.
//
// void *arena_malloc(arena_allocator_t *arena);
//   - Allocates memory from the arena. Fast, non-zeroed.
//
// void destroy_arena(arena_allocator_t *arena);
//   - Frees all memory associated with the arena.
//
// Example Usage:
// ----------------------------------------
//     arena_allocator_t *arena = arena_new(100, sizeof(MyStruct));
//     MyStruct *s = (MyStruct *)arena_malloc(arena);
//     ...
//     destroy_arena(arena); // clean up when done
//
// Notes:
// ----------------------------------------
// - Memory from `arena_malloc` is *not* individually freeable
// - Call `destroy_arena` to free all chunks at once
// - Internally uses `vector_t` from fluent_libc for chunk tracking
//
// Dependencies:
// ----------------------------------------
// - fluent/types/types.h
// - fluent/vector/vector.h
//
// ----------------------------------------
// Initial revision: 2025-05-26
// ----------------------------------------

// ============= FLUENT LIB C++ =============
#if defined(__cplusplus)
extern "C"
{
#endif

#ifndef FLUENT_LIBC_RELEASE
#   include <types.h> // fluent_libc
#   include <vector.h> // fluent_libc
#else
#   include <fluent/types/types.h> // fluent_libc
#   include <fluent/vector/vector.h> // fluent_libc
#endif
#include <stdbool.h>
#include <stdlib.h>

/**
 * \brief Represents a memory arena for efficient memory allocation.
 *
 * The arena manages a contiguous block of memory, tracking the total size and
 * the amount of memory used. Allocations are performed linearly from the arena.
 */
typedef struct
{
    void *memory;      /**< Pointer to the allocated memory block */
    size_t size;       /**< Size of the memory block */
    size_t used;       /**< Amount of memory currently used */
} arena_t;

// ==== VECTOR DEFINITION ===
#ifndef FLUENT_LIBC_ARENA_VEC_DEFINED
    DEFINE_VECTOR(arena_t *, arena); // Define a vector type for arena_t
#   define FLUENT_LIBC_ARENA_VEC_DEFINED 1
#endif

/**
 * \brief Arena allocator managing a linked list of arena chunks.
 *
 * The allocator maintains the head of the chunk list and the size of each chunk,
 * enabling efficient allocation and expansion.
 */
typedef struct
{
    vector_arena_t *chunks;    /**< Vector of arena chunks */
    size_t el_size;            /**< Size of each element in the arena */
    size_t chunk_els;          /**< Number of elements in each chunk */
} arena_allocator_t;

/**
 * \brief Creates a new arena allocator.
 *
 * Allocates and initializes an arena allocator structure, which manages
 * a vector of memory chunks for efficient allocation of fixed-size elements.
 *
 * \param chunk_els The number of elements per chunk.
 * \param el_size The size of each element in bytes.
 * \return Pointer to the initialized arena_allocator_t, or NULL on failure.
 */
static inline arena_allocator_t *arena_new(const size_t chunk_els, const size_t el_size)
{
    // Allocate memory for the arena allocator
    arena_allocator_t *allocator = (arena_allocator_t *)malloc(sizeof(arena_allocator_t));

    // Handle allocation failure
    if (!allocator)
    {
        return NULL; // Return NULL if memory allocation fails
    }

    // Initialize the arena allocator
    vector_arena_t *v = (vector_arena_t *)malloc(sizeof(vector_arena_t));
    if (!v)
    {
        free(allocator); // Free the allocator if vector allocation fails
        return NULL; // Return NULL
    }

    // Initialize the vector of chunks
    vec_arena_init(v, 30, 1.5);

    allocator->chunks = v; // Initialize the vector of chunks
    allocator->el_size = el_size; // Set the size of each element in the arena
    allocator->chunk_els = chunk_els; // Set the number of elements in each chunk

    return allocator; // Return the initialized arena allocator
}

/**
 * \brief Allocates memory for a single element from the arena allocator.
 *
 * This function returns a pointer to a memory block of size `el_size` managed by the arena allocator.
 * If the current chunk does not have enough space, a new chunk is allocated and added to the arena.
 *
 * \param arena Pointer to the arena allocator (`arena_allocator_t`).
 * \return Pointer to the allocated memory block, or NULL if allocation fails or the arena is NULL.
 *
 * The returned memory is not zero-initialized. The arena allocator manages the memory and
 * individual allocations cannot be freed; instead, the entire arena should be destroyed when done.
 */
static inline void *arena_malloc(arena_allocator_t *arena)
{
    // Skip if the arena is NULL
    if (!arena)
    {
        return NULL; // Return NULL if the arena is not initialized
    }

    // Determine if there is enough memory in the current chunk
    bool has_space = false;
    if (arena->chunks->length > 0)
    {
        // Get the last chunk in the vector
        const arena_t *last_chunk = vec_arena_get(arena->chunks, arena->chunks->length - 1);

        // Check if there is enough space in the last chunk
        has_space = last_chunk->used + arena->el_size <= last_chunk->size;
    }

    // Check if there are any chunks in the arena
    if (
        arena->chunks->length == 0
        || !has_space
    )
    {
        // Allocate a new chunk of memory
        void *chunk = malloc(arena->el_size * arena->chunk_els);
        if (!chunk)
        {
            return NULL; // Return NULL if memory allocation fails
        }

        // Allocate a new arena_t structure for the chunk
        arena_t *new_chunk = (arena_t *)malloc(sizeof(arena_t));
        if (!new_chunk)
        {
            free(chunk); // Free the chunk memory if arena_t allocation fails
            return NULL; // Return NULL if memory allocation fails
        }

        // Initialize the new arena
        new_chunk->memory = chunk; // Set the memory pointer to the allocated chunk
        new_chunk->size = arena->el_size * arena->chunk_els; // Set the size of the chunk
        new_chunk->used = 0; // Initialize the used memory to 0

        // Add the new chunk to the vector of chunks
        vec_arena_push(arena->chunks, new_chunk);
    }

    // Get the last chunk in the vector
    arena_t *last_chunk = vec_arena_get(arena->chunks, arena->chunks->length - 1);

    // Return a pointer to the next available memory in the last chunk
    void *ptr = (char *)last_chunk->memory + last_chunk->used;
    last_chunk->used += arena->el_size; // Update the used memory in the last chunk

    // Return the pointer to the allocated memory
    return ptr;
}

/**
 * \brief Destroys an arena allocator and frees all associated memory.
 *
 * This function releases all memory chunks managed by the arena allocator,
 * destroys the vector of chunks, and finally frees the arena allocator itself.
 * After calling this function, the arena pointer becomes invalid.
 *
 * \param arena Pointer to the arena allocator (`arena_allocator_t`) to destroy.
 */
static inline void destroy_arena(arena_allocator_t *arena)
{
    // Check if the arena is NULL
    if (!arena)
    {
        return; // Do nothing if the arena is not initialized
    }

    // Free each chunk in the vector
    for (size_t i = 0; i < arena->chunks->length; i++)
    {
        arena_t *chunk = vec_arena_get(arena->chunks, i);
        free(chunk->memory); // Free the memory of the chunk
        free(chunk); // Free the arena_t structure
    }

    // Free the vector of chunks
    vec_arena_destroy(arena->chunks, NULL);

    // Free the arena allocator itself
    free(arena);
}

// ============= FLUENT LIB C++ =============
#if defined(__cplusplus)
}
#endif

#endif //FLUENT_LIBC_ARENA_LIBRARY_H