#include <libcomm.h>
#include <stdio.h>

int main (int argc, char **argv)
{
	if (comm_srv_init(argc, argv))
		return 1;
	
	printf("%s\n", comm_rdstr());
	printf("%i\n", comm_rdint());
	printf("%.2f\n", comm_rdfloat());

	return comm_srv_deinit();
}
