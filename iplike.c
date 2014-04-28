/*
// This file is part of the OpenNMS(R) Application.
//
// OpenNMS(R) is Copyright (C) 2002-2003 The OpenNMS Group, Inc.  All rights reserved.
// OpenNMS(R) is a derivative work, containing both original code, included code and modified
// code that was published under the GNU General Public License. Copyrights for modified 
// and included code are below.
//
// OpenNMS(R) is a registered trademark of The OpenNMS Group, Inc.
//
// Copyright (C) 1999-2001 Oculan Corp.  All rights reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// For more information contact:
//      OpenNMS Licensing       <license@opennms.org>
//      http://www.opennms.org/
//      http://www.opennms.com/
//
// Tab Size = 8
//
// iplike.c,v 1.1.1.1 2001/11/11 17:40:07 ben Exp
//
*/

#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <ctype.h>		/* used for isdigit() & isspace() */
#include <pwd.h>		/* so we don't get errors from pgsql's port.h */
#include <netdb.h>		/* so we don't get errors from pgsql's port.h */

#ifdef HAVE_WINDEF_H
#include <winsock2.h>
#include <windef.h>
#endif

/*
 * These PACKAGE_* defines might be defined both by PostgreSQL and us, so undef 
 * ours before we include postgres.h (sigh).  Maybe we could do the opposite
 * and move these if/undef/endifs after the include of postgres.h and then
 * include our config.h?  It doesn't seem to matter now because we don't use
 * any of the PACKAGE_* defines.
 */
#ifdef PACKAGE_BUGREPORT
# undef PACKAGE_BUGREPORT
#endif

#ifdef PACKAGE_NAME
# undef PACKAGE_NAME
#endif

#ifdef PACKAGE_STRING
# undef PACKAGE_STRING
#endif

#ifdef PACKAGE_TARNAME
# undef PACKAGE_TARNAME
#endif

#ifdef PACKAGE_VERSION
# undef PACKAGE_VERSION
#endif

#ifdef UNDEF_FILE_OFFSET_BITS
# undef _FILE_OFFSET_BITS
#endif /* UNDEF_FILE_OFFSET_BITS */
#include <postgres.h>		/* PostgreSQL types */

#ifdef HAVE_FMGR_H
# include <fmgr.h>
# ifdef PG_MODULE_MAGIC
#  ifndef DLLIMPORT
#   define DLLIMPORT
#  endif
   PG_MODULE_MAGIC;
# endif
#endif

/**
 * The OctetRange element is used to hold a single item from
 * an iplike match string. An octet may be either specific,
 * all, or an inclusive range or a list of any of these filters
 * separated by commas.
 */
struct OctetRange
{
#define	RANGE_TYPE_SPECIFIC  0
#define RANGE_TYPE_ALL	     1
#define RANGE_TYPE_INCLUSIVE 3
	int	type;
	
	union
	{
		int		specific;
		int		endpoints[2];
	} un;
};
typedef struct OctetRange	OctetRange_t;

struct OctetRangeArray
{
    int count;
    OctetRange_t *ranges;
};
typedef struct OctetRangeArray   OctetRangeArray_t;

static const int trim(const char * *const p, int *const len);
static const int getIPv4RangeInfo(const char *p, int len, OctetRangeArray_t *const dest);
static const int getIPv6RangeInfo(const char *p, int len, OctetRangeArray_t *const dest);

void getOctetRangeArrayString(const OctetRangeArray_t array, char *string) {
	int i;
	
	strcpy(string, "");
	for (i = 0; i < array.count; i++) {
		char temp[255] = "";
		switch(array.ranges[i].type) {
			case RANGE_TYPE_SPECIFIC:
				sprintf(temp, "[%d]", array.ranges[i].un.specific);
				break;
			case RANGE_TYPE_ALL:
				sprintf(temp, "[*]");
				break;
			case RANGE_TYPE_INCLUSIVE:
				sprintf(temp, "[%d-%d]", array.ranges[i].un.endpoints[0], array.ranges[i].un.endpoints[1]);
				break;
		}
		strcat(string, temp);
	}
}

