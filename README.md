#Mist Library

Mist provides encrypted, anonymous connections and shared databases
for open source, peer to peer applications. Mist consists of a connection and
database library, as well as an address lookup service.

The development of Mist is sponsored by IIS (The Internet Foundation in Sweden).

The connection library creates end-to-end encrypted connections
between peers. A peer is usually assumed to be an user, but could be
an IoT device or something else. The library uses the HTTP/2
protocol. It can multiplex any number of services ontop of a single
HTTP/2 connection. It uses TOR to anonymously establish connection,
but also supports direct connections for performance, for applications
or users who do not need full anonymity.

The database library stores JSON objects in a schema-less object
store. The database is append only, it stores every version of an
object and can only grow, not shrink. It is thus excellent for
applications that require versioning. It allows replication in any
direction. It performs a best effort conflict resolution, in cases
where changes are made in parallell. The database will always be in a
valid state, but some changes may be invisible due to the conflict
resolution. Such changes will still be available as an older version. 

Mist uses the NSS library for TLS, the TOR client for anonymous
connection and hidden service, the nginx library for HTTP/2 and
SQLite3 for storage. 

For this release the only way to use Mist is as a library to build
your own peer-to-peer application, for use on Window, MacOS X, Linux
or other Unix derived operating systems. However we intend to continue
to develop Mist so supports more platforms and becomes easier to
use.

## Usage

Make sure you use Mist on a computer that has the correct time. Mist
will not function properly between computers that do not share the
same time.

Install all prerequisite and compile the library. See below.

### Central

Start by creating a Mist `Central` object, then call the `create`
method. Mist will automatically create an RSA key pair and some SQLite
databases. Next time you want to start Mist, call `init` since it has
already created everything needed.

Then you have a few choices. You can either interact with Mist
databases, or start direct connections with peers. Databases are
managed by the `getDatabase`, `createDatabase`, `receiveDatabase` and
`listDatabases` methods. Databases can replicate their content between
users, this is started with `startSync` and paused with `stopSync`.

In order to start any networking, such as syncing databases, you need to call
`addAddressLookupServer`. There is a public address lookup server at
`lookup.mist.nu` port `40443`. The Mist library stores your address
lookup servers, you only have to add them once.

Connections between peers are made through services. A peer has a
list of services, as well as a simple access control system that
determines which peers may connect to a certain service. Services
either use a virtual socket interface, or send http2 requests.

As a user of the library you need to register services other peer can
connect to, with `registerService` or `registerHttpService`. You
control peer access with `addPeer`, `changePeer`, `removePeer`,
`listPeers`, `getPeer`, `addServicePermission` and
`removeServicePermission`.

You can connect to services at other peers with `openServiceSocket` or
`openServiceRequest`.

### Database

The database stores versioned JSON objects. It has a simple query
language that operates on the name/value pair of each JSON object. We
call these name/value pairs for attributes.

Each JSON object has a parent relationship. Queries operate on child
objects.

Queries have three parts, `select`, `filter` and `sort`.

**Select** makes it possible to limit the attributes that are returned
from the query, or to run a function on one attribute. Available
functions are `avg`, `count`, `min`, `max` and `sum`. For example:

`sum( o.*attribute* )`

Example of a select to limit attributes:

`o.*attribute1*, o.*attribute2*, *...* o.*attributeN*`

Leave the select part empty if you do not want to limit the attributes
returned.

**Filter** is a JavaScript expression that limites the child objects
returned in the query. It takes a map with arguments, just like `?`
arguments in SQL. However the available expressions are limited to
`(`, `)`, `!`, `||`, `&&`, `==`, `!=`, `<`, `<=`, `>` and `>`. The
`<`, `<=`, `>` and `>` are restricted to number attributes. Each
comparison must have an attribute on the left or right hand, like
`o.*attribute*`. The other side may be an attribute, and argument in
the form of `a.*argument*` or a string, number, boolean or null
constant.

For example:

`o.len <= a.maxLen && o.len >= a.minLen || o.name == 'Kalle'`

