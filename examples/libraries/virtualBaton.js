"use strict";
/*jslint nomen: true, plusplus: true, vars: true */
/*global Messages, Script, MyAvatar, AvatarList, Entities, print */

//  Created by Howard Stearns
//  Copyright 2016 High Fidelity, Inc.
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Allows cooperating scripts to pass a "virtual baton" between them,
//  which is useful when part of a script should only be executed by
//  the one participant that is holding this particular baton.
// 
//  A virtual baton is simply any string agreed upon by the scripts
//  that use it. Only one script at a time can hold the baton, and it
//  holds it until that script releases it, or the other scripts
//  determine that the holding script is not responding. The script
//  automatically determines who among claimants has the baton, if anyone,
//  and holds an "election" if necessary.
//
//  See entityScript/tribble.js as an example, and the functions
//  virtualBaton(), claim(), release().
//

// Answers a new virtualBaton for the given parameters, of which 'key'
// is required.
function virtualBatonf(options) {
    // Answer averages (number +/- variability). Avoids having everyone act in lockstep.
    function randomize(number, variability) {
        var allowedDeviation = number * variability;
        return number - allowedDeviation + (Math.random() * 2 * allowedDeviation);
    }
    // Allow testing outside in a harness outside of High Fidelity.
    // See sourceCodeSandbox/tests/mocha/test/testVirtualBaton.js
    var globals = options.globals || {},
        messages = globals.Messages || Messages,
        myAvatar = globals.MyAvatar || MyAvatar,
        avatarList = globals.AvatarList || AvatarList,
        entities = globals.Entities || Entities,
        timers = globals.Script || Script,
        log = globals.print || print;

    var batonName = options.batonName,  // The identify of the baton.
        // instanceId is the identify of this particular copy of the script among all copies using the same batonName
        // in the domain. For example, if you wanted only one entity among multiple entity scripts to hold the baton,
        // you could specify virtualBaton({batonName: 'someBatonName', instanceId: MyAvatar.sessionUUID + entityID}).
        instanceId = options.instanceId || myAvatar.sessionUUID,
        // virtualBaton() returns the exports object with properties. You can pass in an object to be side-effected.
        exports = options.exports || {},
        // Handy to set false if we believe the optimizations are wrong, or to use both values in a test harness.
        useOptimizations = (options.useOptimizations === undefined) ? true : options.useOptimizations,
        electionTimeout = options.electionTimeout || randomize(500, 0.2), // ms. If no winner in this time, hold a new election.
        recheckInterval = options.recheckInterval || randomize(500, 0.2), // ms. Check that winners remain connected.
        // If you supply your own instanceId, you might also supply a connectionTest that answers
        // truthy iff the given id is still valid and connected, and is run at recheckInterval. You
        // can use exports.validId (see below), and the default answers truthy if id is valid or a
        // concatenation of two valid ids. (This handles the most common cases of instanceId being
        // either (the default) MyAvatar.sessionUUID, an entityID, or the concatenation (in either
        // order) of both.)
        connectionTest = options.connectionTest || function connectionTest(id) {
            var idLength = 38;
            if (id.length === idLength) { return exports.validId(id); }
            return (id.length === 2 * idLength) && exports.validId(id.slice(0, idLength)) && exports.validId(id.slice(idLength));
        };

    if (!batonName) {
        throw new Error("A virtualBaton must specify a batonName.");
    }
    // Truthy if id exists as either a connected avatar or valid entity.
    exports.validId = function validId(id) {
        var avatar = avatarList.getAvatar(id);
        if (avatar && (avatar.sessionUUID === id)) { return true; }
        var properties = entities.getEntityProperties(id, ['type']);
        return properties && properties.type;
    };

    // Various logging, controllable through options.
    function debug() { // Display the arguments not just [Object object].
        log.apply(null, [].map.call(arguments, JSON.stringify));
    }
    function debugFlow() {
        if (options.debugFlow) { debug.apply(null, arguments); }
    }
    function debugSend(destination, operation, data) {
        if (options.debugSend) { debug('baton:', batonName, instanceId, 's=>', destination, operation, data); }
    }
    function debugReceive(senderID, operation, data) { // senderID is client sessionUUID -- not necessarily instanceID!
        if (options.debugReceive) { debug('baton:', batonName, senderID, '=>r', instanceId, operation, data); }
    }

    // Messages: Just synactic sugar for hooking things up to Messages system.
    // We create separate subchannel strings for each operation within our general channelKey, instead of using
    // a switch in the receiver.
    var channelKey = "io.highfidelity.virtualBaton:" + batonName,
        subchannelHandlers = {}, // Message channel string => {receiver, op}
        subchannelKeys = {};     // operation => Message channel string
    function subchannelKey(operation) { return channelKey + ':' + operation; }
    function receive(operation, handler) { // Record a handler for an operation on our channelKey
        var subKey = subchannelKey(operation);
        subchannelHandlers[subKey] = {receiver: handler, op: operation};
        subchannelKeys[operation] = subKey;
        messages.subscribe(subKey);
    }
    function sendHelper(subchannel, data) {
        var message = JSON.stringify(data);
        messages.sendMessage(subchannel, message);
    }
    function send1(operation, destination, data) { // Send data for an operation to just one destination on our channelKey.
        debugSend(destination, operation, data);
        sendHelper(subchannelKey(operation) + destination, data);
    }
    function send(operation, data) {  // Send data for an operation on our channelKey.
        debugSend('-', operation, data);
        sendHelper(subchannelKeys[operation], data);
    }
    function messageHandler(channel, messageString, senderID) {
        var handler = subchannelHandlers[channel];
        if (!handler) { return; }
        var data = JSON.parse(messageString);
        debugReceive(senderID, handler.op, data);
        handler.receiver(data);
    }
    messages.messageReceived.connect(messageHandler);

    var nPromises = 0, nAccepted = 0, electionWatchdog;

    // It would be great if we had a way to know how many subscribers our channel has. Failing that...
    var nNack = 0, previousNSubscribers = 0, lastGathering = 0, thisTimeout = electionTimeout;
    function nSubscribers() { // Answer the number of subscribers.
        // To find nQuorum, we need to know how many scripts are being run using this batonName, which isn't
        // the same as the number of clients!
        //
        // If we overestimate by too much, we may fail to reach consensus, which triggers a new
        // election proposal, so we take the number of acceptors to be the max(nPromises, nAccepted)
        // + nNack reported in the previous round.
        //
        // If we understimate by too much, there can be different pockets on the Internet that each
        // believe they have agreement on different holders of the baton, which is precisely what
        // the virtualBaton is supposed to avoid. Therefore we need to allow 'nack' to gather stragglers.

        var now = Date.now(), elapsed = now - lastGathering;
        if (elapsed >= thisTimeout) {
            previousNSubscribers = Math.max(nPromises, nAccepted) + nNack;
            lastGathering = now;
        } // ...otherwise we use the previous value unchanged.

        // On startup, we do one proposal that we cannot possibly close, so that we'll
        // lock things up for the full electionTimeout to gather responses.
        if (!previousNSubscribers) {
            var LARGE_INTEGER = Number.MAX_SAFE_INTEGER || (-1 >>> 1); // QT doesn't define the ECMA constant. Max int will do for our purposes.
            previousNSubscribers = LARGE_INTEGER;
        }
        return previousNSubscribers;
    }

    // MAIN ALGORITHM
    //
    // Internally, this uses the Paxos algorith to hold elections.
    // Alternatively, we could have the message server pick and maintain a winner, but it would
    // still have to deal with the same issues of verification in the presence of lost/delayed/reordered messages.
    // Paxos is known to be optimal under these circumstances, except that its best to have a dedicated proposer
    // (such as the server).
    function betterNumber(number, best) {
        return (number.number || 0) > best.number;
    }
    // Paxos Proposer behavior
    var proposalNumber = 0,
        nQuorum = 0,
        bestPromise = {number: 0},
        claimCallback,
        releaseCallback;
    function propose() {  // Make a new proposal, so that we learn/update the proposalNumber and winner.
        // Even though we send back a 'nack' if the proposal is obsolete, with network errors
        // there's no way to know for certain that we've failed. The electionWatchdog will try a new
        // proposal if we have not been accepted by a quorum after election Timeout.
        if (electionWatchdog) {
            // If we had a means of determining nSubscribers other than by counting, we could just
            // timers.clearTimeout(electionWatchdog) and not return.
            return;
        }
        thisTimeout = randomize(electionTimeout, 0.5); // Note use in nSubcribers.
        electionWatchdog = timers.setTimeout(function () {
            electionWatchdog = null;
            propose();
        }, thisTimeout);
        var nAcceptors = nSubscribers();
        nQuorum = Math.floor(nAcceptors / 2) + 1;

        proposalNumber = Math.max(proposalNumber, bestPromise.number) + 1;
        debugFlow('baton:', batonName, instanceId, 'propose', proposalNumber,
                  'claim:', !!claimCallback, 'nAcceptors:', nAcceptors, nPromises, nAccepted, nNack);
        nPromises = nAccepted = nNack = 0;
        send('prepare!', {number: proposalNumber, proposerId: instanceId});
    }
    // We create a distinguished promise subchannel for our id, because promises need only be sent to the proposer.
    receive('promise' + instanceId, function (data) {
        if (betterNumber(data, bestPromise)) {
            bestPromise = data;
        }
        if ((data.proposalNumber === proposalNumber) && (++nPromises >= nQuorum)) { // Note check for not being a previous round
            var answer = {number: data.proposalNumber, proposerId: data.proposerId, winner: bestPromise.winner}; // Not data.number.
            if (!answer.winner || (answer.winner === instanceId)) { // We get to pick.
                answer.winner = claimCallback ? instanceId : null;
            }
            send('accept!', answer);
        }
    });
    receive('nack' + instanceId, function (data) { // An acceptor reports more recent data...
        if (data.proposalNumber === proposalNumber) {
            nNack++;  // For updating nQuorum.
            // IWBNI if we started our next proposal right now/here, but we need a decent nNack count.
            // Lets save that optimization for another day...
        }
    });
    // Paxos Acceptor behavior
    var bestProposal = {number: 0}, accepted = {};
    function acceptedId() { return accepted && accepted.winner; }
    receive('prepare!', function (data) {
        var response = {proposalNumber: data.number, proposerId: data.proposerId};
        if (betterNumber(data, bestProposal)) {
            bestProposal = data;
            if (accepted.winner && connectionTest(accepted.winner)) {
                response.number = accepted.number;
                response.winner = accepted.winner;
            }
            send1('promise', data.proposerId, response);
        } else {
            send1('nack', data.proposerId, response);
        }
    });
    receive('accept!', function (data) {
        if (!betterNumber(bestProposal, data)) {
            bestProposal = accepted = data; // Update both with current data. Might have missed the proposal earlier.
            if (useOptimizations) {
                // The Paxos literature describes every acceptor sending 'accepted' to
                // every proposer and learner. In our case, these are the same nodes that received
                // the 'accept!' message, so we can send to just the originating proposer and invoke
                // our own accepted handler directly.
                // Note that this optimization cannot be used with Byzantine Paxos (which needs another
                // multi-broadcast to detect lying and collusion).
                debugSend('/', 'accepted', data);
                debugReceive(instanceId, 'accepted', data); // direct on next line, which doesn't get logging.
                subchannelHandlers[subchannelKey('accepted') + instanceId].receiver(data);
                if (data.proposerId !== instanceId) { // i.e., we didn't already do it directly on the line above.
                    send1('accepted', data.proposerId, data);
                }
            } else {
                send('accepted', data);
            }
        } else {
            send1('nack', data.proposerId, {proposalNumber: data.number});
        }
    });
    // Paxos Learner behavior.
    function localRelease() {
        var callback = releaseCallback;
        debugFlow('baton:', batonName, 'localRelease', 'callback:', !!releaseCallback);
        if (!releaseCallback) { return; } // Already released, but we might still receive a stale message. That's ok.
        releaseCallback = undefined;
        callback(batonName);  // Pass batonName so that clients may use the same handler for different batons.
    }
    receive('accepted' + (useOptimizations ? instanceId : ''), function (data) { // See note in 'accept!' regarding use of instanceId here.
        if (betterNumber(accepted, data)) { // Especially when !useOptimizations, we can receive other acceptances late.
            return;
        }
        var oldAccepted = accepted;
        debugFlow('baton:', batonName, instanceId, 'accepted', data.number, data.winner);
        accepted = data;
        // If we are proposer, make sure we get a quorum of acceptances.
        if ((data.proposerId === instanceId) && (data.number === proposalNumber) && (++nAccepted >= nQuorum)) {
            if (electionWatchdog) {
                timers.clearTimeout(electionWatchdog);
                electionWatchdog = null;
            }
        }
        // If we are the winner -- regardless of whether we were the proposer.
        if (acceptedId() === instanceId) {
            if (claimCallback) {
                var callback = claimCallback;
                claimCallback = undefined;
                callback(batonName);
            } else if (!releaseCallback) { // We won, but have been released and are no longer interested.
                // Propose that someone else take the job.
                timers.setTimeout(propose, 0); // Asynchronous to queue message handling if some are synchronous and others not.
            }
        } else if (releaseCallback && (oldAccepted.winner === instanceId)) { // We've been released by someone else!
            localRelease(); // This can happen if enough people thought we'd disconnected.
        }
    });

    // Public Interface
    //
    // Registers an intent to hold the baton:
    // Calls onElection(batonName) once, if you are elected by the scripts
    //    to be the unique holder of the baton, which may be never.
    // Calls onRelease(batonName) once, if the baton held by you is released,
    //    whether this is by you calling release(), or by losing
    //    an election when you become disconnected.
    // You may claim again at any time after the start of onRelease
    // being called.
    exports.claim = function claim(onElection, onRelease) {
        debugFlow('baton:', batonName, instanceId, 'claim');
        if (claimCallback) {
            log("Ignoring attempt to claim virtualBaton " + batonName + ", which is already waiting for claim.");
            return;
        }
        if (releaseCallback) {
            log("Ignoring attempt to claim virtualBaton " + batonName + ", which is somehow incorrect released, and that should not happen.");
            return;
        }
        claimCallback = onElection;
        releaseCallback = onRelease;
        propose();
        return exports; // Allows chaining. e.g., var baton = virtualBaton({batonName: 'foo'}.claim(onClaim, onRelease);
    };
    // Release the baton you hold, or just log that you are not holding it.
    exports.release = function release(optionalReplacementOnRelease) {
        debugFlow('baton:', batonName, instanceId, 'release');
        if (optionalReplacementOnRelease) { // If you want to change.
            releaseCallback = optionalReplacementOnRelease;
        }
        if (acceptedId() !== instanceId) {
            log("Ignoring attempt to release virtualBaton " + batonName + ", which is not being held.");
            return;
        }
        localRelease();
        if (!claimCallback) { // No claim set in release callback.
            propose();
        }
        return exports;
    };
    exports.recheckWatchdog = timers.setInterval(function recheck() {
        var holder = acceptedId();  // If we're waiting and we notice the holder is gone, ...
        if (holder && claimCallback && !electionWatchdog && !connectionTest(holder)) {
            bestPromise.winner = null; // used if the quorum agrees that old winner is not there
            propose();  // ... propose an election.
        }
    }, recheckInterval);
    exports.unload = function unload() { // Disconnect from everything.
        messages.messageReceived.disconnect(messageHandler);
        timers.clearInterval(exports.recheckWatchdog);
        if (electionWatchdog) { timers.clearTimeout(electionWatchdog); }
        electionWatchdog = claimCallback = releaseCallback = null;
        Object.keys(subchannelHandlers).forEach(messages.unsubscribe);
        debugFlow('baton:', batonName, instanceId, 'unload');
        return exports;
    };

    // Gather nAcceptors by making two proposals with some gathering time, even without a claim.
    propose();
    return exports;
}
if (typeof module !== 'undefined') { // Allow testing in nodejs.
    module.exports = virtualBatonf;
} else {
    virtualBaton = virtualBatonf;
}
