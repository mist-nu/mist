if (process.argv.length < 4) {
    console.log("Usage: node tool.js WORKDIR TORPATH")
    process.exit();
}

var fs = require("fs");
var path = require("path");
var mist = require("../build/Debug/mist.node");
var parse = require('shell-quote').parse;
var rl = require('readline').createInterface({
    input: process.stdin,
    output: process.stdout
});

var started = false;

function mkdirSync(path) {
    try {
        fs.mkdirSync(path);
    } catch (e) {
        if (e.code != 'EEXIST') throw e;
    }
}

var workdir = process.argv[2];
var torPath = process.argv[3];

mkdirSync(workdir);

var central = new mist.Central(workdir, false);

function create() {
    central.create();
}

function init(startCb, exitCb) {
    central.init();
    console.log("My SHA3 fingerprint is " + central.getPublicKey().fingerprint());
    central.startEventLoop();
    console.log("Starting Tor...");
    central.startServeTor(torPath,
        6000, 6100, 6200, 6300, 6400, 6500, startCb, exitCb);

    var myPubKey = central.getPublicKey();

    fs.writeFile(path.join(workdir, "key.pem"), myPubKey.toString());
}

function publicKey() {
    console.log(central.getPublicKey().toString());
}

function fingerprint() {
    console.log(central.getPublicKey().fingerprint());
}

function createDatabase(dbName) {
    var db = central.createDatabase(dbName);
    console.log( typeof db );
    console.log( typeof db.getManifest() );
    console.log( typeof db.getManifest().getHash() );
    console.log( db.getManifest().getHash().toString() );
}

function listDatabases() {
    central.listDatabases().forEach(function (database) {

        // TODO: Printing a database object crashes..!
        console.log("<database>");
	console.log( "hej" );
	console.log( database === undefined );
	console.log( database === null );
	console.log( typeof database );
	console.log( typeof central );
	for (var i in database) console.log( i );
	console.log( "hej 1" );
	database.toString();
	console.log( "hej 2" );
	database.getHash();
	console.log( "hopp" );
	console.log( database.getHash().toString() );
        //console.log(database);
    });
}

function addPeer(keyFile, name, peerStatus, anonymous) {
    var pubKeyPem;
    try {
        pubKeyPem = fs.readFileSync(keyFile);
    } catch (e) {
        console.log("Unable to read keyfile");
        return;
    }
    var pubKey = mist.PublicKey.fromPem(pubKeyPem);
    console.log("Adding peer " + name + " with hash " + pubKey.hash());
    central.addPeer(pubKey, name, peerStatus, anonymous);
}

function changePeer(hash, status, anonymous) {
    central.changePeer(hash, status, anonymous);
}

function removePeer(hash) {
    central.removePeer(hash);
}

function listPeers() {
    central.listPeers().forEach(function (peer) {
        console.log(peer);
    });
}

function getPeer(hash) {
    central.getPeer(hash);
}

function getPendingInvites(hash) {
    central.getPendingInvites(hash);
}

function addAddressLookupServer(address, port) {
    central.addAddressLookupServer(address, port);
}

function removeAddressLookupServer(address, port) {
    central.removeAddressLookupServer(address, port);
}

function listAddressLookupServers() {
    central.listAddressLookupServers().forEach(function (server) {
        console.log(server);
    });
}

function listDatabasePermissions() {
    central.listDatabasePermissions().forEach(function (permission) {
        console.log(permission);
    })
}

function addServicePermission(peerHash, service, path) {
    central.addServicePermission(peerHash, service, path);
}

function removeServicePermission(peerHash, service, path) {
    central.removeServicePermission(peerHash, service, path);
}

function listServicePermissions(peerHash) {
    central.listServicePermissions().forEach(function (service) {
        console.log(service);
    })
}

function startSync() {
    central.startSync();
}

function stopSync() {
    central.stopSync();
}

function listServices() {
    central.listServices().forEach(function (service) {
        console.log(service)
    });
}

function inviteUserToDatabase(dbHash, keyFile, name, permission) {
    var pubKeyPem;
    try {
        pubKeyPem = fs.readFileSync(keyFile);
    } catch (e) {
        console.log("Unable to read keyfile");
        return;
    }
    var pubKey = mist.PublicKey.fromPem(pubKeyPem);
    var db = central.getDatabase( dbHash );
    console.log("Inviting user " + name + " with hash " + pubKey.hash() + " to database " + dbHash);
    db.inviteUser(name, pubKey, permission);
}

var torStarted = false;