/**
 * Converts a dotted decimal IP Address to its
 * component octets. The component octets are
 * stored in the destination address which
 * must be four in length. No bounds checking
 * is performed on the destination buffer!
 *
 * p		- Pointer to the dotted decimal address buffer
 * len		- The length of the buffer.
 * dest		- The destination buffer 
 *
 * Returns:
 *	Returns zero on success, non-zero on error!
 *
 */
static
const int convertIPv4(const char *p, int len, int *const dest) {
	int	octet = 0;	/* used for converting the text to binary data */
	int	ndx   = 0;	/* index into the dest buffer */
	int	hadDigit = 0;	/* set to one when a digit is encountered */
	
	/*
	 * decode the octets
	 */
	ndx = 0;
	while(len > 0 && ndx < 4)
	{
		if(isdigit((int)*p))
		{	
			/*
			 * convert the digit and multiply the 
			 * current value by 10. Remember it's 
			 * dotted decimal (that's base 10!)
			 */
			octet = (octet * 10) + (*p - '0');
			hadDigit = 1;
		}
		else if(*p == '.')
		{
			/**
			 * Store off the octet, but perform
			 * some basic checks
			 */
			if(octet > 255 || hadDigit == 0)
				return -1;
			
			hadDigit     = 0;
			dest[ndx++]  = octet;
			octet        = 0;
		}
		else
		{
			/**
			 * invalid character
			 */
			return -1;
		}
		
		/*
		 * next please
		 */
		++p;
		--len;
	}
	
	/*
	 * handle the case where we ran out of buffer
	 */
	if(ndx == 3 && hadDigit != 0)
	{
		/**
		 * Again just perform some basic checks
		 */
		if(octet > 255)
			return -1;
		
		dest[ndx++] = octet;
	}
	
	/**
	 * if we got four octets then it was
	 * successful, else it was not and an
	 * error condition needs to be returned.
	 */
	return (ndx == 4 ? 0 : -1);
}

static
const int convertIPv6(const char *p, int len, int *const dest) {
	int	octet = 0;	/* used for converting the text to binary data */
	int	ndx   = 0;	/* index into the dest buffer */
	int	hadDigit = 0;	/* set to one when a digit is encountered */
	bool hexBase = true;

	/*
	 * decode the octets
	 */
	ndx = 0;
	while(len > 0 && ndx < 9)
	{
		if(isdigit((int)*p))
		{
			if (hexBase) {
				// Convert the digit and multiply the current 
				// value by 16. IPv6 is colon-separated
				// hexadecimal.
				octet = (octet * 16) + (*p - '0');
				hadDigit = 1;
			} else {
				// If we're inside the scope identifier,
				// then the digits are decimal integers.
				octet = (octet * 10) + (*p - '0');
				hadDigit = 1;
			}
		}
		else if(*p >= 'A' && *p <= 'F')
		{
			/*
			 * convert the digit and multiply the 
			 * current value by 16. IPv6 is 
			 * colon-separated hexadecimal.
			 */
			octet = (octet * 16) + (*p - 'A' + 10);
			hadDigit = 1;
		}
		else if(*p >= 'a' && *p <= 'f')
		{	
			/*
			 * convert the digit and multiply the 
			 * current value by 16. IPv6 is 
			 * colon-separated hexadecimal.
			 */
			octet = (octet * 16) + (*p - 'a' + 10);
			hadDigit = 1;
		}
		else if(*p == ':' || *p == '%')
		{
			/**
			 * Store off the octet, but perform
			 * some basic checks
			 */
			if(octet > 65535 || hadDigit == 0)
				return -1;
			
			hadDigit     = 0;
			dest[ndx++]  = octet;
			octet        = 0;
			
			// If we've hit the scope identifier, then switch to decimal conversion mode
			if (*p == '%') { hexBase = false; }
		}
		else
		{
			/**
			 * invalid character
			 */
			return -1;
		}
		
		/*
		 * next please
		 */
		++p;
		--len;
	}
	
	/*
	 * handle the case where we ran out of buffer
	 */
	if(ndx < 9 && hadDigit != 0)
	{
		/**
		 * Again just perform some basic checks
		 */
		if(octet > 65535)
			return -1;
		
		dest[ndx++] = octet;
	}
	
	/**
	 * if we got four octets then it was
	 * successful, else it was not and an
	 * error condition needs to be returned.
	 */
	return ((ndx == 8 || ndx == 9) ? 0 : -1);
}

