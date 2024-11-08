#include "../include/usage.h"

_Noreturn void usage_s(const char *prog_name, int exit_code, const char *message)
{
	if(message)
	{
		fprintf(stderr, "%s\n", message);
	}



	// edit this later once the usage is defined
	fprintf(stderr, "usage: %s [-h] <address> <port>\n", program_name);
	fputs("	-h  display this help message\n", stderr);
	exit(exit_code);
}

_Noreturn void usage_c(const char *prog_name, int exit_code, const char *message)
{

	if(message)
	{
		fprintf(stderr, "%s\n", message);
	}



	// edit this later once the usage is defined
	fprintf(stderr, "usage: %s [-h] <address> <port>\n", program_name);
	fputs("	-h  display this help message\n", stderr);
	exit(exit_code);
}
