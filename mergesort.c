#include <stdio.h>
#include <stdlib.h>
#include <string.h>     /* for memcpy */
#include <pthread.h>
#include "mergesort.h"

//segment sorting verification helper function
int isSegmentSorted(int *A, int left, int right) {
    int i; // declare i outside loop
    for (i = left; i < right; i++) {
        if (A[i] > A[i + 1]) {
            return 0; // not sorted
        }
    }
    return 1; // sorted
}

void merge(int leftstart, int leftend, int rightstart, int rightend) {
    int i = leftstart;  /* i: index scanning left subarray A[leftstart..leftend] */
    int j = rightstart; /* j: index scanning right subarray A[rightstart..rightend] */
    int k = leftstart; /* k: write index into auxiliary array B, aligned with A indices */

    /* Merge into B */
    while (i <= leftend && j <= rightend) {
        if (A[i] < A[j]) { //prefer RHS on tie break ---------
            B[k++] = A[i++];
        } else {
            B[k++] = A[j++];
        }
    }
    while (i <= leftend)  B[k++] = A[i++];
    while (j <= rightend) B[k++] = A[j++];

    /* Copy merged segment back to A */
    size_t count = (size_t)(rightend - leftstart + 1);
    memcpy(&A[leftstart], &B[leftstart], count * sizeof(int));
}

/* Standard recursive (single-threaded) mergesort. */
void my_mergesort(int left, int right) {
    if (left >= right) return;
    if (isSegmentSorted(A, left, right)) {
        return;
    }
    int mid = left + (right - left) / 2;

    my_mergesort(left, mid);
    my_mergesort(mid + 1, right);
    merge(left, mid, mid + 1, right);
}

/* Thread routine: recursively spawn threads until level==cutoff, then use my_mergesort. */
void *parallel_mergesort(void *arg) {
    struct argument *args = (struct argument *)arg;
    int left  = args->left;
    int right = args->right;
    int level = args->level;

    if (level >= cutoff) {
        /* Base case for threading: switch to serial mergesort */
        my_mergesort(left, right);
        return NULL;
    }

    int mid = left + (right - left) / 2;

    //printf("Level %d: creating threads for array segment [%d..%d]\n", level, left, right); //verify segment sizes during testing

    /* Build child arguments */
    struct argument *leftArg  = buildArgs(left, mid, level + 1);
    struct argument *rightArg = buildArgs(mid + 1, right, level + 1);

    //printf("Thread %lu: Level %d, creating thread for segment [%d..%d]\n", (unsigned long)pthread_self(), level, left, right); //thread creation info

    pthread_t tleft, tright;

    /* Create two threads for the two halves */
    pthread_create(&tleft,  NULL, parallel_mergesort, (void *)leftArg);
    pthread_create(&tright, NULL, parallel_mergesort, (void *)rightArg);

    /* Join Threads*/
    pthread_join(tleft, NULL);
    pthread_join(tright, NULL);

    /* We created these child arg structs; we own the frees after join */
    free(leftArg);
    free(rightArg);

    /* Merge the two sorted halves */
    merge(left, mid, mid + 1, right);
    /*if (!isSegmentSorted(A, left, right)) {
        printf("ERROR: Segment [%d..%d] is NOT sorted!\n", left, right);
    } else {
        printf("Segment [%d..%d] verified sorted by thread %lu.\n", left, right, (unsigned long)pthread_self());
    }*/
    return NULL;
}

/* Helper to allocate a struct argument for a new (sub)task. */
struct argument *buildArgs(int left, int right, int level) {
    struct argument *p = (struct argument *)malloc(sizeof(struct argument));
    p->left  = left;
    p->right = right;
    p->level = level;
    return p;
}