/**
 * Converts an IPv4 or IPv6 Address to its
 * component octets. The component octets are
 * stored in the destination address which
 * must be less than 9 integers in length. 
 * No bounds checking
 * is performed on the destination buffer!
 *
 * p		- Pointer to the address buffer
 * len		- The length of the buffer
 * dest		- The destination buffer 
 *
 * Returns:
 *	Returns zero on success, non-zero on error!
 *
 */
static 
const int convertIP(const char *p, int len, int *dest)
{
	int i;
	
	/*
	 * check to make sure that the data is value
	 */
	if(dest == NULL || p == NULL || len <= 0)
		return -1;
	
	// Pass by reference so that the current values are updated
	if (trim(&p, &len) < 0)
		return -1;

	// Scan for the first character that can determine whether this
	// is an IPv4 address or IPv6 address.
	for(i = 0; i < len; i++)
	{
		const char curr = *(p+i);
		if (curr == '.') {
			return convertIPv4(p, len, dest);
		} else if (curr >= 'a' && curr <= 'f') {
			return convertIPv6(p, len, dest);
		} else if (curr >= 'A' && curr <= 'F') {
			return convertIPv6(p, len, dest);
		} else if (curr == ':') {
			return convertIPv6(p, len, dest);
		} else if (curr == '%') {
			return convertIPv6(p, len, dest);
		}
	}

	return -1;
}

static
const int trim(const char * *const p, int *const len) {
	// perform some basic checks
	if(*p == NULL || *len <= 0)
		return -1;
		
	// increment past the space chars
	while(isspace((int)**p) && *len > 0) {
		++*p, --*len;
	}
	
	// shift space off the end
	while(*len > 0 && (isspace((int)(*(*p + *len - 1))) || (int)(*(*p + *len - 1)) == 0)) {
		--*len;
	}
	
	// Another basic check
	if(*len <= 0)
		return -1;
	
	return 0;
}

enum {
	DEFAULT = 0,
	ALL = 1,
	RANGE = 2
};

enum {
	OUT = 0,
	IN = 1
};

/**
 * Converts a dotted decimal IP Like Address to its
 * component octets. The component octets are
 * stored in the destination address which
 * must be four in length. No bounds checking
 * is performed on the destination buffer!
 *
 * p		- Pointer to the dotted decimal address buffer
 * len		- The length of the buffer.
 * dest		- The destination buffer 
 *
 * Returns:
 *	Returns zero on success, non-zero on error!
 *
 */
static
const int getRangeInfo(const char *p, int len, OctetRangeArray_t *const dest)
{
	int i;
	
	/**
	 * perform some basic checks
	 */
	if(p == NULL || len <= 0 || dest == NULL)
		return -1;
	
	// Pass by reference so that the current values are updated
	if (trim(&p, &len) < 0)
		return -1;
	
	// Scan for the first character that can determine whether this
	// is an IPv4 address or IPv6 address.
	for(i = 0; i < len; i++)
	{
		const char curr = *(p+i);
		if (curr == '.') {
			return getIPv4RangeInfo(p, len, dest);
		} else if (curr >= 'a' && curr <= 'f') {
			return getIPv6RangeInfo(p, len, dest);
		} else if (curr >= 'A' && curr <= 'F') {
			return getIPv6RangeInfo(p, len, dest);
		} else if (curr == ':') {
			return getIPv6RangeInfo(p, len, dest);
		} else if (curr == '%') {
			return getIPv6RangeInfo(p, len, dest);
		}
	}
	
	return -1;
}