var userQuery = function () {
    rl.question('mist> ', (command) => {
        try {
            var xs = parse(command);
            if (xs.length < 1) {
                // Empty string
            } else if (xs[0] == "quit") {
                central.close();
                rl.close();
                process.exit();
            } else if (xs[0] == "create") {
                create();
            } else if (xs[0] == "init") {
                init(function () {
                    console.log("Tor started");
                    userQuery();
                    torStarted = true;
                }, function () {
                    console.log("Tor exited");
                    if (!torStarted)
                        userQuery();
                    torStarted = false;
                });
                return;
            } else if (xs[0] == "public-key") {
                publicKey();
            } else if (xs[0] == "fingerprint") {
                fingerprint();
            } else if (xs[0] == "create-database") {
                if (xs.length < 2) {
                    console.log("Usage: create-dbatabase NAME");
                } else {
                    var dbName = xs[1];
                    createDatabase(dbName);
                }
            } else if (xs[0] == "list-databases") {
                listDatabases();
            } else if (xs[0] == "add-peer") {
                if (xs.length < 5) {
                    console.log("Usage: add-peer KEYFILE NAME STATUS ANONYMOUS");
                } else {
                    var keyFile = xs[1];
                    var name = xs[2];
                    var peerStatus = xs[3];
                    var anonymous = xs[4];
                    addPeer(keyFile, name, peerStatus, anonymous);
                }
            } else if (xs[0] == "change-peer") {
                if (xs.length < 3) {
                    console.log("Usage: change-peer PEER_HASH STATUS ANONYMOUS");
                } else {
                    var hash = mist.SHA3.fromString(xs[1]);
                    var peerStatus = xs[1];
                    var anonymous = xs[2];
                    changePeer(hash, peerStatus, anonymous);
                }
            } else if (xs[0] == "remove-peer") {
                if (xs.length < 3) {
                    console.log("Usage: remove-peer PEER_HASH");
                } else {
                    var hash = mist.SHA3.fromString(xs[1]);
                    removePeer(hash);
                }
            } else if (xs[0] == "list-peers") {
                listPeers();
            } else if (xs[0] == "get-peer") {
                if (xs.length < 2) {
                    console.log("Usage: get-peer PEER_HASH");
                } else {
                    var hash = mist.SHA3.fromString(xs[1]);
                    getPeer(hash);
                }
            } else if (xs[0] == "get-pending-invites") {
                getPendingInvites();
            } else if (xs[0] == "add-address-lookup-server") {
                if (xs.length < 3) {
                    console.log("Usage: add-address-lookup-server ADDRESS PORT");
                } else {
                    var address = xs[1];
                    var port = xs[2];
                    addAddressLookupServer(address, port);
                }
            } else if (xs[0] == "remove-address-lookup-server") {
                if (xs.length < 3) {
                    console.log("Usage: remove-address-lookup-server ADDRESS PORT");
                } else {
                    var address = xs[1];
                    var port = xs[2];
                    removeAddressLookupServer(address, port);
                }
            } else if (xs[0] == "list-address-lookup-servers") {
                listAddressLookupServers();
            } else if (xs[0] == "list-database-permissions") {
                listDatabasePermissions();
            } else if (xs[0] == "add-service-permission") {
                if (xs.length < 3) {
                    console.log("Usage: add-service-permission PEER_HASH SERVICE PATH");
                } else {
                    var peerHash = mist.SHA3.fromString(xs[1]);
                    var service = xs[2];
                    var path = xs[3];
                    addServicePermission(peerHash, service, path);
                }
            } else if (xs[0] == "remove-service-permission") {
                if (xs.length < 3) {
                    console.log("Usage: remove-service-permission PEER_HASH SERVICE PATH");
                } else {
                    var peerHash = mist.SHA3.fromString(xs[1]);
                    var service = xs[2];
                    var path = xs[3];
                    removeServicePermission(peerHash, service, path);
                }
            } else if (xs[0] == "list-service-permissions") {
                if (xs.length < 2) {
                    console.log("Usage: list-service-permissions PEER_HASH");
                } else {
                    var peerHash = mist.SHA3.fromString(xs[1]);
                    listServicePermissions(peerHash);
                }
            } else if (xs[0] == "start-sync") {
                startSync();
            } else if (xs[0] == "stop-sync") {
                stopSync();
            } else if (xs[0] == "list-services") {
                listServices();
            } else if (xs[0] == "invite-user-to-db") {
                if (xs.length < 4) {
                    console.log("Usage: invite-user-to-db DB_HASH NAME KEYFILE PERMISSION");
                } else {
                    var dbHash = mist.SHA3.fromString(xs[1]);
                    var keyFile = xs[3];
                    var name = xs[2];
                    var permission = xs[4];
                    inviteUserToDatabase(dbHash, keyFile, name, permission);
                }
            } else if (xs[0] == "help") {
                console.log("Available commands:");
                console.log("  create");
                console.log("  init");
                console.log("  quit");
                console.log("  public-key");
                console.log("  fingerprint")
                console.log("  create-database NAME");
                console.log("  list-databases");
                console.log("  add-peer NAME KEYFILE STATUS ANONYMOUS");
                console.log("  remove-peer PEER_HASH");
                console.log("  change-peer PEER_HASH STATUS ANONYMOUS");
                console.log("  get-peer PEER_HASH");
                console.log("  list-peers");
                console.log("  add-address-lookup-server ADDRESS PORT");
                console.log("  remove-address-lookup-server ADDRESS PORT");
                console.log("  list-address-lookup-servers");
                console.log("  list-database-permissions PEER_HASH");
                console.log("  add-service-permission PEER_HASH SERVICE PATH");
                console.log("  remove-service-permission PEER_HASH SERVICE PATH");
                console.log("  list-service-permissions PEER_HASH");
                console.log("  start-sync");
                console.log("  stop-sync");
                console.log("  list-services");
		console.log("  invite-user-to-db DB_HASH NAME KEYFILE PERMISSION");
		console.log("  list-db-invites");
		console.log("  accept-db-invite DB_HASH");
            } else {
                console.log("Unrecognized command");
            }
        } catch (e) {
            console.log("Error: " + e.toString());
        }
        userQuery();
    });
}

userQuery();
