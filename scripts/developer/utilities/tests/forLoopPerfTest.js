//
//  Created by Seiji Emery on 9/4/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Checks various types of js iteration (for loops, for/in loops, Array.forEach, and Object.keys)
//  for speed + efficiency. Mostly a sanity check for my own code.
//

Script.include("perfTest.js");

(function () {
    var input1 = [];
    var input2 = [];
    var N = 1000;
    for (var i = 0; i < N; ++i) {
        input1.push(Math.random());
        input2.push(Math.random());
    }
    TestCase.prototype.withArray = function(n, func) {
        this.before(function () {
            this.array = [];
            for (var i = 0; i < n; ++i) {
                this.array.push(0);
            }
        });
        this.run(fcn);
    }

    print("For loop test -- prelim tests");
    var prelimTests = new TestRunner();
    prelimTests.addTestCase('Array.push (test component)')
        .before(function () {
            this.array = [];
        })
        .run(function () {
            this.array.push(1);
        });
    prelimTests.addTestCase('Array.push with mul op (test component)')
        .before(function() {
            this.array = [];
        })
        .run(function (i) {
            this.array.push(input1[i] * Math.PI);
        })
    prelimTests.runAllTestsWithIterations([1e4, 1e5, 1e6], 1e6, 10);

    print("For loop test (n = " + N + ")");
    var loopTests = new TestRunner();
    loopTests.addTestCase('for (var i = 0; i < n; ++i) { ... }')
        .before(function () {
            this.array = [];
        })
        .run(function () {
            for (var i = 0; i < N; ++i) {
                this.array.push(input1[i] * Math.PI);
            }
        })
    loopTests.addTestCase('while (n --> 0) { ... } (reversed)')
        .before(function () {
            this.array = [];
        })
        .run(function () {
            var n = N;
            while (n --> 0) {
                this.array.push(input1[n] * Math.PI);
            }
        })
    loopTests.addTestCase('Array.forEach(function(v, i) { ... })')
        .before(function () {
            this.array = [];
        })
        .run(function () {
            input1.forEach(function(v) {
                this.array.push(v * Math.PI);
            }, this);
        })
    loopTests.addTestCase('Array.map(function(v, i) { ... })')
        .run(function () {
            this.array = input1.map(function (v) {
                return v * Math.PI;
            }, this);
        });
    loopTests.runAllTestsWithIterations([10, 100, 1000], 1e3, 10);


    // Test iteration on object keys

    // http://stackoverflow.com/questions/10726909/random-alpha-numeric-string-in-javascript
    function makeRandomString(length, chars) {
        var result = '';
        for (var i = length; i > 0; --i) result += chars[Math.round(Math.random() * (chars.length - 1))];
        return result;
    }
    function randomString () {
        var n = Math.floor(Math.random() * 18) + 12;
        return makeRandomString(n, '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ');
    }

    var obj = {}, numKeys = 1000;
    while (numKeys --> 0) {
        obj[randomString()] = Math.floor(Math.random() * 255);
    }
    print("Object iter tests (object has " + Object.keys(obj).length + " keys)");
    var iterTests = new TestRunner();
    iterTests.addTestCase('for (var k in obj) { foo(obj[k]); }')
        .before(function () {
            this.x = 0;
        })
        .run(function () {
            for (var k in obj) {
                this.x = (this.x + obj[k]) % 256;
            }
        });
    iterTests.addTestCase('for (var k in obj) { if (Object.hasOwnProperty(obj, k)) { foo(obj[k]); } }')
        .before(function () {
            this.x = 0;
        })
        .run(function () {
            for (var k in obj) {
                if (Object.hasOwnProperty(obj, k)) {
                    this.x = (this.x + obj[k]) % 256;
                }
            }
        })
    iterTests.addTestCase('Object.keys(obj).forEach(function(k) { foo(obj[k]); }')
        .before(function () {
            this.x = 0;
        })
        .run(function () {
            Object.keys(obj).forEach(function (k) {
                this.x = (this.x + obj[k]) % 256;
            })
        })

    iterTests.runAllTestsWithIterations([10, 100, 1000], 1e3, 10);
})();

