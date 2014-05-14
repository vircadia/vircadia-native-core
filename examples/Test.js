//
//  Test.js
//  examples
//
//  Created by Ryan Huffman on 5/4/14
//  Copyright 2014 High Fidelity, Inc.
//
//  This provides very basic unit testing functionality.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

test = function(name, func) {
    print("Running test: " + name);

    var unitTest = new UnitTest(name, func);

    try {
        unitTest.run();
        print("  Success: " + unitTest.numAssertions + " assertions passed");
    } catch (error) {
        print("  Failure: " + error.message);
    }
};

AssertionException = function(expected, actual, message) {
    print("Creating exception");
    this.message = message + "\n: " + actual + " != " + expected;
    this.name = 'AssertionException';
};

UnitTest = function(name, func) {
    this.numAssertions = 0;
    this.func = func;
};

UnitTest.prototype.run = function() {
    this.func();
};

UnitTest.prototype.assertNotEquals = function(expected, actual, message) {
    this.numAssertions++;
    if (expected == actual) {
        throw new AssertionException(expected, actual, message);
    }
};

UnitTest.prototype.assertEquals = function(expected, actual, message) {
    this.numAssertions++;
    if (expected != actual) {
        throw new AssertionException(expected, actual, message);
    }
};

UnitTest.prototype.assertHasProperty = function(property, actual, message) {
    this.numAssertions++;
    if (actual[property] === undefined) {
        throw new AssertionException(property, actual, message);
    }
};

UnitTest.prototype.assertNull = function(value, message) {
    this.numAssertions++;
    if (value !== null) {
        throw new AssertionException(value, null, message);
    }
};
