#!/bin/sh

NEWVER="$1"
MYDIR=`dirname $0`
MYROOT=`cd $MYDIR; pwd`

if [ -z "$NEWVER" ]; then
	echo "usage: $0 <pgversion>"
	echo ""
	exit 1
fi

SHORTVER=`echo $NEWVER | sed -e 's,\.,,g'`

echo "PostgreSQL version: $NEWVER"
echo "Short version: $SHORTVER"

for INFILE in $MYROOT/*.tmpl; do
	OUTFILE=`echo $INFILE | sed -e 's,.tmpl$,,'`
	echo "$INFILE -> $OUTFILE"
	sed -e "s,@POSTGRESQL_VERSION@,${NEWVER},g" -e "s,@PGSQL_TYPE@,${SHORTVER},g" "$INFILE" > "$OUTFILE"
done
