/*
 * mm.c
 *
 * Name: Cam Thorpe (cqt5263), Jacky Chan (jjc)
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 * Also, read malloclab.pdf carefully and in its entirety before beginning.
 *
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*
 * If you want to enable your debugging output and heap checker code,
 * uncomment the following line. Be sure not to have debugging enabled
 * in your final submission.
 */
// #define DEBUG

#ifdef DEBUG
/* When debugging is enabled, the underlying functions get called */
#define dbg_printf(...) printf(__VA_ARGS__)
#define dbg_assert(...) assert(__VA_ARGS__)
#else
/* When debugging is disabled, no code gets generated */
#define dbg_printf(...)
#define dbg_assert(...)
#endif /* DEBUG */

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#define memset mem_memset
#define memcpy mem_memcpy
#endif /* DRIVER */

//Defining constants
#define ALIGNMENT 16
#define WSIZE 8
#define DSIZE 16
#define SEG_LIST_SIZE 14
#define CHUNKSIZE (1<<12)

// defining the different sizes in the segregated list
#define SEG_SIZE1 32
#define SEG_SIZE2 64
#define SEG_SIZE3 128
#define SEG_SIZE4 256
#define SEG_SIZE5 512
#define SEG_SIZE6 1024
#define SEG_SIZE7 2048
#define SEG_SIZE8 4096
#define SEG_SIZE9 8192
#define SEG_SIZE10 16384


typedef void *blk_ptr;

/* rounds up to the nearest multiple of ALIGNMENT */
static size_t align(size_t x)
{
    return ALIGNMENT * ((x + ALIGNMENT - 1) / ALIGNMENT);
}

static size_t max(size_t x, size_t y)
{ 
	return ((x > y) ? x : y);
}

static size_t min(size_t x, size_t y)
{ 
	return ((x < y) ? x : y);
}

// PACK the size and the allocated bit into a word
static size_t PACK(size_t size, int alloc)
{
	return ((size) | (alloc));
}

// reading the address of the block pointer
static size_t GET(blk_ptr bp)
{
	return (*(size_t *)(bp));
}
// writing the address of the block pointer
static void PUT(blk_ptr bp, size_t val)
{
	*((size_t *)(bp)) = val;
}

// setting the address of the block pointer to a pointer
static void putptr(blk_ptr bp, blk_ptr ptr)
{
	*(size_t *)(bp) = (size_t)(ptr);
}

// return the size of the block pointer
static size_t GET_SIZE(blk_ptr bp)
{
	return (size_t)(GET(bp) & ~(0xf));
}

// returning the allocated bit given a block pointer
static size_t GET_ALLOC(blk_ptr bp)
{
	return (size_t)(GET(bp) & 0x1);
}

//current header address of a block pointer
static size_t *HDRP(void *bp)
{
	return ((size_t *)(bp) - 1);
}

//current footer address of a block pointer
static size_t *FTRP(void *bp)
{
	return ((size_t *)((bp) + GET_SIZE(HDRP(bp)) - 16));
}

//get previous block address with pointer
static size_t *PREV_BLKP(blk_ptr bp)
{
	return (size_t *)((bp) - GET_SIZE((bp) - 16));
}

//get next block address with pointer
static size_t *NEXT_BLKP(blk_ptr bp)
{
	return (size_t *)((bp) + GET_SIZE(HDRP(bp)));
}

//address of previous free block
static size_t *prev_freebp(blk_ptr bp)
{
	return ((size_t *)(bp));
}

//address of next free block
static size_t *next_freebp(blk_ptr bp)
{
	return ((size_t *)((bp) + 8));
}

//address of the previous block in the segregated list 
static size_t *prev_listbp(blk_ptr bp)
{
	return (*(size_t **)(bp));
}

//address of the next block in the segregated list 
static size_t *next_listbp(blk_ptr bp)
{
	return (*(size_t **)(next_freebp(bp)));
}

//declare the segregated list array and heap pointer
blk_ptr segregated_list[SEG_LIST_SIZE];
blk_ptr heap_listp = NULL;

