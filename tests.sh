#!/bin/sh

# true -> 0
# false -> 1
decode_expected() {
	[ "$1" = "true" ]
	echo "$?"
}

ecode=0

while read value rule expected
do
	case "$value" in
		""|"#"*) continue ;;
	esac

	echo -n . 1>&2

	./iplike "$value" "$rule"
	retval=$?

	if [ "$retval" != "$(decode_expected "$expected")" ]
	then
		echo -e "\nFailed test: value $value, rule $rule," \
			 "expected $expected" 1>&2
		ecode=1
	fi
done < "$srcdir/tests.dat"

echo
exit $ecode
