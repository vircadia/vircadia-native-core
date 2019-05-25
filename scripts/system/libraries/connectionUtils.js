//
//  connectionUtils.js
//  scripts/system/libraries
//
//  Copyright 2018 High Fidelity, Inc.
//

// Function Names:
//   - requestJSON
//   - getAvailableConnections
//   - getInfoAboutUser
//   - getConnectionData
//
// Description:
//   - Update all the usernames that I am entitled to see, using my login but not dependent on canKick.
var request = Script.require('request').request;
var METAVERSE_BASE = Account.metaverseServerURL;
function requestJSON(url, callback) { // callback(data) if successfull. Logs otherwise.
    request({
        uri: url
    }, function (error, response) {
        if (error || (response.status !== 'success')) {
            print("Error: unable to get URL", error || response.status);
            return;
        }
        callback(response.data);
    });
}
function getAvailableConnections(domain, callback) { // callback([{usename, location}...]) if successful. (Logs otherwise)
    url = METAVERSE_BASE + '/api/v1/users?per_page=400&'
    if (domain) {
        url += 'status=' + domain.slice(1, -1); // without curly braces
    } else {
        url += 'filter=connections'; // regardless of whether online
    }
    requestJSON(url, function (connectionsData) {
        callback(connectionsData.users);
    });
}
function getInfoAboutUser(specificUsername, callback) {
    url = METAVERSE_BASE + '/api/v1/users?filter=connections'
    requestJSON(url, function (connectionsData) {
        for (user in connectionsData.users) {
            if (connectionsData.users[user].username === specificUsername) {
                callback(connectionsData.users[user]);
                return;
            }
        }
        callback(false);
    });
}
getConnectionData = function getConnectionData(specificUsername, domain) {
    function frob(user) { // get into the right format
        var formattedSessionId = user.location.node_id || '';
        if (formattedSessionId !== '' && formattedSessionId.indexOf("{") != 0) {
            formattedSessionId = "{" + formattedSessionId + "}";
        }
        return {
            sessionId: formattedSessionId,
            userName: user.username,
            connection: user.connection,
            profileUrl: user.images.thumbnail,
            placeName: (user.location.root || user.location.domain || {}).name || ''
        };
    }
    if (specificUsername) {
        getInfoAboutUser(specificUsername, function (user) {
            if (user) {
                updateUser(frob(user));
            } else {
                print('Error: Unable to find information about ' + specificUsername + ' in connectionsData!');
            }
        });
    } else {
        getAvailableConnections(domain, function (users) {
            if (domain) {
                users.forEach(function (user) {
                    updateUser(frob(user));
                });
            } else {
                sendToQml({ method: 'updateConnections', connections: users.map(frob) });
            }
        });
    }
}

// Function Name: sendToQml()
//
// Description:
//   -Use this function to send a message to the QML (i.e. to change appearances). The "message" argument is what is sent to
//    the QML in the format "{method, params}", like json-rpc. See also fromQml().
function sendToQml(message) {
    Tablet.getTablet("com.highfidelity.interface.tablet.system").sendToQml(message);
}
