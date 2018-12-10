#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

const unsigned int FxShift = 2;
const unsigned int FxMask = 0x03;
const unsigned int FxTag = 0x00;

const unsigned int BoolF = 0x2F;
const unsigned int BoolT = 0x6F;

const unsigned int Null = 0x3F;

const unsigned int CharShift = 8;
const unsigned int CharMask = 0xFF;
const unsigned int CharTag = 0x0F;

const unsigned int PairMask = 0x07;
const unsigned int PairTag = 0x01;

const unsigned int VectorMask = 0x07;
const unsigned int VectorTag = 0x05;

const unsigned int StringMask = 0x07;
const unsigned int StringTag = 0x06;

typedef unsigned long ptr;

char* gHeap;

static void print_char(char c) {
    if (c == ' ') {
        printf("#\\\\space");
    } else if (c == '\n') {
        printf("#\\\\newline");
    } else if (c == '\t') {
        printf("#\\\\tab");
    } else if (c == '\r') {
        printf("#\\\\return");
    } else if (c == '"') {
        printf("#\\\\\\\"");
    } else if (c == '\\') {
        printf("#\\\\\\\\");
    } else {
        printf("#\\\\%c", c);
    }
}

static void print_char_within_str(char c) {
    if (c == '"') {
        printf("\\\\\\\"");
    } else if (c == '\\') {
        printf("\\\\\\\\");
    } else {
        printf("%c", c);
    }
}

static void print_ptr(ptr x, int parentIsPair) {
    if ((x & FxMask) == FxTag) {
        // TODO Examine why casting to long causes fx- to fail on boundary
        // cases.
        printf("%d", ((int)x) >> FxShift);
    } else if (x == BoolF) {
        printf("#f");
    } else if (x == BoolT) {
        printf("#t");
    } else if (x == Null) {
        printf("()");
    } else if ((x & CharMask) == CharTag) {
        print_char((char)(((int)x) >> CharShift));
    } else if ((x & PairMask) == PairTag) {
        ptr car = ((ptr*)(x - PairTag))[0];
        ptr cdr = ((ptr*)(x - PairTag))[1];

        // If x is a nested pair/list, don't print the (.
        if (!parentIsPair) {
            printf("(");
        }

        print_ptr(car, 0);

        if (cdr != Null) {
            // If cdr is a nested pair/list, don't print the dot.
            if ((cdr & PairMask) != PairTag) {
                printf(" . ");
            } else {
                printf(" ");
            }

            print_ptr(cdr, 1);
        }

        // If x is a nested pair/list, don't print the ).
        if (!parentIsPair) {
            printf(")");
        }
    } else if ((x & VectorMask) == VectorTag) {
        ptr length = ((ptr*)(x - VectorTag))[0] >> FxShift;
        printf("#(");

        for (int i = 0; i < length; ++i) {
            print_ptr(((ptr*)(x - VectorTag))[i + 1], 0);

            if (i < length - 1) {
                printf(" ");
            }
        }

        printf(")");
    } else if ((x & StringMask) == StringTag) {
        ptr length = ((ptr*)(x - StringTag))[0] >> FxShift;
        printf("\\\"");

        for (int i = 0; i < length; ++i) {
            assert((((ptr*)(x - StringTag))[i + 1] & CharMask) == CharTag);
            char c = (char)(((ptr*)(x - StringTag))[i + 1] >> CharShift);
            print_char_within_str(c);
        }

        printf("\\\"");
    } else {
        printf("#<unknown 0x%08lx>", x);
    }
}

static char* allocate_protected_space(int size) {
    int page = getpagesize();
    int status;
    int aligned_size = ((size + page - 1) / page) * page;
    char* p = mmap(0, aligned_size + 2 * page, PROT_READ | PROT_WRITE,
                   MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);

    if (p == MAP_FAILED) {
        exit(1);
    }

    status = mprotect(p, page, PROT_NONE);

    if (status != 0) {
        exit(1);
    }

    status = mprotect(p + page + aligned_size, page, PROT_NONE);

    if (status != 0) {
        exit(1);
    }

    return (p + page);
}

static void deallocate_protected_space(char* p, int size) {
    int page = getpagesize();
    int status;
    int aligned_size = ((size + page - 1) / page) * page;
    status = munmap(p - page, aligned_size + 2 * page);

    if (status != 0) {
        exit(1);
    }
}

typedef struct {
    void* rax;
    void* rbx;
    void* rcx;
    void* rdx;
    void* rsi;
    void* rdi;
    void* rbp;
    void* rsp;
} context;

long scheme_entry(context*, char*, char*);

int main(int argc, char** argv) {
    int stack_size = (16 * 4096);
    int heap_size = (16 * 4096);
    char* stack_top = allocate_protected_space(stack_size);
    char* stack_base = stack_top + stack_size;
    gHeap = allocate_protected_space(heap_size);
    context ctxt;
    print_ptr(scheme_entry(&ctxt, stack_base, gHeap), 0);
    printf("\n");
    deallocate_protected_space(stack_top, stack_size);

    return 0;
}
