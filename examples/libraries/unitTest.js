//
//  Unittest.js
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

test = function(name, func, timeout) {
    print("Running test: " + name);
    var unitTest = new UnitTest(name, func, timeout);
    unitTest.run(function(unitTest) {
        print("  Success: " + unitTest.numAssertions + " assertions passed");
    }, function(unitTest, error) {
        print("  Failure: " + error.name + " " + error.message);
    });
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

TimeoutException = function() {
    print("Creating exception");
    this.message = "UnitTest timed out\n";
    this.name = 'TimeoutException';
};

SequentialUnitTester = function() {
    this.tests = [];
    this.testIndex = -1;
};

SequentialUnitTester.prototype.addTest = function(name, func, timeout) {
    var _this = this;
    this.tests.push(function() {
        print("Running test: " + name);
        var unitTest = new UnitTest(name, func, timeout);
        unitTest.run(function(unitTest) {
            print("  Success: " + unitTest.numAssertions + " assertions passed");
            _this._nextTest();
        }, function(unitTest, error) {
            print("  Failure: " + error.name + " " + error.message);
            _this._nextTest();
        });
    });
};

SequentialUnitTester.prototype._nextTest = function() {
    this.testIndex++;
    if (this.testIndex < this.tests.length) {
        this.tests[this.testIndex]();
        return;
    }
    print("Completed all UnitTests");
};

SequentialUnitTester.prototype.run = function() {
    this._nextTest();
};

UnitTest = function(name, func, timeout) {
    this.numAssertions = 0;
    this.func = func;
    this.timeout = timeout;
};

UnitTest.prototype.run = function(successCallback, failureCallback) {
    var _this = this;
    this.successCallback = successCallback;
    this.failureCallback = failureCallback;
    if (this.timeout !== undefined) {
       this.timeoutTimer = Script.setTimeout(function() {
           _this.failureCallback(this, new TimeoutException());
       }, this.timeout);
    }
    try {
        this.func();
        if (this.timeout === undefined) {
            successCallback(this);
        }
    } catch (exception) {
        this.handleException(exception);
    }
};

UnitTest.prototype.registerCallbackFunction = function(func) {
    var _this = this;
    return function(one, two, three, four, five, six) {
        try {
            func(one, two, three, four, five, six);
        } catch (exception) {
            _this.handleException(exception);
        }
    };
};

UnitTest.prototype.handleException = function(exception) {
    if (this.timeout !== undefined) {
        Script.clearTimeout(this.timeoutTimer);
    }
    this.failureCallback(this, exception);
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
        throw new AssertionException(array1.length, array2.length , message);
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

UnitTest.prototype.done = function() {
    if (this.timeout !== undefined) {
        Script.clearTimeout(this.timeoutTimer);
        this.successCallback(this);
    }
}
