// This file is part of the OpenNMS(R) Application.
// See iplike.c for license.

#ifdef UNDEF_FILE_OFFSET_BITS
# undef _FILE_OFFSET_BITS
#endif /* UNDEF_FILE_OFFSET_BITS */
#include <postgres.h>		/* PostgreSQL types */

// VARATT_SIZEP is deprecated on modern PostgreSQL
#ifndef SET_VARSIZE 
#define SET_VARSIZE(v,l) (VARATT_SIZEP(v) = (l)) 
#endif

bool iplike(text *value, text *rule);

int main(int argc, char **argv)
{
	text *	arg1;
	text *	arg2;
	char	arg1_buf[1024];
	char	arg2_buf[1024];
	
	if(argc != 3)
		return 1;
	
	
	arg1 = (text *)arg1_buf;
	arg2 = (text *)arg2_buf;
	SET_VARSIZE(arg1, strlen(argv[1])+VARHDRSZ);
	SET_VARSIZE(arg2, strlen(argv[2])+VARHDRSZ);
	strcpy(VARDATA(arg1), argv[1]);
	strcpy(VARDATA(arg2), argv[2]);
	
	return (iplike(arg1, arg2) == true ? 0 : 1);
}
