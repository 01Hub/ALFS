#include <libcomm.h>
#include <stdlib.h>

int main ()
{
	if (comm_init("127.0.0.1", getenv("PWD"))<0)
		return 1;
	
	comm_wrstr("ALFS");
	comm_wrint(7);
	comm_wrfloat(3.14);
	
	return comm_deinit();
}
