//
//  Created by Seiji Emery on 9/4/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Simple benchmarking library. See the test scripts for example usage.
//

(function () {
    // Runs multiple 'test cases' (benchmarks that get invoked w/ varying iteration counts)
    function TestRunner () {
        this._testCases = {};
        this._testResults = [];
    }
    this.TestRunner = TestRunner;

    // Add a test case. Define behavior by declaratively calling .before, .after, and .run w/ impls.
    TestRunner.prototype.addTestCase = function (name) {
        var testcase = new TestCase(name);
        this._testCases[name] = testcase;
        return testcase;
    }

    // Runs a function n times. Uses context object so it runs sandboxed.
    function runTimedWithIterations (f, context, numIterations) {
        var start = new Date().getTime();
        while (numIterations --> 0) {
            f.apply(context);
        }
        var end = new Date().getTime();
        return end - start;
    }

    function fmtSecs (secs) {
        if (secs >= 1.0) { 
            return ""+secs.toFixed(3)+" secs";
        } else if (secs >= 1e-3) {
            return ""+(secs * 1e3).toFixed(3)+" ms";
        } else if (secs >= 1e-6) {
            return ""+(secs * 1e6).toFixed(3)+" µs";
        } else {
            return ""+(secs * 1e9).toFixed(3)+" ns";
        }
    }
    function avg(samples) {
        return samples.length ? 
            samples.reduce(function(a, b){return a+b;}, 0.0) / samples.length : 0.0;
    }

    // Runs a named performance test for multiple iterations, and optionally calculates an average.
    // @param name: the test name registered with addTest
    // @param iterations: a list of iteration sequences. eg. [10, 100, 1000] => runs for 10, then 100, then 1000 iterations
    //   and prints the timing info for each (in ms)
    // @param iterationsForAvg: optional
    // @param samplesForAvg: optional
    // After running iterations, will compute an average if iterationsForAvg and samplesForAvg are both set:
    //  average := 
    //      collects n samples by running the testcase n times with i iterations
    //          where n = samplesForAvg, i = iterationsForAvg
    //      we then average the samples, and print that.
    //
    TestRunner.prototype.runPerfTestWithIterations = function (name, iterations, iterationsForAvg, samplesForAvg) {
        if (!this._testCases[name]) {
            print("No test case with name '" + name + "'!");
        }
        var testcase = this._testCases[name];

        var runAverages = [];   // secs

        var noErrors = true;
        iterations.forEach(function(n, i) {
            var sandbox = {};
            try {
                if (testcase.setupFunction) {
                    testcase.setupFunction.apply(sandbox);
                }

                var dt = runTimedWithIterations(testcase.runFunction, sandbox, n);

                if (testcase.teardownFunction) {
                    testcase.teardownFunction.apply(sandbox);
                }

                this._testResults.push(""+name+" with "+n+" iterations: "+dt+" ms");
            } catch (e) {
                this._testResults.push("Testcase '"+name+"':"+i+" ("+n+") failed with error: "+e);
                noErrors = false;
            }
        }, this);
        this.writeResultsToLog();
        if (noErrors && iterationsForAvg && samplesForAvg) {
            try {
                var samples = [];
                for (var n = samplesForAvg; n --> 0; ) {
                    var sandbox = {};
                    if (testcase.setupFunction) {
                        testcase.setupFunction.apply(sandbox);
                    }
    
                    var dt = runTimedWithIterations(testcase.runFunction, sandbox, iterationsForAvg);
    
                    if (testcase.teardownFunction) {
                        testcase.teardownFunction.apply(sandbox);
                    }
                    samples.push(dt / iterationsForAvg * 1e-3); // dt in ms
                }
                print("    average: " + ((avg(samples) * 1e6).toFixed(3)) + " µs")
                // print("\t(" + samplesForAvg + " samples with " + iterationsForAvg + " iterations)");
            } catch (e) {
                print("Error while calculating average: " + e);
                return;
            }
        }
    }
    // Runs all registered tests w/ iteration + average parameters
    TestRunner.prototype.runAllTestsWithIterations = function (iterations, iterationsForAvg, samplesForAvg) {
        Object.keys(this._testCases).forEach(function(name) {
            this.runPerfTestWithIterations(name, iterations, iterationsForAvg, samplesForAvg);
        }, this);
    }

    // Dump results to the debug log. You don't need to call this.
    TestRunner.prototype.writeResultsToLog = function () {
        var s = '    ' + this._testResults.join('\n    ');
        this._testResults = [];
        // print(s);
        s.split('\n').forEach(function(line) {
            print(line);
        });
    }

    // Implements a benchmark test case, that has optional setup = teardown code, and a (non-optional) run function.
    // setup + teardown get called once, and run gets called for n sequential iterations (see TestRunner.runTestWithIterations)
    // setup, run, and teardown all get applied to the same sandboxed this object, so use that for storing test data for each run.
    function TestCase (name) {
        this.name = name;
        this.setupFunction = null;
        this.runFunction = null;
        this.teardownFunction = null;
    }
    this.TestCase = TestCase;
    TestCase.prototype.before = function (f) {
        this.setupFunction = f;
        return this;
    }
    TestCase.prototype.after = function (f) {
        this.teardownFunction = f;
        return this;
    }
    TestCase.prototype.run = function (f) {
        this.runFunction = f;
    }
})();