static
const int getIPv4RangeInfo(const char *p, int len, OctetRangeArray_t *const dest)
{
	int ndx        = 0; /* index into the dest parameter */
	int hadDigit   = 0; /* set true when a digit is encountered */
	int octet      = 0; /* the octet value, if any */
	int rangeBegin = 0; /* the octet value, if any */
	
	int i;
	
	int state = DEFAULT;
	int listState = OUT;
	
	/*
	 * count the number of dots, must equal 3
	 */
	for(i = 0; i < len && ndx < 3; i++)
	{
		if(*(p+i) == '.')
			++ndx;
	}
	if(ndx != 3)
		return -1;
	
	/*
	 * it's ok so decode it
	 */
	ndx   = 0;
	// octet = 0;
	while(len > 0 && ndx < 4)
	{
		switch(*p) {
		case '*':
			/**
			 * sanity check
			 */
			// Must not follow a digit, must not occur in a range, and the next character must be
			// either '.' or ','
			if(hadDigit || octet != 0 || state == RANGE || rangeBegin != 0 || (len > 1 &&  (*(p+1) != '.' && *(p+1) != ',')))
				return -1;
			
			state = ALL;
			break;

		case ',':
			/**
			 * sanity check 
			 */
			if(octet > 255)
				return -1;
			
			listState = IN;
			switch(state) {
				case RANGE:
					dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_INCLUSIVE;
					// Set the beginning of the range
					dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = rangeBegin;
					// Set the end of the range
					dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = octet;
					// If the beginning is greater than the end...
					if(dest[ndx].ranges[dest[ndx].count].un.endpoints[0] > dest[ndx].ranges[dest[ndx].count].un.endpoints[1])
					{
						// ... then swap the digits
						int swap = dest[ndx].ranges[dest[ndx].count].un.endpoints[0];
						dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = dest[ndx].ranges[dest[ndx].count].un.endpoints[1];
						dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = swap;
					}
					dest[ndx].count++;
					rangeBegin = 0;
					break;
				case ALL:
					dest[ndx].ranges[dest[ndx].count++].type = RANGE_TYPE_ALL;
					break;
				case DEFAULT:
					dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_SPECIFIC;
					dest[ndx].ranges[dest[ndx].count++].un.specific = octet;
					break;
			}
			
			state = DEFAULT;
			hadDigit = 0;
			octet    = 0;
			break;

		case '-':
			/**
			 * sanity check
			 */
			if(!hadDigit || state == ALL || rangeBegin != 0 || octet > 255)
				return -1;
			
			if(state != RANGE)
			{
				state = RANGE;
				// Set the beginning of the range
				rangeBegin = octet;
			}
			else
			{
				// You cannot have two dashes within a range
				return -1;
			}
			
			hadDigit = 0;
			octet    = 0;
			break;

		case '.':
			/**
			 * basic sanity check
			 */
			if(octet > 255 || (hadDigit == 0 && state != ALL))
				return -1;
			
			/**
			 * what type of value is it?
			 */
			if(listState == IN)
			{
				switch(state) {
					case RANGE:
						dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_INCLUSIVE;
						// Set the beginning of the range
						dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = rangeBegin;
						// Set the end of the range
						dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = octet;
						// If the beginning is greater than the end...
						if(dest[ndx].ranges[dest[ndx].count].un.endpoints[0] > dest[ndx].ranges[dest[ndx].count].un.endpoints[1])
						{
							// ... then swap the digits
							int swap = dest[ndx].ranges[dest[ndx].count].un.endpoints[0];
							dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = dest[ndx].ranges[dest[ndx].count].un.endpoints[1];
							dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = swap;
						}
						dest[ndx].count++;
						rangeBegin = 0;
						break;
					case ALL:
						dest[ndx].ranges[dest[ndx].count++].type = RANGE_TYPE_ALL;
						break;
					case DEFAULT:
						dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_SPECIFIC;
						dest[ndx].ranges[dest[ndx].count++].un.specific = octet;
						break;
				}
				listState = OUT;
			}
			else if(state == RANGE)
			{
				dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_INCLUSIVE;
				// Set the beginning of the range
				dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = rangeBegin;
				// Set the end of the range
				dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = octet;
				// If the beginning is greater than the end...
				if(dest[ndx].ranges[dest[ndx].count].un.endpoints[0] > dest[ndx].ranges[dest[ndx].count].un.endpoints[1])
				{
					// ... then swap the digits
					int swap = dest[ndx].ranges[dest[ndx].count].un.endpoints[0];
					dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = dest[ndx].ranges[dest[ndx].count].un.endpoints[1];
					dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = swap;
				}
				dest[ndx].count++;
				rangeBegin = 0;
			}
			else if(state == ALL)
			{
				dest[ndx].ranges[dest[ndx].count++].type = RANGE_TYPE_ALL;
			}
			else
			{
				dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_SPECIFIC;
				dest[ndx].ranges[dest[ndx].count++].un.specific = octet;
			}
			ndx++;
			
			state = DEFAULT;
			hadDigit     = 0;
			octet        = 0;
			break;

		default:
			if(isdigit((int)*p))
			{
				// Update the decimal octet value
				octet = (octet * 10) + (*p - '0');
				hadDigit = 1;
				break;
			}
			else
			{
				// This character sequence was unexpected
				return -1;
			}
		}

		/*
		 * next please
		 */
		++p;
		--len;
	}
	
	/*
	 * handle the case where we ran out of buffer
	 */
	if(ndx == 3)
	{
		if(octet > 255 || (hadDigit == 0 && state != ALL) || (listState == IN && state == RANGE))
			return -1;

		if(listState == IN)
		{
			switch(state) {
				case RANGE:
					dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_INCLUSIVE;
					// Set the beginning of the range
					dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = rangeBegin;
					// Set the end of the range
					dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = octet;
					// If the beginning is greater than the end...
					if(dest[ndx].ranges[dest[ndx].count].un.endpoints[0] > dest[ndx].ranges[dest[ndx].count].un.endpoints[1])
					{
						// ... then swap the digits
						int swap = dest[ndx].ranges[dest[ndx].count].un.endpoints[0];
						dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = dest[ndx].ranges[dest[ndx].count].un.endpoints[1];
						dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = swap;
					}
					dest[ndx].count++;
					break;
				case ALL:
					dest[ndx].ranges[dest[ndx].count++].type = RANGE_TYPE_ALL;
					break;
				case DEFAULT:
					dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_SPECIFIC;
					dest[ndx].ranges[dest[ndx].count++].un.specific = octet;
					break;
			}
		}
		else if(state == RANGE)
		{
			dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_INCLUSIVE;
			// Set the beginning of the range
			dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = rangeBegin;
			// Set the end of the range
			dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = octet;
			// If the beginning is greater than the end...
			if(dest[ndx].ranges[dest[ndx].count].un.endpoints[0] > dest[ndx].ranges[dest[ndx].count].un.endpoints[1])
			{
				// ... then swap the digits
				int swap = dest[ndx].ranges[dest[ndx].count].un.endpoints[0];
				dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = dest[ndx].ranges[dest[ndx].count].un.endpoints[1];
				dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = swap;
			}
			dest[ndx].count++;
		}
		else if(state == ALL)
		{
			dest[ndx].ranges[dest[ndx].count++].type = RANGE_TYPE_ALL;
		}
		else
		{
			dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_SPECIFIC;
			dest[ndx].ranges[dest[ndx].count++].un.specific = octet;
		}
		ndx++;
	}
	
	/**
	 * return the result
	 */
	return (ndx == 4 ? 0 : -1);
}

