#include <stdio.h>
#include <stdlib.h>

#include <musiccast.h>

void print_usage(char *argv[])
{
	fprintf(stderr, "usage: %s hostname\n", argv[0]);

	exit(1);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		print_usage(argv);
	}

	if (musiccast_init(argv[1]) == NULL)
	{
		fprintf(stderr, "Failed to initialize libMusicCast!\n");
	}
	return 0;
}
