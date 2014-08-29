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
        print("  Failure: " + error.name + " " + error.message);
    }
};

AssertionException = function(expected, actual, message) {
    print("Creating exception");
    this.message = message + "\n: " + actual + " != " + expected;
    this.name = 'AssertionException';
};

UnthrownException = function(message) {
    print("Creating exception");
    this.message = message + "\n";
    this.name = 'UnthrownException';
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

UnitTest.prototype.assertContains = function (expected, actual, message) {
    this.numAssertions++;
    if (actual.indexOf(expected) == -1) {
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
}

UnitTest.prototype.arrayEqual = function(array1, array2, message) {
    this.numAssertions++;
    if (array1.length !== array2.length) {
        throw new AssertionException(array1.length , array2.length , message);
    }
    for (var i = 0; i < array1.length; ++i) {
        if (array1[i] !== array2[i]) {
            throw new AssertionException(array1[i], array2[i], i + " " + message);
        }
    }
}

UnitTest.prototype.raises = function(func, message) {
    this.numAssertions++;
    try {
        func();
    } catch (error) {
        return;
    }
    
    throw new UnthrownException(message);
}