"use strict";
/*jslint nomen: true, plusplus: true, vars: true */
var assert = require('assert');
var mocha = require('mocha'), describe = mocha.describe, it = mocha.it, after = mocha.after;
var virtualBaton = require('../../../examples/libraries/virtualBaton.js');

describe('temp', function () {
    var messageCount = 0, testStart = Date.now();
    function makeMessager(nodes, me, mode) { // shim for High Fidelity Message system
        function noopSend(channel, string, source) {
        }
        function hasChannel(node, channel) {
            return -1 !== node.subscribed.indexOf(channel);
        }
        function sendSync(channel, message, nodes, skip) {
            nodes.forEach(function (node) {
                if (!hasChannel(node, channel) || (node === skip)) {
                    return;
                }
                node.sender(channel, message, me.name);
            });
        }
        nodes.forEach(function (node) {
            node.sender = node.sender || noopSend;
            node.subscribed = node.subscribed || [];
        });
        return {
            subscriberCount: function () {
                var c = 0;
                nodes.forEach(function (n) {
                    if (n.subscribed.length) {
                        c++;
                    }
                });
                return c;
            },
            subscribe: function (channel) {
                me.subscribed.push(channel);
            },
            unsubscribe: function (channel) {
                me.subscribed.splice(me.subscribed.indexOf(channel), 1);
            },
            sendMessage: function (channel, message) {
                if ((mode === 'immediate2Me') && hasChannel(me, channel)) {
                    me.sender(channel, message, me.name);
                }
                if (mode === 'immediate') {
                    sendSync(channel, message, nodes, null);
                } else {
                    process.nextTick(function () {
                        sendSync(channel, message, nodes, (mode === 'immediate2Me') ? me : null);
                    });
                }
            },
            messageReceived: {
                connect: function (f) {
                    me.sender = function (c, m, i) {
                        messageCount++; f(c, m, i);
                    };
                },
                disconnect: function () {
                    me.sender = noopSend;
                }
            }
        };
    }
    var debug = {}; //{flow: true, send: false, receive: false};
    function makeBaton(testKey, nodes, node, debug, mode, optimize) {
        debug = debug || {};
        var baton = virtualBaton({
            batonName: testKey,
            debugSend: debug.send,
            debugReceive: debug.receive,
            debugFlow: debug.flow,
            useOptimizations: optimize,
            connectionTest: function (id) {
                return baton.validId(id);
            },
            globals: {
                Messages: makeMessager(nodes, node, mode),
                MyAvatar: {sessionUUID: node.name},
                Script: {
                    setTimeout: setTimeout,
                    clearTimeout: clearTimeout,
                    setInterval: setInterval,
                    clearInterval: clearInterval
                },
                AvatarList: {
                    getAvatar: function (id) {
                        return {sessionUUID: id};
                    }
                },
                Entities: {getEntityProperties: function () {
                }},
                print: console.log
            }
        });
        return baton;
    }
    function noRelease(batonName) {
        assert.ok(!batonName, "should not release");
    }
    function defineABunch(mode, optimize) {
        function makeKey(prefix) {
            return prefix + mode + (optimize ? '-opt' : '');
        }
        var testKeys = makeKey('single-');
        it(testKeys, function (done) {
            var nodes = [{name: 'a'}];
            var a = makeBaton(testKeys, nodes, nodes[0], debug, mode).claim(function (key) {
                console.log('claimed a');
                assert.equal(testKeys, key);
                a.unload();
                done();
            }, noRelease);
        });
        var testKeydp = makeKey('dual-parallel-');
        it(testKeydp, function (done) {
            this.timeout(10000);
            var nodes = [{name: 'ap'}, {name: 'bp'}];
            var a = makeBaton(testKeydp, nodes, nodes[0], debug, mode, optimize),
                b = makeBaton(testKeydp, nodes, nodes[1], debug, mode, optimize);
            function accepted(key) { // Under some circumstances of network timing, either a or b can win.
                console.log('claimed ap');
                assert.equal(testKeydp, key);
                done();
            }
            a.claim(accepted, noRelease);
            b.claim(accepted, noRelease);
        });
        var testKeyds = makeKey('dual-serial-');
        it(testKeyds, function (done) {
            var nodes = [{name: 'as'}, {name: 'bs'}],
                gotA = false,
                gotB = false;
            makeBaton(testKeyds, nodes, nodes[0], debug, mode, optimize).claim(function (key) {
                console.log('claimed as', key);
                assert.ok(!gotA, "should not get A after B");
                gotA = true;
                done();
            }, noRelease);
            setTimeout(function () {
                makeBaton(testKeyds, nodes, nodes[1], debug, mode, optimize).claim(function (key) {
                    console.log('claimed bs', key);
                    assert.ok(!gotB, "should not get B after A");
                    gotB = true;
                    done();
                }, noRelease);
            }, 500);
        });
        var testKeydsl = makeKey('dual-serial-long-');
        it(testKeydsl, function (done) {
            this.timeout(5000);
            var nodes = [{name: 'al'}, {name: 'bl'}],
                gotA = false,
                gotB = false,
                releaseA = false;
            makeBaton(testKeydsl, nodes, nodes[0], debug, mode, optimize).claim(function (key) {
                console.log('claimed al', key);
                assert.ok(!gotB, "should not get A after B");
                gotA = true;
                if (!gotB) {
                    done();
                }
            }, function () {
                assert.ok(gotA, "Should claim it first");
                releaseA = true;
                if (gotB) {
                    done();
                }
            });
            setTimeout(function () {
                makeBaton(testKeydsl, nodes, nodes[1], debug, mode, optimize).claim(function (key) {
                    console.log('claimed bl', key);
                    gotB = true;
                    if (releaseA) {
                        done();
                    }
                }, noRelease);
            }, 3000);
        });
        var testKeydsr = makeKey('dual-serial-with-release-');
        it(testKeydsr, function (done) {
            this.timeout(5000);
            var nodes = [{name: 'asr'}, {name: 'bsr'}],
                gotClaimA = false,
                gotReleaseA = false,
                a = makeBaton(testKeydsr, nodes, nodes[0], debug, mode, optimize),
                b = makeBaton(testKeydsr, nodes, nodes[1], debug, mode, optimize);
            a.claim(function (key) {
                console.log('claimed asr');
                assert.equal(testKeydsr, key);
                gotClaimA = true;
                b.claim(function (key) {
                    console.log('claimed bsr');
                    assert.equal(testKeydsr, key);
                    assert.ok(gotReleaseA);
                    done();
                }, noRelease);
                a.release();
            }, function (key) {
                console.log('released asr');
                assert.equal(testKeydsr, key);
                assert.ok(gotClaimA);
                gotReleaseA = true;
            });
        });
        var testKeydpr = makeKey('dual-parallel-with-release-');
        it(testKeydpr, function (done) {
            this.timeout(5000);
            var nodes = [{name: 'ar'}, {name: 'br'}];
            var a = makeBaton(testKeydpr, nodes, nodes[0], debug, mode, optimize),
                b = makeBaton(testKeydpr, nodes, nodes[1], debug, mode, optimize),
                gotClaimA = false,
                gotReleaseA = false,
                gotClaimB = false;
            a.claim(function (key) {
                console.log('claimed ar');
                assert.equal(testKeydpr, key);
                gotClaimA = true;
                assert.ok(!gotClaimB, "if b claimed, should not get a");
                a.release();
            }, function (key) {
                console.log('released ar');
                assert.equal(testKeydpr, key);
                assert.ok(gotClaimA);
                gotReleaseA = true;
            });
            b.claim(function (key) {
                console.log('claimed br', gotClaimA ? 'with' : 'without', 'ar first');
                assert.equal(testKeydpr, key);
                gotClaimB = true;
                assert.ok(!gotClaimA || gotReleaseA);
                done();
            }, noRelease);
        });
    }
    function defineAllModeTests(optimize) {
        defineABunch('delayed', optimize);
        defineABunch('immediate2Me', optimize);
        defineABunch('immediate', optimize);
    }
    defineAllModeTests(true);
    defineAllModeTests(false);
    after(function () {
        console.log(messageCount, 'messages sent over', (Date.now() - testStart), 'ms.');
    });
});
