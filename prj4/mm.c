/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your information in the following struct.
 ********************************************************/
team_t team = {
    /* Your student ID */
    "20210428",
    /* Your full name*/
    "Hyeonjung Jeong",
    /* Your email address */
    "cathy2750@sogang.ac.kr",
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define MAX(x, y) ((x) > (y)? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define NEXT_FREE(bp) (*(char **)(bp))
#define PREV_FREE(bp) (*(char **)(bp + WSIZE))


static char *heap_listp;
static char *free_listp;

static void *extend_heap(size_t words);
static void *find_fit(size_t asize);
static void *place(void *bp, size_t asize);
static void *coalesce(void *bp);
static void delete_free_list(void *bp);
static void add_free_list(size_t size, void *bp);
static char *extend_heap_2(char *bp, size_t asize, int *left);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    free_listp = NULL;
    if((heap_listp = mem_sbrk(4 * WSIZE)) == (void*)-1){
        return -1;
    }
    
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);

    if(extend_heap(1 << 6) == NULL) return -1;
    return 0;
}

static void *extend_heap(size_t words){
    //printf("\nextend heap start\n");
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    
    if((long)(bp = mem_sbrk(size)) == -1) return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    add_free_list(ALIGN(words), bp);
    //printf("\nextend heap\n");
    return coalesce(bp);
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

static char *extend_heap_2(char *bp, size_t asize, int *left){
    if(bp == NULL) {
        bp = extend_heap(MAX(asize, CHUNKSIZE));
    }
    else if(left && *left < 0){
        //printf("\nextend heap 2 start\n");
        int extendsize = MAX(CHUNKSIZE, -(*left));
        if(extend_heap(extendsize) == NULL) return NULL;
        *left += extendsize;
    }
    //printf("\nextend heap 2 end\n");
    return bp;
}

void *mm_malloc(size_t size)
{   
    //printf("\nmalloc start - %d\n", size);
    size_t asize;
    size_t extendsize;
    char *bp;

    if(size == 0) return NULL;

    if(size <= DSIZE) asize = 2 * DSIZE;
    else asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    if((bp = find_fit(asize)) != NULL){
        bp = place(bp, asize);
        //printf("\nmalloc end - fit\n");
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL) {
        //printf("\nextend failed\n");
        return NULL;
    }
    bp = place(bp, asize);
    return bp;
}

static void *find_fit(size_t asize)
{
    char *bp = free_listp;
    while (bp && (GET_SIZE(HDRP(bp)) < asize)) {
        bp = NEXT_FREE(bp);
    }
    return bp;
}

static void *place(void *bp, size_t asize){
    delete_free_list(bp);
    size_t csize = GET_SIZE(HDRP(bp));

    if((DSIZE << 1) >= (csize - asize)){
        asize = csize;
    }
    else if(asize < (ALIGNMENT << 3)){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(csize - asize, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(csize - asize, 0));
        add_free_list(csize - asize, NEXT_BLKP(bp));
        //SPLIT_BLK(bp, asize, csize - asize);
    }
    else{
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        //SET_HDR_FTR(bp, csize - asize, 0);
        add_free_list(csize - asize, bp);
        bp = NEXT_BLKP(bp);
    }

    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    //SET_HDR_FTR(bp, asize, 1);
    return bp;
}

static void add_free_list(size_t size, void *ptr){
    char *current = free_listp;
    char *previous = NULL;

    //printf("\nadd start\n");
    while (current && (GET_SIZE(HDRP(current))) < size) {
        previous = current;
        current = NEXT_FREE(current);
        //printf("\nhere3\n");
    }
    if (previous != NULL) {
        PREV_FREE(ptr) = previous;
        NEXT_FREE(previous) = ptr;
        if (current != NULL) {
            NEXT_FREE(ptr) = current;
            PREV_FREE(current) = ptr;
        } 
        else NEXT_FREE(ptr) = NULL;
    }
    else {
        free_listp = ptr;
        PREV_FREE(ptr) = NULL;
        if (current != NULL) {
            NEXT_FREE(ptr) = current;
            PREV_FREE(current) = ptr;
        } 
        else NEXT_FREE(ptr) = NULL;
    }
    //printf("\nadd end\n");
}

static void delete_free_list(void *bp){
    if(PREV_FREE(bp) != NULL){
        NEXT_FREE(PREV_FREE(bp)) = NEXT_FREE(bp);
        if(NEXT_FREE(bp)) PREV_FREE(NEXT_FREE(bp)) = PREV_FREE(bp);
        //printf("\ndelete - not first\n");
    }
    else {
        free_listp = NEXT_FREE(bp);
        if(free_listp) PREV_FREE(free_listp) = NULL;
        //printf("\ndelete - first node\n");
    }
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{   //printf("\nfree start\n");
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    add_free_list(size, bp);
    coalesce(bp);
    //printf("\nfree end\n");
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc) {
        //printf("\ncoal - case 1\n");
        return bp;
    }
    else if(prev_alloc && !next_alloc){
        //printf("\ncoal - case 2\n");
        delete_free_list(bp);
        delete_free_list(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        add_free_list(size, bp);
    }
    else if(!prev_alloc && next_alloc){
        //printf("\ncoal - case 3\n");
        delete_free_list(bp);
        delete_free_list(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        bp = PREV_BLKP(bp);
        add_free_list(size, bp);
    }
    else{
        //printf("\ncoal - case 4\n");
        delete_free_list(bp);
        delete_free_list(PREV_BLKP(bp));
        delete_free_list(NEXT_BLKP(bp));
        size += (GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        add_free_list(size, bp);
    }
    return bp;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    //printf("\nrealloc start\n");
    if(size == 0){
        mm_free(ptr);
        return 0;
    }
    else if(ptr == NULL) return mm_malloc(size);
    else{
        size_t asize;
        if(size <= DSIZE) asize = 2 * DSIZE;
        else asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
        asize += (1 << 8);

        if(GET_SIZE(HDRP(ptr)) >= asize) return ptr;

        if(!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) && ((GET_SIZE(HDRP(NEXT_BLKP(ptr))) + GET_SIZE(HDRP(ptr))) > asize)){
            int left_size = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - asize;
            ptr = extend_heap_2(ptr, asize, &left_size);
            
            if(ptr == NULL) return NULL;
            delete_free_list(NEXT_BLKP(ptr));
            PUT(HDRP(ptr), PACK(asize + left_size, 1));
            PUT(FTRP(ptr), PACK(asize + left_size, 1));
            //printf("here\n");
            return ptr;
        }

        void *new_ptr = mm_malloc(size);
        if(new_ptr == NULL) return 0;

        size_t old_size = GET_SIZE(HDRP(ptr));
        if(size < old_size) memcpy(new_ptr, ptr, size);
        else if(size >= old_size) memcpy(new_ptr, ptr, old_size);

        mm_free(ptr);

        //printf("\nrealloc end\n");
        return new_ptr;
    }
}
