# Advanced IP Address String Matching for PostgreSQL

While PostgreSQL has native support for IP addresses, there are use
cases for text columns when you either have a legacy database
schema or need more control over how addresses are stored.

IPLIKE is a PostgreSQL plugin for doing wildcard-like IP address
matching using a simple rule grammar.

# Building from Source

## Preparing a Git Checkout for Building

If you checked IPLIKE out from Git, you will need to prepare the build
directory.  If you downloaded a source tarball you can skip this step.

You will need to have GNU Autoconf, Automake, and Libtool installed.

```
git submodule init
git submodule update
autoreconf -fvi
```

## Building

Building uses the standard GNU Autoconf process:


```
./configure
make
sudo make install
```

If your PostgreSQL is in a non-standard location, you can compile
against it by passing `--with-pgsql=/path/to/pg_config` to the
`./configure` command.

## Installing IPLIKE to PostgreSQL

You should be able to run the `install_iplike.sh` script to insert
IPLIKE into your database.  By default it will associate it with
a database called `opennms`, but you can choose a different database
by passing the `-d` flag (eg, `-d template1`).

## Using IPLIKE

IPLIKE uses a very flexible search format, allowing you to separate
the octets (fields) of an IPv4 or IPv6 address into specific searches.
An asterisk (*) in place of any octet matches any value for that octet.
Ranges are indicated by two numbers separated by a dash (-),
and commas are used for lists of matching octets.

### Example Queries

Match all addresses from 192.168.0.0 through 192.168.255.255:

* `SELECT iplike(ipaddr, '192.168.*.*');`
* `SELECT iplike(ipaddr, '192.168.0-255.0-255');`
* `SELECT iplike(ipaddr, '192.168.*.0-255');`

Match any link-local IPv6 address:

* `SELECT iplike(ipaddr, 'fe80:*:*:*:*:*:*:*');`

