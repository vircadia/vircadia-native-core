/* eslint-env jasmine */

instrument_testrunner(true);

describe("Assets", function () {
    var context = {
        definedHash: null,
        definedPath: null,
        definedContent: null,
    };
    var NULL_HASH = new Array(64+1).join('0'); // 64 hex
    var SAMPLE_FOLDER = '/assetUnitTests';
    var SAMPLE_FILE_PATH = SAMPLE_FOLDER + "/test.json";
    var SAMPLE_CONTENTS = 'Test Run on ' + JSON.stringify(Date());
    var IS_ASSET_HASH_REGEX = /^[a-f0-9]{64}$/i;
    var IS_ASSET_URL = /^atp:/;
    
    it('Entities.canWriteAssets', function() {
        expect(Entities.canWriteAssets()).toBe(true);
    });

    it('Assets.extractAssetHash(input)', function() {
        // new helper method that provides a catch-all way to get at the sha256 hash
        // considering the multiple, different ways ATP hash URLs are found across the
        // system and in content.
        
        var POSITIVE_TEST_URLS = [
            'atp:HASH',
            'atp:HASH.obj',
            'atp:HASH.fbx?cache-buster',
            'atp:/.baked/HASH/asset.fbx',
            'HASH'
        ];
        var NEGATIVE_TEST_URLS = [
            'asdf',
            'http://domain.tld',
            '/path/filename.fbx',
            'atp:/path/filename.fbx?#HASH',
            ''
        ];
        pending();
    });

    // new APIs

    it('Assets.getAsset(options, {callback(error, result)})', function() { pending(); });
    it('Assets.putAsset(options, {callback(error, result)})', function() { pending(); });
    it('Assets.deleteAsset(options, {callback(error, result)})', function() { pending(); });
    it('Assets.resolveAsset(options, {callback(error, result)})', function() { pending(); });


    // existing APIs
    it('Assets.getATPUrl(input)', function() { pending(); });
    it('Assets.isValidPath(input)', function() { pending(); });
    it('Assets.isValidFilePath(input)', function() { pending(); });
    it('Assets.isValidHash(input)', function() { pending(); });
    it('Assets.setBakingEnabled(path, enabled, {callback(error)})', function() { pending(); });

    it("Assets.uploadData(data, {callback(url, hash)})", function (done) {
        Assets.uploadData(SAMPLE_CONTENTS, function(url, hash) {
            expect(url).toMatch(IS_ASSET_URL);
            expect(hash).toMatch(IS_ASSET_HASH_REGEX);
            context.definedHash = hash; // used in later tests
            context.definedContent = SAMPLE_CONTENTS;
            print('context.definedHash = ' + context.definedHash);
            print('context.definedContent = ' + context.definedContent);
            done();
        });
    });

    it("Assets.setMapping(path, hash {callback(error)})", function (done) {
        expect(context.definedHash).toMatch(IS_ASSET_HASH_REGEX);
        Assets.setMapping(SAMPLE_FILE_PATH, context.definedHash, function(error) {
            if (error) error += ' ('+JSON.stringify([SAMPLE_FILE_PATH, context.definedHash])+')';
            expect(error).toBe(null);
            context.definedPath = SAMPLE_FILE_PATH;
            print('context.definedPath = ' + context.definedPath);
            done();
        });
    });

    it("Assets.getMapping(path, {callback(error, hash)})", function (done) {
        expect(context.definedHash).toMatch(IS_ASSET_HASH_REGEX, 'asfasdf');
        expect(context.definedPath).toMatch(/^\//, 'asfasdf');
        Assets.getMapping(context.definedPath, function(error, hash) {
            if (error) error += ' ('+JSON.stringify([context.definedPath])+')';
            expect(error).toBe(null);
            expect(hash).toMatch(IS_ASSET_HASH_REGEX);
            expect(hash).toEqual(context.definedHash);
            done();
        });
    });

    it('Assets.getMapping(nullHash, {callback(data)})', function(done) {
        // FIXME: characterization test -- current behavior is that downloadData silently fails
        // when given an asset that doesn't exist
        Assets.downloadData(NULL_HASH, function(result) {
            throw new Error("this callback 'should' not be triggered");
        });
        setTimeout(function() { done(); }, 2000);
    });

    it('Assets.downloadData(hash, {callback(data)})', function(done) {
        expect(context.definedHash).toMatch(IS_ASSET_HASH_REGEX, 'asfasdf');
        Assets.downloadData('atp:' + context.definedHash, function(result) {
            expect(result).toEqual(context.definedContent);
            done();
        });
    });
    
    it('Assets.downloadData(nullHash, {callback(data)})', function(done) {
        // FIXME: characterization test -- current behavior is that downloadData silently fails
        // when given an asset doesn't exist
        Assets.downloadData(NULL_HASH, function(result) {
            throw new Error("this callback 'should' not be triggered");
        });
        setTimeout(function() {
            done();
        }, 2000);
    });
    
    describe('exceptions', function() {
        describe('Asset.setMapping', function() {
            it('-- invalid path', function(done) {
                Assets.setMapping('foo', NULL_HASH, function(error/*, result*/) {
                    expect(error).toEqual('Path is invalid');
                    /* expect(result).not.toMatch(IS_ASSET_URL); */
                    done();
                });
            });
            it('-- invalid hash', function(done) {
                Assets.setMapping(SAMPLE_FILE_PATH, 'bar', function(error/*, result*/) {
                    expect(error).toEqual('Hash is invalid');
                    /* expect(result).not.toMatch(IS_ASSET_URL); */
                    done();
                });
            });
        });
        describe('Asset.getMapping', function() {
            it('-- invalid path throws immediate', function() {
                expect(function() {
                    Assets.getMapping('foo', function(error, hash) {
                        throw new Error("should never make it here...");
                    });
                    throw new Error("should never make it here...");
                }).toThrowError(/invalid.*path/i);
            });
            it('-- non-existing path', function(done) {
                Assets.getMapping('/foo/bar/'+Date.now(), function(error, hash) {
                    expect(error).toEqual('Asset not found');
                    expect(hash).not.toMatch(IS_ASSET_HASH_REGEX);
                    done();
                });
            });
        });
    });
});

// ----------------------------------------------------------------------------
// this stuff allows the unit tests to be loaded indepenently and/or as part of testRunner.js execution
function run() {}
function instrument_testrunner(force) {
    if (force || typeof describe === 'undefined') {
        // Include testing library
        Script.include('../../libraries/jasmine/jasmine.js');
        Script.include('../../libraries/jasmine/hifi-boot.js');

        run = function() {
            if (!/:console/.test(Script.resolvePath(''))) {
                // invoke Script.stop (after any async tests complete)
                jasmine.getEnv().addReporter({ jasmineDone: Script.stop });
            } else {
                jasmine.getEnv().addReporter({ jasmineDone: function() { print("JASMINE DONE"); } });
            }

            // Run the tests
            jasmine.getEnv().execute();
        };
    }
}
run();
