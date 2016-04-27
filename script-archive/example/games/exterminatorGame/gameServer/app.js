'use strict';

/**
 * Module dependencies.
 */

var express = require('express');
var http = require('http');
var _ = require('underscore');
var shortid = require('shortid');


var app = express();
var server = http.createServer(app);

var WebSocketServer = require('websocket').server;
var wsServer = new WebSocketServer({
    httpServer: server
});

var users = [];
var connections = [];
wsServer.on('request', function(request) {
    console.log("SOMEONE JOINED");
    var connection = request.accept(null, request.origin);
    connections.push(connection);
    connection.on('message', function(data) {
        var userData = JSON.parse(data.utf8Data);
        var user = _.find(users, function(user) {
            return user.username === userData.username;
        });
        if (user) {
            // This user already exists, so just update score
            users[users.indexOf(user)].score = userData.score;
        } else {
            users.push({
                id: shortid.generate(),
                username: userData.username,
                score: userData.score
            });
        }
        connections.forEach(function(aConnection) {
            aConnection.sendUTF(JSON.stringify({
                users: users
            }));
        })
    });
});

app.get('/users', function(req, res) {
    res.send({
        users: users
    });
});



/* Configuration */
app.set('views', __dirname + '/views');
app.use(express.static(__dirname + '/public'));
app.set('port', (process.env.PORT || 5000));

if (process.env.NODE_ENV === 'development') {
    app.use(express.errorHandler({
        dumpExceptions: true,
        showStack: true
    }));
}


/* Start server */
server.listen(app.get('port'), function() {
    console.log('Express server listening on port %d in %s mode', app.get('port'), app.get('env'));
});

module.exports = app;