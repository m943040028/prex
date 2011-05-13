#ifndef _DBG_H_
#define _DBG_H_


/*
 * Usage:
 *
 * #define DBG
 * #define MODULE_NAME	"xxx"
 *
 * #include <sys/dbg.h>
 *
 * #ifdef DBG
 * enum your_extension {
 * 	DBG_TX = CUSTOM_TAG_START,
 *	DBG_RX,
 * };
 * static int debugflags = DBGBIT(INFO) | DBGBIT(TRACE);
 * #endif
 */

#ifdef DBG
enum {
	DBG_INFO, 
	DBG_TRACE,
	DBG_WARN,
	DBG_VERBOSE,
	CUSTOM_TAG_START,
};
#define DBGBIT(x)       (1<<DBG_##x)

/* system servers should use dprintf */
#ifdef _STANDALONE
#undef printf
#define printf dprintf
#endif

#define DPRINTF(what, fmt, ...) do { \
	if (debugflags & DBGBIT(what)) \
	printf("%s" fmt, MODULE_NAME ": ", ## __VA_ARGS__); \
} while (0)
#else
#define DPRINTF(what, fmt, ...) do {} while (0)
#endif


#define LOG_FUNCTION_NAME_ENTRY() \
	DPRINTF(TRACE, "Calling %s()\n",  __func__);
#define LOG_FUNCTION_NAME_EXIT_NORET() \
	DPRINTF(TRACE, "Leaving %s()\n",  __func__);
#define LOG_FUNCTION_NAME_EXIT(r) \
	DPRINTF(TRACE, "Leaving %s(), ret=%d\n",  __func__, r);

#endif