//Declaring prototype functions
static blk_ptr extend_heap(size_t size);
static blk_ptr coalesce(void *ptr);
static blk_ptr place(void* ptr, size_t asize);
static void delete_block_list(void *ptr);
static void add_list_block(void *ptr, size_t size);
static int seg_list_sizes(size_t asize);

int mm_check(char *function);

// extend_heap takes in an input of size_t words and uses it to extend the heap when called and create new free blocks in necessary 
static blk_ptr extend_heap(size_t words){
	//Adjust the size the align with the next word
	size_t asize = align(words);
	size_t *bp;

	//If the block pointer is equal to -1, it fails and returns NULL
	if ((size_t *)(bp = mem_sbrk(asize)) == (void *) - 1)
	{
		return NULL;
	}
	
	// Else we add the new block into the list of blocks
	add_list_block(bp,asize);
	
	// Set the block header, footer, and epilogue header
	PUT(HDRP(bp), PACK(asize, 0));
	PUT(FTRP(bp), PACK(asize, 0));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));
	
	//Coaelesce the new free block with the surrounding blocks
	return coalesce(bp);
}

// coalesce will take in a block pointer input and check to see if the adjacent blocks are free. If they are free, they will be merged
static blk_ptr coalesce(void *bp)
{
	//maintain information of previous and next block
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	
	//CASE 1: Previous and next blocks not free
	if(prev_alloc && next_alloc)
	{
		return bp;
	}
	//CASE 2: Previous block is free and next block is allocated
	else if(!prev_alloc && next_alloc)
	{
		// Get previous block info
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));

		// Merge the two blocks by placing the header pointer to the previous block and the footer pointer to the current block
		delete_block_list(bp);

		delete_block_list(PREV_BLKP(bp));

		PUT(FTRP(bp),PACK(size,0));
		PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
		bp = PREV_BLKP(bp);

	}
	//CASE 3: Previous block is allocated and the next block is free
	else if(prev_alloc && !next_alloc)
	{
		// Get next block info
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

		// Merge the two blocks by placing the header pointer to the current block and the footer pointer to the next block
		delete_block_list(bp);

		delete_block_list(NEXT_BLKP(bp));


		PUT(HDRP(bp),PACK(size, 0));
		PUT(FTRP(bp),PACK(size, 0));

	}
	//CASE 4: Next and Prev Free
	else
	{
		// Get size of blocks
		size += GET_SIZE(FTRP(NEXT_BLKP(bp))) + GET_SIZE(HDRP(PREV_BLKP(bp)));

		// Delete blocks and merge them by placing the footer pointer with the previous block and the header pointer with the next block
		delete_block_list(bp);
		delete_block_list(NEXT_BLKP(bp));
		delete_block_list(PREV_BLKP(bp));
		PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
		PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
		bp = PREV_BLKP(bp);

	}

	// add the new free block to the segregated list
	add_list_block(bp,size);

	return bp;
}

// function takes in a size and sorts it into the appropriate class for the segregated list
static int seg_list_sizes(size_t size)
{
	if (size < SEG_SIZE1)	return 0;
	else if ((size >= SEG_SIZE1) && (size < SEG_SIZE2))	return 1;
	else if ((size >= SEG_SIZE2) && (size < SEG_SIZE3))	return 2;
	else if ((size >= SEG_SIZE3) && (size < SEG_SIZE4))	return 3;
	else if ((size >= SEG_SIZE4) && (size < SEG_SIZE5))	return 4;
	else if ((size >= SEG_SIZE5) && (size < SEG_SIZE6))	return 5;
	else if ((size >= SEG_SIZE6) && (size < SEG_SIZE7))	return 6;
	else if ((size >= SEG_SIZE7) && (size < SEG_SIZE8))	return 7;
	else if ((size >= SEG_SIZE8) && (size < SEG_SIZE9))	return 8;
	else if ((size >= SEG_SIZE9) && (size < SEG_SIZE10))	return 9;
	else	return 10;
}

