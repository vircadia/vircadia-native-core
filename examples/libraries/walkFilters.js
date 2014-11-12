//
//  walkFilters.js
//
//  version 1.001
//
//  Created by David Wooldridge, Autumn 2014
//
//  Provides a variety of filters for use by the walk.js script v1.1
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

AveragingFilter = function(length) {

    //this.name = name;
    this.pastValues = [];

    for(var i = 0; i < length; i++) {
        this.pastValues.push(0);
    }

    // single arg is the nextInputValue
    this.process = function() {

        if (this.pastValues.length === 0 && arguments[0]) {
            return arguments[0];
        } else if (arguments[0]) {
            // apply quick and simple LP filtering
            this.pastValues.push(arguments[0]);
            this.pastValues.shift();
            var nextOutputValue = 0;
            for (var ea in this.pastValues) nextOutputValue += this.pastValues[ea];
            return nextOutputValue / this.pastValues.length;
        } else {
            return 0;
        }
    };
};

// 2nd order Butterworth LP filter - calculate coeffs here: http://www-users.cs.york.ac.uk/~fisher/mkfilter/trad.html
// provides LP filtering with a more stable frequency / phase response
ButterworthFilter = function(cutOff) {

    // cut off frequency = 5Hz
    this.gain = 20.20612010;
    this.coeffOne = -0.4775922501;
    this.coeffTwo = 1.2796324250;

    // initialise the arrays
    this.xv = [];
    this.yv = [];
    for(var i = 0; i < 3; i++) {
        this.xv.push(0);
        this.yv.push(0);
    }

    // process values
    this.process = function(nextInputValue) {

        this.xv[0] = this.xv[1];
        this.xv[1] = this.xv[2];
        this.xv[2] = nextInputValue / this.gain;

        this.yv[0] = this.yv[1];
        this.yv[1] = this.yv[2];
        this.yv[2] = (this.xv[0] + this.xv[2]) +
                      2 * this.xv[1] +
                     (this.coeffOne * this.yv[0]) +
                     (this.coeffTwo * this.yv[1]);

        return this.yv[2];
    };
}; // end Butterworth filter contructor

// Add harmonics to a given sine wave to form square, sawtooth or triangle waves
// Geometric wave synthesis fundamentals taken from: http://hyperphysics.phy-astr.gsu.edu/hbase/audio/geowv.html
WaveSynth = function(waveShape, numHarmonics, smoothing) {

    this.numHarmonics = numHarmonics;
    this.waveShape = waveShape;
    this.averagingFilter = new AveragingFilter(smoothing);

    // NB: frequency in radians
    this.shapeWave = function(frequency) {

        // make some shapes
        var harmonics = 0;
        var multiplier = 0;
        var iterations = this.numHarmonics * 2 + 2;
        if (this.waveShape === TRIANGLE) {
            iterations++;
        }

        for(var n = 2; n < iterations; n++) {

            switch(this.waveShape) {

                case SAWTOOTH: {

                    multiplier = 1 / n;
                    harmonics += multiplier * Math.sin(n * frequency);
                    break;
                }

                case TRIANGLE: {

                    if (n % 2 === 1) {
                        var mulitplier = 1 / (n * n);
                        // multiply (4n-1)th harmonics by -1
                        if (n === 3 || n === 7 || n === 11 || n === 15) {
                            mulitplier *= -1;
                        }
                        harmonics += mulitplier * Math.sin(n * frequency);
                    }
                    break;
                }

                case SQUARE: {

                    if (n % 2 === 1) {
                        multiplier = 1 / n;
                        harmonics += multiplier * Math.sin(n * frequency);
                    }
                    break;
                }
            }
        }

        // smooth the result and return
        return this.averagingFilter.process(harmonics);
    };
};

// Create a wave shape by summing pre-calcualted sinusoidal harmonics
HarmonicsFilter = function(magnitudes, phaseAngles) {

    this.magnitudes = magnitudes;
    this.phaseAngles = phaseAngles;

    this.calculate = function(twoPiFT) {

        var harmonics = 0;
        var numHarmonics = magnitudes.length;

        for(var n = 0; n < numHarmonics; n++) {
            harmonics += this.magnitudes[n] * Math.cos(n * twoPiFT - this.phaseAngles[n]);
        }
        return harmonics;
    };
};

// the main filter object literal
filter = (function() {

    // Bezier private functions
    function _B1(t) { return t * t * t };
    function _B2(t) { return 3 * t * t * (1 - t) };
    function _B3(t) { return 3 * t * (1 - t) * (1 - t) };
    function _B4(t) { return (1 - t) * (1 - t) * (1 - t) };

    return {

        // helper methods
        degToRad: function(degrees) {

            var convertedValue = degrees * Math.PI / 180;
            return convertedValue;
        },

        radToDeg: function(radians) {

            var convertedValue = radians * 180 / Math.PI;
            return convertedValue;
        },

        // these filters need instantiating, as they hold arrays of previous values
        createAveragingFilter: function(length) {

            var newAveragingFilter = new AveragingFilter(length);
            return newAveragingFilter;
        },

        createButterworthFilter: function(cutoff) {

            var newButterworthFilter = new ButterworthFilter(cutoff);
            return newButterworthFilter;
        },

        createWaveSynth: function(waveShape, numHarmonics, smoothing) {

            var newWaveSynth = new WaveSynth(waveShape, numHarmonics, smoothing);
            return newWaveSynth;
        },

        createHarmonicsFilter: function(magnitudes, phaseAngles) {

            var newHarmonicsFilter = new HarmonicsFilter(magnitudes, phaseAngles);
            return newHarmonicsFilter;
        },


        // the following filters do not need separate instances, as they hold no previous values
        bezier: function(percent, C1, C2, C3, C4) {

            // Bezier functions for more natural transitions
            // based on script by Dan Pupius (www.pupius.net) http://13thparallel.com/archive/bezier-curves/
            var pos = {x: 0, y: 0};
            pos.x = C1.x * _B1(percent) + C2.x * _B2(percent) + C3.x * _B3(percent) + C4.x * _B4(percent);
            pos.y = C1.y * _B1(percent) + C2.y * _B2(percent) + C3.y * _B3(percent) + C4.y * _B4(percent);
            return pos;
        },

        // simple clipping filter (clips bottom of wave only, special case for hips y-axis skeleton offset)
        clipTrough: function(inputValue, peak, strength) {

            var outputValue = inputValue * strength;
            if (outputValue < -peak) {
                outputValue = -peak;
            }
            return outputValue;
        }
    }

})();