static
const int getIPv6RangeInfo(const char *p, int len, OctetRangeArray_t *const dest)
{
	int ndx        = 0; /* index into the dest parameter */
	int hadDigit   = 0; /* set true when a digit is encountered */
	int octet      = 0; /* the octet value, if any */
	int rangeBegin = 0; /* the octet value, if any */
	bool hexBase = true;
	
	int i;
	
	int state = DEFAULT;
	int listState = OUT;
	
	/*
	 * count the number of colons, must equal 7
	 */
	for(i = 0; i < len && ndx < 7; i++)
	{
		if(*(p+i) == ':')
			++ndx;
	}
	if(ndx != 7)
		return -1;
	
	/*
	 * it's ok so decode it
	 */
	ndx = 0;
	while(len > 0 && ndx < 9)
	{
		switch(*p) {
		case '*':
			/**
			 * sanity check
			 */
			// Must not follow a digit, must not occur in a range, and the next character must be
			// either ':' or '%' or ','
			if(hadDigit || octet != 0 || state == RANGE || rangeBegin != 0 || (len > 1 && (*(p+1) != '%' && *(p+1) != ':' && *(p+1) != ',')))
				return -1;
			
			state = ALL;
			break;

		case ',':
			/**
			 * sanity check 
			 */
			if(octet > 65535)
				return -1;
			
			listState = IN;
			switch(state) {
				case RANGE:
					dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_INCLUSIVE;
					// Set the beginning of the range
					dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = rangeBegin;
					// Set the end of the range
					dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = octet;
					// If the beginning is greater than the end...
					if(dest[ndx].ranges[dest[ndx].count].un.endpoints[0] > dest[ndx].ranges[dest[ndx].count].un.endpoints[1])
					{
						// ... then swap the digits
						int swap = dest[ndx].ranges[dest[ndx].count].un.endpoints[0];
						dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = dest[ndx].ranges[dest[ndx].count].un.endpoints[1];
						dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = swap;
					}
					dest[ndx].count++;
					rangeBegin = 0;
					break;
				case ALL:
					dest[ndx].ranges[dest[ndx].count++].type = RANGE_TYPE_ALL;
					break;
				case DEFAULT:
					dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_SPECIFIC;
					dest[ndx].ranges[dest[ndx].count++].un.specific = octet;
					break;
			}
			
			state = DEFAULT;
			hadDigit = 0;
			octet    = 0;
			break;

		case '-':
			/**
			 * sanity check
			 */
			if(!hadDigit || state == ALL || rangeBegin != 0 || octet > 65535)
				return -1;
			
			if(state != RANGE)
			{
				state = RANGE;
				// Set the beginning of the range
				rangeBegin = octet;
			}
			else
			{
				// You cannot have two dashes within a range
				return -1;
			}
			
			hadDigit = 0;
			octet    = 0;
			break;

		case ':':
		case '%':
			/**
			 * basic sanity check
			 */
			if(octet > 65535 || (hadDigit == 0 && state != ALL))
				return -1;
			
			// If we've hit the scope identifier, then switch to decimal conversion mode
			if (*p == '%') { hexBase = false; }
			
			/**
			 * what type of value is it?
			 */
			if(listState == IN)
			{
				switch(state) {
					case RANGE:
						dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_INCLUSIVE;
						// Set the beginning of the range
						dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = rangeBegin;
						// Set the end of the range
						dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = octet;
						// If the beginning is greater than the end...
						if(dest[ndx].ranges[dest[ndx].count].un.endpoints[0] > dest[ndx].ranges[dest[ndx].count].un.endpoints[1])
						{
							// ... then swap the digits
							int swap = dest[ndx].ranges[dest[ndx].count].un.endpoints[0];
							dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = dest[ndx].ranges[dest[ndx].count].un.endpoints[1];
							dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = swap;
						}
						dest[ndx].count++;
						rangeBegin = 0;
						break;
					case ALL:
						dest[ndx].ranges[dest[ndx].count++].type = RANGE_TYPE_ALL;
						break;
					case DEFAULT:
						dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_SPECIFIC;
						dest[ndx].ranges[dest[ndx].count++].un.specific = octet;
						break;
				}
				listState = OUT;
			}
			else if(state == RANGE)
			{
				dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_INCLUSIVE;
				// Set the beginning of the range
				dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = rangeBegin;
				// Set the end of the range
				dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = octet;
				// If the beginning is greater than the end...
				if(dest[ndx].ranges[dest[ndx].count].un.endpoints[0] > dest[ndx].ranges[dest[ndx].count].un.endpoints[1])
				{
					// ... then swap the digits
					int swap = dest[ndx].ranges[dest[ndx].count].un.endpoints[0];
					dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = dest[ndx].ranges[dest[ndx].count].un.endpoints[1];
					dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = swap;
				}
				dest[ndx].count++;
				rangeBegin = 0;
			}
			else if(state == ALL)
			{
				dest[ndx].ranges[dest[ndx].count++].type = RANGE_TYPE_ALL;
			}
			else
			{
				dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_SPECIFIC;
				dest[ndx].ranges[dest[ndx].count++].un.specific = octet;
			}
			ndx++;
			
			state = DEFAULT;
			hadDigit     = 0;
			octet        = 0;
			break;

		default:
			if(isdigit((int)*p))
			{
				if (hexBase) {
					// Convert the digit and multiply the current 
					// value by 16. IPv6 is colon-separated
					// hexadecimal.
					octet = (octet * 16) + (*p - '0');
				} else {
					// If we're inside the scope identifier,
					// then the digits are decimal integers.
					octet = (octet * 10) + (*p - '0');
				}
				hadDigit = 1;
			}
			else if(*p >= 'A' && *p <= 'F')
			{	
				/*
				* convert the digit and multiply the 
				* current value by 16. IPv6 is 
				* colon-separated hexadecimal.
				*/
				octet = (octet * 16) + (*p - 'A' + 10);
				hadDigit = 1;
			}
			else if(*p >= 'a' && *p <= 'f')
			{	
				/*
				* convert the digit and multiply the 
				* current value by 16. IPv6 is 
				* colon-separated hexadecimal.
				*/
				octet = (octet * 16) + (*p - 'a' + 10);
				hadDigit = 1;
			}
			else
			{
				// This character sequence was unexpected
				return -1;
			}
		}

		/*
		 * next please
		 */
		++p;
		--len;
	}
	
	/*
	 * handle the case where we ran out of buffer
	 */
	if(ndx < 9)
	{
		if(octet > 65535 || (hadDigit == 0 && state != ALL) || (listState == IN && state == RANGE))
			return -1;

		if(listState == IN)
		{
			switch(state) {
				case RANGE:
					dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_INCLUSIVE;
					// Set the beginning of the range
					dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = rangeBegin;
					// Set the end of the range
					dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = octet;
					// If the beginning is greater than the end...
					if(dest[ndx].ranges[dest[ndx].count].un.endpoints[0] > dest[ndx].ranges[dest[ndx].count].un.endpoints[1])
					{
						// ... then swap the digits
						int swap = dest[ndx].ranges[dest[ndx].count].un.endpoints[0];
						dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = dest[ndx].ranges[dest[ndx].count].un.endpoints[1];
						dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = swap;
					}
					dest[ndx].count++;
					break;
				case ALL:
					dest[ndx].ranges[dest[ndx].count++].type = RANGE_TYPE_ALL;
					break;
				case DEFAULT:
					dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_SPECIFIC;
					dest[ndx].ranges[dest[ndx].count++].un.specific = octet;
					break;
			}
		}
		else if(state == RANGE)
		{
			dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_INCLUSIVE;
			// Set the beginning of the range
			dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = rangeBegin;
			// Set the end of the range
			dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = octet;
			// If the beginning is greater than the end...
			if(dest[ndx].ranges[dest[ndx].count].un.endpoints[0] > dest[ndx].ranges[dest[ndx].count].un.endpoints[1])
			{
				// ... then swap the digits
				int swap = dest[ndx].ranges[dest[ndx].count].un.endpoints[0];
				dest[ndx].ranges[dest[ndx].count].un.endpoints[0] = dest[ndx].ranges[dest[ndx].count].un.endpoints[1];
				dest[ndx].ranges[dest[ndx].count].un.endpoints[1] = swap;
			}
			dest[ndx].count++;
		}
		else if(state == ALL)
		{
			dest[ndx].ranges[dest[ndx].count++].type = RANGE_TYPE_ALL;
		}
		else
		{
			dest[ndx].ranges[dest[ndx].count].type = RANGE_TYPE_SPECIFIC;
			dest[ndx].ranges[dest[ndx].count++].un.specific = octet;
		}
		ndx++;
	}
	
	/**
	 * return the result
	 */
	return ((ndx == 8 || ndx == 9) ? 0 : -1);
}

