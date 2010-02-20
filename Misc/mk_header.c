/* gcc -Wall mk_header.c -o mk_header */

/*

dump the bytes of given input to a C header.

use the header in the relevant source like:

const char array[] =
{
# include "conback.h"
};

*/

#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
	FILE *f;
	struct stat s;
	unsigned char *buf, *ptr;
	const char *output;
	int i, j;

	if (argc != 2 && argc != 3)
	{
		printf ("Usage: mk_header  <input>  [output]\n"
			"       output defaults to conback.h\n");
		return 1;
	}

	if (stat(argv[1], &s) == -1 ||
		! S_ISREG(s.st_mode) )
	{
		printf ("Couldn't stat %s\n", argv[1]);
		return 1;
	}

	if (s.st_size == 0)
	{
		printf ("%s is an empty file\n", argv[1]);
		return 1;
	}

	buf = (unsigned char *) malloc (s.st_size);
	if (!buf)
	{
		printf ("Couldn't malloc %ld bytes\n",
					(long)s.st_size);
		return 1;
	}

	f = fopen (argv[1], "rb");
	if (!f)
	{
		printf ("Couldn't open %s\n", argv[1]);
		return 1;
	}

	if (fread (buf, 1, s.st_size, f) != s.st_size)
	{
		fclose (f);
		free (buf);
		printf ("Error reading %s\n", argv[1]);
		return 1;
	}
	fclose (f);

	output = (argc == 3) ? argv[2] : "conback.h";
	f = fopen (output, "wb");
	if (!f)
	{
		free (buf);
		printf ("Couldn't open %s\n", output);
		return 1;
	}

	for (i = 0, j = 0, ptr = buf; i < s.st_size; ++i)
	{
		fprintf (f, "0x%02x", *ptr++);
		if (i == s.st_size - 1)
			break;
		fprintf (f, ",");
		if (++j < 16)
			fprintf (f, " ");
		else
		{
			j = 0;
			fprintf (f, "\n");
		}
	}
	fprintf (f, "\n");

	fclose (f);
	free (buf);

	return 0;
}

