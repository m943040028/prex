#ifndef _SERVER_H_
#define _SERVER_H_

#include <sys/prex.h>

__BEGIN_DECLS
void	wait_server(const char *name, object_t *pobj);
void	register_process(void);
int	run_thread(void (*entry)(void));

__END_DECLS

#endif
