if (process.argv.length < 4) {
    console.log("Usage: node tool.js WORKDIR TORPATH")
    process.exit();
}

var fs = require("fs");
var path = require("path");
var mist = require("./mist.js");
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
var chatSockets = Object();

function readAll(readable, callback) {
    str = "";
    readable.on('data', function (chunk) {
        str += chunk;
    });
    readable.on('end', function () {
        callback(str);
    });
}

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
        console.log(database.toString());
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
    central.startSync(function (manifest) {
        console.log("Received database invite for " + manifest.getHash().toString());
        console.log(manifest.toString());
        central.receiveDatabase(manifest);
    }, true);
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

function listDbInvites()
{
    central.getPendingInvites().forEach( m => {
        console.log( m.getHash().toString() );
    });
}

function acceptDbInvite( dbHash )
{
    central.getPendingInvites().forEach( m => {
        if (m.getHash() == dbHash)
            central.receiveDatabase( m );
    });
}

function createTestObject(dbHash, name) {
    var db = central.getDatabase(mist.SHA3.fromString(dbHash));
    var t = db.beginTransaction(mist.Database.AccessDomain.Normal);
    var refA
        = new mist.Database.ObjectRef(mist.Database.AccessDomain.Normal, 0);
    console.log("New object A ref " + refA.toString());
    var idA = t.newObject(refA, {
        name: name,
        test1: 17,
        test2: "test",
    });
    console.log("New object A " + idA);
    var refB
        = new mist.Database.ObjectRef(mist.Database.AccessDomain.Normal, idA);
    console.log("New object B ref " + refB.toString());
    var idB = t.newObject(refB, {
        name: name + "_B",
        test1: 18,
        test2: "testB",
    });
    console.log("New object B " + idA);
    t.commit();
    console.log("Committed");
}

function startHttpChat() {
    var db = central.registerHttpService("http-", function (request)
    {
        readAll(request.readStream(), function (keyHash, message, path) {
            console.log("Got http chat message '" + message + "' from " + keyHash.toString());
        })
    });
}

function sayHttp(keyHash, message) {
    central.openServiceRequest(mist.SHA3.fromString(keyHash), "http-chat", "post",
        "/", function (keyHash, request) {
            console.log("Sent http chat message '" + message + "' to " + keyHash.toString());
            request.end(message);
        });
}

function registerSocketChatSocket(keyHash, socket) {
    if (chatSockets[keyHash.toString()] !== undefined) {
        console.log("We already have a socket to this peer");
    } else {
        socket.stream().on('data', function (chunk) {
            console.log("Got message '" + chunk + "' from " + keyHash.toString());
        })
        chatSockets[keyHash.toString()] = socket;
    }
}

function startSocketChat() {
    var db = central.registerService("socket-chat", function (keyHash, socket, path) {
        console.log("Incoming chat socket from " + keyHash.toString());
        registerSocketChatSocket(keyHash, socket)
    });
}

function openSocketChat(keyHash) {
    if (chatSockets[keyHash.toString()] !== undefined) {
        console.log("We already have a socket to " + keyHash.toString());
    } else {
        central.openServiceSocket(mist.SHA3.fromString(keyHash), "socket-chat", "/",
            function (keyHash, socket) {
                console.log("Opened chat socket to " + keyHash.toString());
                registerSocketChatSocket(keyHash, socket);
            });
    }
}

function saySocket(keyHash, message) {
    if (chatSockets[keyHash.toString()] === undefined) {
        console.log("No socket open for " + keyHash.toString());
    } else {
        socket = chatSockets[keyHash.toString()]
        socket.stream().write(message);
        console.log("Wrote '" + message + "' to " + keyHash.toString());
    }
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
                if (xs.length < 2) {
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
            } else if (xs[0] == "list-db-invites") {
                listDbInvites();
            } else if (xs[0] == "accept-db-invite") {
                if (xs.length < 2) {
                    console.log("Usage: accept-db-invite DB_HASH");
                } else {
                    var dbHash = mist.SHA3.fromString(xs[1]);
                    acceptDbInvite(dbHash);
                }
            } else if (xs[0] == "create-test-object") {
                if (xs.length < 3) {
                    console.log("Usage: create-test-object DB_HASH NAME");
                } else {
                    var dbHash = mist.SHA3.fromString(xs[1]);
                    var name = xs[2];
                    createTestObject(dbHash, name);
                }
            } else if (xs[0] == "start-http-chat") {
                startHttpChat();
            } else if (xs[0] == "say-http") {
                if (xs.length < 3) {
                    console.log("Usage: say-http PEER_HASH MESSAGE");
                } else {
                    var peerHash = mist.SHA3.fromString(xs[1]);
                    var message = xs[2];
                    sayHttp(peerHash, message);
                }
            } else if (xs[0] == "start-socket-chat") {
                startSocketChat();
            } else if (xs[0] == "open-socket-chat") {
                if (xs.length < 2) {
                    console.log("Usage: open-socket-chat PEER_HASH");
                } else {
                    var peerHash = mist.SHA3.fromString(xs[1]);
                    openSocketChat(peerHash);
                }
            } else if (xs[0] == "say-socket") {
                if (xs.length < 3) {
                    console.log("Usage: say-http PEER_HASH MESSAGE");
                } else {
                    var peerHash = mist.SHA3.fromString(xs[1]);
                    var message = xs[2];
                    saySocket(peerHash, message);
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
                console.log("  create-test-object NAME");
                console.log("  start-http-chat");
                console.log("  say-http PEER_HASH MESSAGE");
                console.log("  start-socket-chat");
                console.log("  open-socket-chat PEER_HASH");
                console.log("  say-socket PEER_HASH MESSAGE");
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
