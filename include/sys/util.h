#ifndef _SYS_UTIL_H_
#define _SYS_UTIL_H_


#define container_of(p, type, member) \
    ((type *)((char *)(p) - (unsigned long)(&((type *)0)->member)))

#endif /* _SYS_UTIL_H_ */
