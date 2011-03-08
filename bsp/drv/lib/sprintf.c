#include <sys/types.h>
#include <dki.h>

int
sprintf(char *str, char const *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = vsprintf(str, fmt, ap);
	va_end(ap);
	return (ret);
}
