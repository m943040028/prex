#include <sys/prex.h>
#include <ipc/ipc.h>
#include <ipc/proc.h>

void
register_process(void)
{
	struct msg m;
	object_t obj;
	int error;

	error = object_lookup("!proc", &obj);
	if (error)
		sys_panic("pow: no proc found");

	m.hdr.code = PS_REGISTER;
	msg_send(obj, &m, sizeof(m));
}

/*
 * Wait until specified server starts.
 */
void
wait_server(const char *name, object_t *pobj)
{
	int i, error = 0;

	/* Give chance to run other servers. */
	thread_yield();

	/*
	 * Wait for server loading. timeout is 1 sec.
	 */
	for (i = 0; i < 100; i++) {
		error = object_lookup((char *)name, pobj);
		if (error == 0)
			break;

		/* Wait 10msec */
		timer_sleep(10, 0);
		thread_yield();
	}
	if (error)
		sys_panic("pow: server not found");
}

/*
 * Run specified routine as a thread.
 */
int
run_thread(void (*entry)(void))
{
	task_t self;
	thread_t t;
	void *stack, *sp;
	int error;

	self = task_self();
	if ((error = thread_create(self, &t)) != 0)
		return error;
	if ((error = vm_allocate(self, &stack, DFLSTKSZ, 1)) != 0)
		return error;

	sp = (void *)((u_long)stack + DFLSTKSZ - sizeof(u_long) * 3);
	if ((error = thread_load(t, entry, sp)) != 0)
		return error;

	return thread_resume(t);
}
