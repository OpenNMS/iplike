// This file is part of the OpenNMS(R) Application.
// See iplike.c for license.

#ifdef UNDEF_FILE_OFFSET_BITS
# undef _FILE_OFFSET_BITS
#endif /* UNDEF_FILE_OFFSET_BITS */
#include <postgres.h>		/* PostgreSQL types */

bool _iplike(const char *value, const char *rule);

int main(int argc, char **argv)
{
	if(argc != 3)
		return 1;

	return (_iplike(argv[1], argv[2]) == true ? 0 : 1);
}
