"use strict";
/*jslint nomen: true, plusplus: true, vars: true */
/*global Entities, Script, MyAvatar, Messages, AvatarList, print */
//
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
virtualBaton = function virtualBaton(options) {
    // Answer averages (number +/- variability). Avoids having everyone act in lockstep.
    function randomize(number, variability) {
        var randomPart = number * variability;
        return number - (randomPart / 2) + (Math.random() * randomPart);
    }
    var key = options.key,
        useOptimizations = (options.useOptimizations === undefined) ? true : options.useOptimizations,
        exports = options.exports || {},
        electionTimeout = options.electionTimeout || randomize(1000, 0.2), // ms. If no winner in this time, hold a new election.
        claimCallback,
        releaseCallback,
        ourId = MyAvatar.sessionUUID; // better be stable!
    if (!key) {
        throw new Error("A VirtualBaton must specify a key.");
    }
    function debug() {
        print.apply(null, [].map.call(arguments, JSON.stringify));
    }
    function debugFlow() {
        if (options.debugFlow) { debug.apply(null, arguments); }
    }

    // Messages: Just synactic sugar for hooking things up to Messages system.
    // We create separate subchannel strings for each operation within our general channelKey, instead of using
    // a switch in the receiver.
    var channelKey = "io.highfidelity.virtualBaton:" + key,
        subchannelHandlers = {}, // Message channel string => {function, op}
        subchannelKeys = {};     // operation => Message channel string
    function subchannelKey(operation) { return channelKey + ':' + operation; }
    function receive(operation, handler) { // Record a handler for an operation on our channelKey
        var subKey = subchannelKey(operation);
        subchannelHandlers[subKey] = {receiver: handler, op: operation};
        subchannelKeys[operation] = subKey;
        Messages.subscribe(subKey);
    }
    function sendHelper(subchannel, data) {
        var message = JSON.stringify(data);
        Messages.sendMessage(subchannel, message);
    }
    function send1(operation, destination, data) { // Send data for an operation to just one destination on our channelKey.
        if (options.debugSend) { debug('baton:', ourId, '=>', destination, operation, data); }
        sendHelper(subchannelKey(operation) + destination, data);
    }
    function send(operation, data) {  // Send data for an operation on our channelKey.
        if (options.debugSend) { debug('baton:', ourId, '=>', '-', operation, data); }
        sendHelper(subchannelKeys[operation], data);
    }
    Messages.messageReceived.connect(function (channel, messageString, senderID) {
        var handler = subchannelHandlers[channel];
        if (!handler) { return; }
        var data = JSON.parse(messageString);
        if (options.debugReceive) { debug('baton:', senderID, '=>', ourId, handler.op, data); }
        handler.receiver(data);
    });

    // Internally, this uses the Paxos algorith to hold elections.
    // Alternatively, we could have the message server pick and maintain a winner, but it would
    // still have to deal with the same issues of verification in the presence of lost/delayed/reordered messages.
    // Paxos is known to be optimal under these circumstances, except that its best to have a dedicated proposer
    // (such as the server).
    //
    // Paxos makes several tests of one "proposal number" versus another, assuming
    // that better proposals from the same proposer have a higher number,
    // and different proposers use a different set of numbers. We achieve that
    // by dividing the "number" into two parts, and integer and a proposerId,
    // which keeps the combined number unique and yet still strictly ordered.
    function betterNumber(number, best) {
        // FIXME restore debug('baton: betterNumber', number, best);
        //FIXME return ((number.number || 0) > best.number) && (!best.proposerId || (number.proposerId >= best.proposerId));
        return (number.number || 0) > best.number;
    }
    // Paxos Proposer behavior
    var nPromises = 0,
        proposalNumber = 0,
        nQuorum,
        mostRecentInterested,
        bestPromise = {number: 0},
        electionWatchdog,
        recheckInterval = options.recheckInterval || 1000, // ms. Check that winners remain connected.  FIXME rnadomize
        recheckWatchdog;
    function propose() {
        debugFlow('baton:', ourId, 'propose', !!claimCallback);
        if (electionWatchdog) { Script.clearTimeout(electionWatchdog); }
        if (!claimCallback) { return; } // We're not participating.
        electionWatchdog = Script.setTimeout(propose, electionTimeout);
        nPromises = 0;
        proposalNumber = Math.max(proposalNumber, bestPromise.number);
        nQuorum = Math.floor(AvatarList.getAvatarIdentifiers().length / 2) + 1;  // N.B.: ASSUMES EVERY USER IS RUNNING THE SCRIPT!
        send('prepare!', {number: ++proposalNumber, proposerId: ourId});
    }
    // We create a distinguished promise subchannel for our id, because promises need only be sent to the proposer.
    receive('promise' + ourId, function (data) {
        if (data.proposerId !== ourId) { return; } // Only the proposer needs to do anything.
        mostRecentInterested = mostRecentInterested || data.interested;
        if (betterNumber(data, bestPromise)) {
            bestPromise = data;
        }
        if (++nPromises >= nQuorum) {
            var answer = {number: data.proposalNumber, proposerId: data.proposerId, winner: data.winner};
            if (!answer.winner) { // we get to pick
                answer.winner = claimCallback ? ourId : mostRecentInterested;
            }
            send('accept!', answer);
        }
    });
    // Paxos Acceptor behavior
    var bestProposal = {number: 0}, accepted = {};
    function acceptedId() { return accepted && accepted.winner; }
    receive('prepare!', function (data) {
        if (useOptimizations) { // Don't waste time with low future proposals.
            proposalNumber = Math.max(proposalNumber, data.number);
        }
        if (betterNumber(data, bestProposal)) {
            bestProposal = data;
            if (claimCallback && useOptimizations) {
                // Let the proposer know we're interested in the job if the proposer doesn't
                // know who else to pick. Avoids needing to start multiple simultaneous proposals.
                accepted.interested = ourId;
            }
            send1('promise', data.proposerId,
                 accepted.winner ? // data must include proposerId and number so that proposer catalogs results.
                 {number: accepted.number, winner: accepted.winner, proposerId: data.proposerId, proposalNumber: data.number} :
                 {proposerId: data.proposerId, proposalNumber: data.number});
        } // FIXME nack?
    });
    receive('accept!', function (data) {
        if (!betterNumber(bestProposal, data)) {
            bestProposal = data;
            if (useOptimizations) {
                // The Paxos literature describe every acceptor sending 'accepted' to
                // every proposer and learner. In our case, these are the same nodes that received
                // the 'accept!' message, so we can send to just the originating proposer and invoke
                // our own accepted handler directly.
                // Note that this optimization cannot be used with Byzantine Paxos (which needs another
                // multi-broadcast to detect lying and collusion).
                subchannelHandlers[subchannelKey('accepted') + ourId].receiver(data);
                send1('accepted', data.proposerId, data);
            } else {
                send('accepted', data);
            }
        }
    });
    // Paxos Learner behavior.
    receive('accepted' + (useOptimizations ? ourId : ''), function (data) { // See note in 'accept!' regarding use of ourId here.
        accepted = data;
        if (acceptedId() === ourId) { // Note that we might not have been the proposer.
            if (electionWatchdog) {
                Script.clearTimeout(electionWatchdog);
                electionWatchdog = null;
            }
            if (claimCallback) {
                var callback = claimCallback;
                claimCallback = undefined;
                callback(key);
            } else if (!releaseCallback) { // We won, but have been released and are no longer interested.
                propose(); // Propose that someone else take the job.
            }
        }
    });

    receive('release', function (data) {
        if (!betterNumber(accepted, data)) { // Unless our data is fresher...
            debugFlow('baton:', ourId, 'handle release', data);
            accepted.winner = null; // ... allow next proposer to have his way by making this explicitly null (not undefined).
            if (recheckWatchdog) {
                Script.clearInterval(recheckWatchdog);
                recheckWatchdog = null;
            }
        }
    });
    function localRelease() {
        var callback = releaseCallback, oldAccepted = accepted;
        releaseCallback = undefined;
        accepted = {number: oldAccepted.number, proposerId: oldAccepted.proposerId}; // A copy without winner assigned, preserving number.
        debugFlow('baton: localRelease', key, !!callback);
        if (!callback) { return; } // Already released, but we might still receive a stale message. That's ok.
        callback(key);  // Pass key so that clients may use the same handler for different batons.
        return oldAccepted;
    }

    // Registers an intent to hold the baton:
    // Calls onElection(key) once, if you are elected by the scripts
    //    to be the unique holder of the baton, which may be never.
    // Calls onRelease(key) once, if you release the baton held by you,
    //    whether this is by you calling release(), or by loosing
    //    an election when you become disconnected.
    // You may claim again at any time after the start of onRelease
    // being called. Otherwise, you will not participate in further elections.
    exports.claim = function claim(onElection, onRelease) {
        debugFlow('baton:', ourId, 'claim');
        if (claimCallback) {
            print("Ignoring attempt to claim virtualBaton " + key + ", which is already waiting for claim.");
            return;
        }
        if (releaseCallback) {
            print("Ignoring attempt to claim virtualBaton " + key + ", which is somehow incorrect released, and that should not happen.");
            return;
        }
        claimCallback = onElection;
        releaseCallback = onRelease;
        propose();
    };

    // Release the baton you hold, or just log that you are not holding it.
    exports.release = function release(optionalReplacementOnRelease) {
        debugFlow('baton:', ourId, 'release');
        if (optionalReplacementOnRelease) { // E.g., maybe normal onRelease reclaims, but at shutdown you explicitly don't.
            releaseCallback = optionalReplacementOnRelease;
        }
        if (acceptedId() !== ourId) {
            print("Ignoring attempt to release virtualBaton " + key + ", which is not being held.");
            return;
        }
        var released = localRelease();
        if (released) {
            send('release', released); // Let everyone know right away, including old number in case we overlap with reclaim.
        }
        if (!claimCallback) { // No claim set in release callback.
            propose(); // We are the distinguished proposer, but we'll pick anyone else interested.
        }
    };

    return exports;
};