// function that places the requested block into a free block
// and check the free block to see if the remaining size of the free block can be used
// if not, allocate a new free block
// and add to the list.
static blk_ptr place(blk_ptr bp, size_t asize)
{
	// delete the block from the segregated list
	delete_block_list(bp);

	size_t csize = GET_SIZE(HDRP(bp));
	size_t *next_block;

	// check to see if the block needs to be split, if it does, split the block
	if ((csize - asize) >= (2*DSIZE))
	{
		PUT(HDRP(bp), PACK(asize,1));
		PUT(FTRP(bp), PACK(asize,1));

		next_block = NEXT_BLKP(bp);

		PUT(HDRP(NEXT_BLKP(bp)), PACK(csize-asize,0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(csize-asize,0));

		add_list_block(NEXT_BLKP(bp), csize-asize);
	}

	// if the remaining size is not larger than min block size, 
	// then assign a new free block to be allocated
	else
	{
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));

		if (!GET_ALLOC(HDRP(next_block)))
		{
			PUT(FTRP(next_block), GET(HDRP(next_block)));
		}
	}

	return bp;
}

// function removes the specified block from the segregated list and adjusts the pointers
static void delete_block_list(blk_ptr bp)
{
	// get block size info
	size_t size = GET_SIZE(HDRP(bp));

	// search for size class index
	int seg_index = seg_list_sizes(size);
	
	if (prev_listbp(bp) == NULL)
	{
		// update the previous free block pointer
		if (next_listbp(bp) != NULL)
		{
			putptr(prev_freebp(next_listbp(bp)), NULL);
			segregated_list[seg_index] = next_listbp(bp);
		}
		else
		{
			segregated_list[seg_index] = NULL;
		}

	}
	else
	{
		// update the pointers on the previous free block pointer and the next free block pointer
		if (next_listbp(bp) != NULL)
		{
			putptr(prev_freebp(next_listbp(bp)), prev_listbp(bp));
			putptr(next_freebp(prev_listbp(bp)), next_listbp(bp));
		}
		else
		{
			putptr(next_freebp(prev_listbp(bp)), NULL);
		}

	}

}

// function inserts a new block into the segreated list
static void add_list_block(blk_ptr bp, size_t size){
	// find the right size for the segregated list
    int seg_index = seg_list_sizes(size);

	// set the head of the segreated list
    void *curr_head_ptr = segregated_list[seg_index]; 

 // if there are no blocks in the list, update the pointers
    if (curr_head_ptr != NULL) 
    { 
		// set the current block pointer to be the head and then update the previous and next pointers
		segregated_list[seg_index] = bp;
        putptr(prev_freebp(bp), NULL);
        putptr(next_freebp(bp), curr_head_ptr);
        putptr(prev_freebp(curr_head_ptr), bp);
        
    }
	// else we just update the previous and next pointers
    else 
    { 
		// set bp to the list head and set previous and next free block to NULL
		segregated_list[seg_index] = bp;
		putptr(prev_freebp(bp), NULL);
        putptr(next_freebp(bp), NULL);  
    }
}


/*
 * Initialize: return false on error, true on success.
 */
bool mm_init(void)
{
	for(int i = 0; i < SEG_LIST_SIZE; i++)
	{
		segregated_list[i] = NULL;
	}
	
	if ((heap_listp = mem_sbrk(4 * WSIZE)) == NULL)
	{
		return false;
	}

	// Create the initial empty heap
	PUT(heap_listp, 0); /*Alignment padding*/

	PUT(heap_listp + WSIZE, PACK(2*WSIZE,1)); /*Proglogue header*/

	PUT(heap_listp + (2*WSIZE), PACK(2*WSIZE, 1)); /*Proglogue footer*/

	PUT(heap_listp + (3*WSIZE), PACK(0,1)); /*Epilogue header*/

	heap_listp+=(2*WSIZE);

	// Extend the empty heap with a free block of CHUNKSIZE bytes
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
	{
		return false;
	}

	return true;

}

/*
 * malloc: return a pointer to the payload of the allocated block
 * search the segregated list for free block and extend the heap if 
 * more blocks are required.
 */