/**
 * Compares the IP Address against the match string
 * and returns true if the IP Address is part of
 * the passed range.
 *
 *
 * Returns:
 *	Returns true if it matches, false if it does not
 *
 */
const bool iplike(const text *const value, const text *const rule)
{
	bool    rcode = false;		/* the return code */
	int     i,j;			/* loop variables */
	int     octets[9];		/* the split apart IP address */
	
	// the matching ranges for each octet
	OctetRangeArray_t ranges[9];
	
	// basic sanity check
	if(value == NULL || rule == NULL)
		return false;
	
	// Initialize the arrays
	for(i = 0; i < 9; i++)
	{
		// Preallocate 255 ranges per octet (to match previous version behavior)
		// TODO: Dynamically allocate ranges as necessary
		const int rangeCount = 255;
		ranges[i].count = 0;
		ranges[i].ranges = (OctetRange_t*)calloc(rangeCount, sizeof(OctetRange_t));
		
		// Init the octet array
		octets[i] = -1;
	}
	
	// Decode the address
	rcode = (convertIP((const char *)VARDATA(value),
			   VARSIZE(value)-VARHDRSZ, 
			   octets) == 0 
		 ? true : false);
	if(rcode == false)
	{
		// Free the allocated ranges
		for (i = 0; i < 9; i++) {
			if (ranges[i].ranges != NULL) free(ranges[i].ranges);
		}
		return false;
	}
	
	/*
	printf("Octets: %d %d %d %d %d %d %d %d %d\n",
		(int)octets[0],
		(int)octets[1],
		(int)octets[2],
		(int)octets[3],
		(int)octets[4],
		(int)octets[5],
		(int)octets[6],
		(int)octets[7],
		(int)octets[8]
	);
	*/
	
	// printf("\nProcessing rule: %s %s\n", (const char *)VARDATA(value), (const char *)VARDATA(rule));
	
	// Decode the filter rule
	rcode = (getRangeInfo((const char *)VARDATA(rule), 
			      VARSIZE(rule)-VARHDRSZ, 
			      ranges) == 0 
		 ? true : false); 
	if(rcode == false)
	{
		// Free the allocated ranges
		for (i = 0; i < 9; i++) {
			if (ranges[i].ranges != NULL) free(ranges[i].ranges);
		}
		return false;
	}
	
	/*
	for (i = 0; i < 9; i++) {
		char rangeString[255];
		getOctetRangeArrayString(ranges[i], rangeString);
		printf("Rules[%d]:  %s\n", i, rangeString);
	}
	*/
	
	// Now do the comparisons
	rcode = true;
	for(i = 0; i < 9 && rcode == true; i++)
	{
		// If we've reached a non-existent octet...
		if (octets[i] < 0) {
			if (ranges[i].count == 0) {
				// If there is no rule for that octet, then break out of the loop
				break;
			} else {
				// Otherwise, the rule cannot be evaluated so the comparison fails.
				// The only time you will really hit this case is when an IPv6 address
				// rule specifies a scope ID but the address in the database contains
				// no scope ID.
				rcode = false;
				break;
			}
		}

		// If there are no rules for this octet, than it passes!
		if (ranges[i].count == 0) {
			rcode = true;
		} else {
			bool arrayCode = false;
			for (j = 0; j < ranges[i].count; j++) {
				OctetRange_t currentRange = ranges[i].ranges[j];
				switch(currentRange.type) {
					case RANGE_TYPE_SPECIFIC:
						arrayCode = arrayCode || (octets[i] == currentRange.un.specific);
						break;
						
					case RANGE_TYPE_INCLUSIVE:
						if(octets[i] < currentRange.un.endpoints[0] || octets[i] > currentRange.un.endpoints[1]) {
							arrayCode = arrayCode || false;
						} else {
							arrayCode = true;
						}
						break;
						
					case RANGE_TYPE_ALL:
						arrayCode = true;
						break;
					
					default:
						arrayCode = arrayCode || false;
						break;
				}
			}
			rcode = arrayCode;
		}
	}
	
	// Free the allocated ranges
	for (i = 0; i < 9; i++) {
		if (ranges[i].ranges != NULL) free(ranges[i].ranges);
	}
	
	return rcode;
}
