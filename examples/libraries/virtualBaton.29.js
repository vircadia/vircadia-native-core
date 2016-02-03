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
    var key = options.key,
        channel = "io.highfidelity.virtualBaton:" + key,
        exports = options.exports || {},
        claimCallback,
        releaseCallback,
        // paxos proposer state
        nPromises = 0,
        proposalNumber = 0,
        nQuorum,
        mostRecentInterested,
        bestPromise = {number: 0},
        electionTimeout = options.electionTimeout || 1000, // ms. If no winner in this time, hold a new election.  FIXME randomize
        electionWatchdog,
        recheckInterval = options.recheckInterval || 1000, // ms. Check that winners remain connected.  FIXME rnadomize
        recheckWatchdog,
        // paxos acceptor state
        bestProposal = {number: 0},
        accepted = {};
    if (!key) {
        throw new Error("A VirtualBaton must specify a key.");
    }
    function debug() {
        print.apply(print, [].map.call(arguments, JSON.stringify)); // fixme no console
    }
    function debugFlow() {
        if (options.debugFlow) { debug.apply(null, arguments); }
    }
    function send(operation, data) {
        if (options.debugSend) { debug('baton:', MyAvatar.sessionUUID, '=>', '-', operation, data); }
        var message = JSON.stringify({op: operation, data: data});
        Messages.sendMessage(channel, message);
    }
    function localRelease() {
        var callback = releaseCallback, oldAccepted = accepted;
        releaseCallback = undefined;
        accepted = {number: oldAccepted.number, proposerId: oldAccepted.proposerId}; // A copy without winner assigned, preserving number.
        debugFlow('baton: localRelease', key, !!callback);
        if (!callback) { return; } // Already released, but we might still receive a stale message. That's ok.
        //Messages.messageReceived.disconnect(messageHandler);
        //Messages.unsubscribe(channel);   // Messages currently allow publishing without subscription.
        callback(key);  // Pass key so that clients may use the same handler for different batons.
        return oldAccepted;
    }

    // Internally, this uses the Paxos algorith to hold elections.
    // Alternatively, we could have the message server pick and maintain a winner, but it would
    // still have to deal with the same issues of verification in the presence of lost/delayed/reordered messages.
    // Paxos is known to be optimal under these circumstances, except that its best to have a dedicated proposer
    // (such as the server).
    function acceptedId() { return accepted && accepted.winner; }
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
    function propose() {
        debugFlow('baton:', MyAvatar.sessionUUID, 'propose', !!claimCallback);
        if (electionWatchdog) { Script.clearTimeout(electionWatchdog); }
        if (!claimCallback) { return; } // We're not participating.
        nPromises = 0;
        proposalNumber = Math.max(proposalNumber, bestPromise.number);
        nQuorum = Math.floor(AvatarList.getAvatarIdentifiers().length / 2) + 1;  // N.B.: ASSUMES EVERY USER IS RUNNING THE SCRIPT!
        send('prepare!', {number: ++proposalNumber, proposerId: MyAvatar.sessionUUID});
        electionWatchdog = Script.setTimeout(propose, electionTimeout);
    }
    function messageHandler(messageChannel, messageString, senderID) {
        if (messageChannel !== channel) { return; }
        var message = JSON.parse(messageString), data = message.data;
        if (options.debugReceive) { debug('baton:', senderID, '=>', MyAvatar.sessionUUID, message.op, data); }
        switch (message.op) {
        case 'prepare!':
            // Optimization: Don't waste time with low future proposals.
            // Does not remove the need for betterNumber() to consider proposerId, because
            // participants might not receive this prepare! message before their next proposal.
            proposalNumber = Math.max(proposalNumber, data.number);

            if (betterNumber(data, bestProposal)) {
                bestProposal = data;
                if (claimCallback) {
                    // Optimization: Let the proposer know we're interested in the job if the proposer doesn't
                    // know who else to pick. Avoids needing to start multiple simultaneous proposals.
                    accepted.interested = MyAvatar.sessionUUID;
                }
                send('promise', accepted.winner ? // data must include proposerId so that proposer catalogs results.
                        {number: accepted.number, proposerId: data.proposerId, winner: accepted.winner} :
                        {number: data.number, proposerId: data.proposerId});
            } // FIXME nack?
            break;
        case 'promise':
            if (data.proposerId !== MyAvatar.sessionUUID) { return; } // Only the proposer needs to do anything.
            mostRecentInterested = mostRecentInterested || data.interested;
            if (betterNumber(data, bestPromise)) {
                bestPromise = data;
            }
            if (++nPromises >= nQuorum) {
                if (!bestPromise.winner) { // we get to pick
                    bestPromise.winner = claimCallback ? MyAvatar.sessionUUID : mostRecentInterested;
                }
                send('accept!', bestPromise);
            }
            break;
        case 'accept!':
            if (!betterNumber(bestProposal, data)) {
                accepted = data;
                //send('accepted', accepted); // With the collapsed roles here, do we need this message? Maybe just go to 'accepted' case here?
                messageHandler(messageChannel, JSON.stringify({op: 'accepted', data: accepted}), senderID);
            }
            break;
        case 'accepted':
            if (betterNumber(accepted, data)) { return; }
            accepted = data;
            if (acceptedId() === MyAvatar.sessionUUID) { // Note that we might not have been the proposer.
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
            break;
        case 'release':
            if (!betterNumber(accepted, data)) { // Unless our data is fresher...
                accepted.winner = undefined; // ... allow next proposer to have his way.
                if (recheckWatchdog) {
                    Script.clearInterval(recheckWatchdog);
                    recheckWatchdog = null;
                }
            }
            break;
        default:
            print("Unrecognized virtualBaton message:", message);
        }
    }
    Messages.messageReceived.connect(messageHandler); // FIXME MUST BE DONE. quorum will be wrong if no one claims and we only subscrbe with claims
    Messages.subscribe(channel); // FIXME

    // Registers an intent to hold the baton:
    // Calls onElection(key) once, if you are elected by the scripts
    //    to be the unique holder of the baton, which may be never.
    // Calls onRelease(key) once, if you release the baton held by you,
    //    whether this is by you calling release(), or by loosing
    //    an election when you become disconnected.
    // You may claim again at any time after the start of onRelease
    // being called. Otherwise, you will not participate in further elections.
    exports.claim = function claim(onElection, onRelease) {
        debugFlow('baton:', MyAvatar.sessionUUID, 'claim');
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
        //Messages.messageReceived.connect(messageHandler);
        //Messages.subscribe(channel);
        propose();
    };

    // Release the baton you hold, or just log that you are not holding it.
    exports.release = function release(optionalReplacementOnRelease) {
        debugFlow('baton:', MyAvatar.sessionUUID, 'release');
        if (optionalReplacementOnRelease) { // E.g., maybe normal onRelease reclaims, but at shutdown you explicitly don't.
            releaseCallback = optionalReplacementOnRelease;
        }
        if (acceptedId() !== MyAvatar.sessionUUID) {
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