void* malloc(size_t size)
{
	
	blk_ptr bp = NULL;
	size_t asize;
	size_t extendsize;
	int seg_index = 0;
	
	// check if size is valid
	if (size == 0)
	{
		return NULL;
	}
	
	// Adjust the block size to include overhead
	if (size <= DSIZE)
	{
		asize = 2*DSIZE;
	}
	// align it to the nearest word
	else
	{
		asize = align(size + DSIZE);
	}

	// find the right size in the segregated list
	seg_index = seg_list_sizes(asize);
	// either extend by the adjusted size or the chunksize
	extendsize = max(asize, CHUNKSIZE);

	// place the block in the right spot
	if (seg_index != SEG_LIST_SIZE)
	{
		bp = segregated_list[seg_index];
		
		if (bp != NULL)
		{
			for (int i = 0; i < 2*DSIZE; i++)
			{
				if (bp == NULL)
				{
					break;
				}
				
				else if (asize <= GET_SIZE(HDRP(bp)))
				{
					place(bp,asize);
					return bp;
				}

				bp = next_listbp(bp);
			}
		}
	}

	seg_index++;
	bp = NULL;
	
	// allocate more memory by extending the heap if not enough
	while ((seg_index < SEG_LIST_SIZE) && (bp == NULL))
	{
		bp = segregated_list[seg_index];
		seg_index++;
	}
	
	if (bp == NULL)
	{
		bp = extend_heap(extendsize);
	}

	bp = place(bp,asize);
	
	return bp;
}

/*
 * free the specified block and add it to the list of free blocks to be reallocated or allocated
 */
void free(void* ptr)
{
	size_t size;
   	
   	if (ptr == NULL) 
   		return;

	size = GET(HDRP(ptr));
	
	// unallocate the header and fooder
	PUT(HDRP(ptr),PACK(size, 0));
	PUT(FTRP(ptr),PACK(size, 0));

	// add to list of available free blocks
	add_list_block(ptr, size);
	coalesce(ptr);
}

/*
 * realloc: return a new pointer that has the size and content of the old pointer
 * malloc space for the new pointer
 */
void* realloc(void* oldptr, size_t size)
{
	// pointer of new block
	blk_ptr newptr;

	// if the oldpointer in null, just do malloc(size)
	if (oldptr == NULL)
	{
		return malloc(size);
	}
	
	// if size is zero just do a free (oldptr)
	if (size == 0)
	{
		free(oldptr);
		return NULL;
	}

	// copy the contents of current block after allocating the space for the new block and copy it
	newptr = malloc(size);
	size_t copysize = min(size, GET_SIZE(HDRP(oldptr)) - WSIZE);
	
	memmove(newptr,oldptr,copysize);
	free(oldptr);
	

	return newptr;

}

/*
 * calloc
 * This function is not tested by mdriver, and has been implemented for you.
 */
void* calloc(size_t nmemb, size_t size)
{
    void* ret;
    size *= nmemb;
    ret = malloc(size);
    if (ret) {
        memset(ret, 0, size);
    }
    return ret;
}

/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static bool in_heap(const void* p)
{
    return (p <= mem_heap_hi() && p >= mem_heap_lo());
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static bool aligned(const void* p)
{
    size_t ip = (size_t) p;
    
    return (align(ip) == ip);
}

/*
 * commented out for submission. 
 * checks both the heap and the free list
 */

bool mm_checkheap(int lineno)
{
	#ifdef DEBUG
	
	printf("Enter checkheap\n");
    
    size_t size = 0;
    heap_listp = mem_heap_lo() + (2 * WSIZE);

    while (GET_SIZE(HDRP(heap_listp)) != 0)
    {
		// check for size
        size = GET_SIZE(HDRP(heap_listp));
        if (size != GET_SIZE(FTRP(heap_listp)))
        {
            printf("Header and footer size mismatch\n");
        }

        //check for adjacent free block 
        if (!GET_ALLOC(HDRP(heap_listp)))
        {
            if (!GET_ALLOC(HDRP(NEXT_BLKP(heap_listp))))
            {
                printf("Found adjacent block\n");
            }
        }

        heap_listp = heap_listp + size;
    }

	#endif /* DEBUG */
    return true;
}
