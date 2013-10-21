#ifndef _OSFMK_ARM_SETJMP_H_
#define _OSFMK_ARM_SETJMP_H_

typedef struct jmp_buf {
    int jmp_buf[64];
} jmp_buf_t;

#endif
