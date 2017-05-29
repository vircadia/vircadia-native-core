/* eslint-env jasmine */

// Include testing library
Script.include('../../libraries/jasmine/jasmine.js');
Script.include('../../libraries/jasmine/hifi-boot.js');

// Include unit tests
Script.include('avatarUnitTests.js');
Script.include('bindUnitTest.js');
Script.include('entityUnitTests.js');

describe("jasmine internal tests", function() {
    it('should support async .done()', function(done) {
        var start = new Date;
        Script.setTimeout(function() {
            expect((new Date - start)/1000).toBeCloseTo(0.5, 1);
            done();
        }, 500);
    });
    // jasmine pending test
    xit('disabled test', function() {
        expect(false).toBe(true);
    });
});

// invoke Script.stop (after any async tests complete)
jasmine.getEnv().addReporter({ jasmineDone: Script.stop });

// Run the tests
jasmine.getEnv().execute();