**Sort** just specifies an attribute that will be used to sort the
result. To sort in decending order use the function `desc`. For
example:

`o.name`  
`desc( o.name )`

Apart from these query expressions the query can also specify a max
version, if you want to query the state of the database at a specific
time.

Furthermore, it is possible to subscribe to queries. Then a callback
will deliver new results in case the state of the database has
changed. That way your application can make sure it always shows the
current information.


### Transaction

To change information in the database, use the `beginTransaction`
method. This returns a transaction object, that can change the
database. Available methods are `newObject`, `moveObject`,
`updateObject`, `deleteObject`, `commit` and `rollback`.

**newObject** creates a new object.

**moveObject** changes the parent relationship of the object.

**updateObject** changes the attributes of the object.

**deleteObject** sets the deleted status of an object. The old version
is still available in the database.

**commit** commits the transaction. It will immediatly be available
for local users, and eventuallt be replicated to other users.

**rollback** rollback the transaction. Nothing will be saved.


### Access control

The Mist database has a simple access control schema. A user may have
`read`, `write` and `admin` permission. The creator of a database will
have implicit admin permission. Other users get permission by being
added as a child object to the `USERS` object in the `settings` access
domain. Only users with admin permission can perform changes to the
settings access domain.

Users who have permission to a database you have downloaded, or
created, will automatically be added as authorized peers.

## Building

### Prerequisits
Currely supported OSes are **Linux**, **OS X** and **Windows**.

The project is based on **C++11**, so that is a requirement as well,
currently tested with **gcc 4.9** and later.

To compile the node-wrapper, **node v4** or later is required. Tested with **4.6.0** and later. https://nodejs.org/ 

Futhermore the **boost** libraries are required to build this library.

You can get boost from your Linux repo e.g. `apt install
libboost-all-dev`, on OS X you can get it with `brew install
boost`. Alternativly you can download it from their website:
http://www.boost.org/

Using `boost::filesystem` version 3 from boost **1.50** or above. The filesystem has to be compiled if you're using sources, it should be precompiled in most cases.

Currently you must have cmake, can be obtained with `apt install
cmake` **or** `brew install cmake`  
Other than that it can be downloaded from https://cmake.org/download/

You must also have the *nss* library, the *nginx http/2* library,
*sqlite3*  and the *TOR* client.

### Install

Clone this repository.

#### The first alternative (recommended):

##### Currently clunky part:
You have to init the git submodules with `git submodule init` **and** `git submodule update`
After that you have to build the logger by doing the following or equvivilant: 
```cd lib/g3log
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCHANGE_G3LOG_DEBUG_TO_DBUG=On ..
make
cd ../..
``` 
##### And the rest
On Linux and OS X:

```npm install
```

Built files can be found in: `build/Debug`

#### The second alternative:
Compile the lib manully.

View the bindings.gyp file to see what directories to include,  flags to use and libraries to link.

### Test the library

#### With google test framework (recommended)

##### Prerequisits

The clunky part from the install above.

##### The easy way
```cd gtests
./init_build.sh 
./build_and_run_gtest.sh```

##### A more descriptive way
Run the following to download googletest:

```git submodule init
git submodule update
```

Then go to `gtest` and create a directory called e.g. `build`:
```mkdir build
cd build
```

Then `cmake` can create the build `Makefile` file for you.

Example on how to do on Linux:

`cmake -Dtest=ON .. && make && ./testDatabase ; rm test.db`

Where `-Dtest=ON` enables the building of the test files, `..` is the directory to get the `CMakeLists.txt` from. The rest should be self explanatory.

## Contributors

Andreas Enström  
Oskar Holstensson  
Mattias Wingstedt

## Sponsor

Mist is sponsored by IIS (The Internet Foundation in Sweden). IIS finances independent, noncommercial projects that promote internet development in Sweden through the Internet Fund. Since its launch in 2004, the fund has financed over 350 projects. Private individuals and organizations are welcome to apply for financing.

## Copyright

The Mist Library copyright is owned by VISIARC AB.

## License

The Mist Library is licensed under the GNU General Public License, version 3 or later.
