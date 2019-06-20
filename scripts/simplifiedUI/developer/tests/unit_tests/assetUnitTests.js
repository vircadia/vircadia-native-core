/* eslint-env jasmine */

if (!Entities.canWriteAssets()) {
    Window.alert('!Entities.canWriteAssets -- please goto a domain with asset rights and reload this test script');
    throw new Error('!Entities.canWriteAssets');
}

instrument_testrunner(true);

describe("Assets", function () {
    var context = {
        memo: {},
        cache: {},
        definedHash: null,
        definedPath: null,
        definedContent: null,
    };
    var ATP_TIMEOUT_MS = 1000; // ms

    var NULL_HASH = new Array(64+1).join('0'); // 64 hex
    var SAMPLE_FOLDER = '/assetUnitTests';
    var SAMPLE_FILE_PATH = SAMPLE_FOLDER + "/test.json";
    var SAMPLE_CONTENTS = 'Test Run on ' + JSON.stringify(Date());
    var SAMPLE_JSON = JSON.stringify({ content: SAMPLE_CONTENTS });
    var SAMPLE_FLOATS = [ Math.PI, 1.1, 2.2, 3.3 ];
    var SAMPLE_BUFFER = new Float32Array(SAMPLE_FLOATS).buffer;

    var IS_ASSET_HASH_REGEX = /^[a-f0-9]{64}$/i;
    var IS_ASSET_URL = /^atp:/;
    it('Entities.canWriteAssets', function() {
        expect(Entities.canWriteAssets()).toBe(true);
    });

    describe('extractAssetHash(input)', function() {
        // new helper method that provides a catch-all way to get at the sha256 hash
        // considering the multiple, different ways ATP hash URLs are found across the
        // system and in content.

        var POSITIVE_TEST_URLS = [
            'atp:HASH',
            'atp:HASH.obj',
            'atp:HASH.fbx#cache-buster',
            'atp:HASH.fbx?cache-buster',
            'HASH'
        ];
        var NEGATIVE_TEST_URLS = [
            'asdf',
            'http://domain.tld',
            '/path/filename.fbx',
            'atp:/path/filename.fbx?#HASH',
            'atp:/.baked/HASH/asset.fbx',
            ''
        ];
        it("POSITIVE_TEST_URLS", function() {
            POSITIVE_TEST_URLS.forEach(function(url) {
                url = url.replace('HASH', NULL_HASH);
                expect([url, Assets.extractAssetHash(url)].join('|')).toEqual([url, NULL_HASH].join('|'));
            });
        });
        it("NEGATIVE_TEST_URLS", function() {
            NEGATIVE_TEST_URLS.forEach(function(url) {
                expect(Assets.extractAssetHash(url.replace('HASH', NULL_HASH))).toEqual('');
            });
        });
    });

    // new APIs
    describe('.putAsset', function() {
        it("data", function(done) {
            Assets.putAsset(SAMPLE_CONTENTS, function(error, result) {
                expect(error).toBe(null);
                expect(result.url).toMatch(IS_ASSET_URL);
                expect(result.hash).toMatch(IS_ASSET_HASH_REGEX);
                context.memo.stringURL = result.url;
                done();
            });
        });
        it('nopath.text', function(done) {
            Assets.putAsset({
                data: SAMPLE_CONTENTS,
            }, function(error, result) {
                expect(error).toBe(null);
                expect(result.url).toMatch(IS_ASSET_URL);
                expect(result.hash).toMatch(IS_ASSET_HASH_REGEX);
                context.memo.textHash = result.hash;
                context.memo.textURL = result.url;
                done();
            });
        });
        it('path.text', function(done) {
            var samplePath =  SAMPLE_FOLDER + '/content.json';
            Assets.putAsset({
                path: samplePath,
                data: SAMPLE_JSON,
            }, function(error, result) {
                expect(error).toBe(null);
                expect(result.url).toMatch(IS_ASSET_URL);
                expect(result.path).toEqual(samplePath);
                expect(result.hash).toMatch(IS_ASSET_HASH_REGEX);
                context.memo.jsonPath = result.path;
                done();
            });
        });
        it('path.buffer', function(done) {
            var samplePath =  SAMPLE_FOLDER + '/content.buffer';
            Assets.putAsset({
                path: samplePath,
                data: SAMPLE_BUFFER,
            }, function(error, result) {
                expect(error).toBe(null);
                expect(result.url).toMatch(IS_ASSET_URL);
                expect(result.path).toEqual(samplePath);
                expect(result.hash).toMatch(IS_ASSET_HASH_REGEX);
                expect(result.byteLength).toEqual(SAMPLE_BUFFER.byteLength);
                context.memo.bufferURL = result.url;
                context.memo.bufferHash = result.hash;
                done();
            });
        });
        it('path.json.gz', function(done) {
            var samplePath = SAMPLE_FOLDER + '/content.json.gz';
            Assets.putAsset({
                path: samplePath,
                data: SAMPLE_JSON,
                compress: true,
            }, function(error, result) {
                expect(error).toBe(null);
                expect(result.url).toMatch(IS_ASSET_URL);
                expect(result.path).toEqual(samplePath);
                expect(result.hash).toMatch(IS_ASSET_HASH_REGEX);
                context.memo.jsonCompressedPath = result.path;
                done();
            });
        });
    });
    // it('.deleteAsset(options, {callback(error, result)})', function() { pending(); });
    it('.resolveAsset', function(done) {
        expect(context.memo.bufferURL).toBeDefined();
        Assets.resolveAsset(context.memo.bufferURL, function(error, result) {
            expect(error).toBe(null);
            expect(result.url).toEqual(context.memo.bufferURL);
            expect(result.hash).toEqual(context.memo.bufferHash);
            done();
        });
    });
    describe('.getAsset', function() {
        it('path/', function(done) {
            expect(context.memo.jsonPath).toBeDefined();
            Assets.getAsset(context.memo.jsonPath, function(error, result) {
                expect(error).toBe(null);
                expect(result.response).toEqual(SAMPLE_JSON);
                done();
            });
        });
        it('text', function(done) {
            expect(context.memo.textURL).toBeDefined();
            Assets.getAsset({
                url: context.memo.textURL,
                responseType: 'text',
            }, function(error, result) {
                expect(error).toBe(null);
                expect(result.response).toEqual(SAMPLE_CONTENTS);
                expect(result.url).toEqual(context.memo.textURL);
                done();
            });
        });
        it('arraybuffer', function(done) {
            expect(context.memo.bufferURL).toBeDefined();
            Assets.getAsset({
                url: context.memo.bufferURL,
                responseType: 'arraybuffer',
            }, function(error, result) {
                expect(error).toBe(null);
                expect(result.response.byteLength).toEqual(SAMPLE_BUFFER.byteLength);
                var expected = [].slice.call(new Float32Array(SAMPLE_BUFFER)).join(',');
                var actual = [].slice.call(new Float32Array(result.response)).join(',');
                expect(actual).toEqual(expected);
                done();
            });
        });
        it('json', function(done) {
            expect(context.memo.jsonPath).toBeDefined();
            Assets.getAsset({
                url: context.memo.jsonPath,
                responseType: 'json',
            }, function(error, result) {
                expect(error).toBe(null);
                expect(result.response.content).toEqual(SAMPLE_CONTENTS);
                done();
            });
        });
        it('json.gz', function(done) {
            expect(context.memo.jsonCompressedPath).toBeDefined();
            Assets.getAsset({
                url: context.memo.jsonCompressedPath,
                responseType: 'json',
                decompress: true,
            }, function(error, result) {
                expect(error).toBe(null);
                expect(result.decompressed).toBe(true);
                expect(result.response.content).toEqual(SAMPLE_CONTENTS);
                done();
            });
        });
    });

    // cache APIs
    it('.getCacheStatus', function(done) {
        Assets.getCacheStatus(function(error, result) {
            expect(error).toBe(null);
            expect(result.cacheSize).toBeGreaterThan(0);
            expect(result.maximumCacheSize).toBeGreaterThan(0);
            done();
        });
    });
    describe('.saveToCache', function() {
        it(".data", function(done) {
            Assets.saveToCache({ data: SAMPLE_CONTENTS }, function(error, result) {
                expect(error).toBe(null);
                expect(result.success).toBe(true);
                context.cache.textURL = result.url;
                done();
            });
        });
        it('.url', function(done) {
            var sampleURL =  'atp:' + SAMPLE_FOLDER + '/cache.json';
            Assets.saveToCache({
                url: sampleURL,
                data: SAMPLE_JSON,
            }, function(error, result) {
                expect(error).toBe(null);
                expect(result.url).toMatch(IS_ASSET_URL);
                expect(result.url).toEqual(sampleURL);
                expect(result.success).toBe(true);
                context.cache.jsonURL = result.url;
                done();
            });
        });
        it('.buffer', function(done) {
            var sampleURL =  'atp:' + SAMPLE_FOLDER + '/cache.buffer';
            Assets.saveToCache({
                url: sampleURL,
                data: SAMPLE_BUFFER,
            }, function(error, result) {
                expect(error).toBe(null);
                expect(result.url).toMatch(IS_ASSET_URL);
                expect(result.url).toEqual(sampleURL);
                expect(result.success).toBe(true);
                expect(result.byteLength).toEqual(SAMPLE_BUFFER.byteLength);
                context.cache.bufferURL = result.url;
                done();
            });
        });
    });
    it('.queryCacheMeta', function(done) {
        expect(context.cache.bufferURL).toBeDefined();
        Assets.queryCacheMeta(context.cache.bufferURL, function(error, result) {
            expect(error).toBe(null);
            expect(result.url).toMatch(IS_ASSET_URL);
            expect(result.url).toEqual(context.cache.bufferURL);
            done();
        });
    });
    describe('.loadFromCache', function() {
        it('urlString', function(done) {
            expect(context.cache.jsonURL).toBeDefined();
            Assets.loadFromCache(context.cache.jsonURL, function(error, result) {
                expect(error).toBe(null);
                expect(result.response).toEqual(SAMPLE_JSON);
                done();
            });
        });
        it('.responseType=text', function(done) {
            expect(context.cache.textURL).toBeDefined();
            Assets.loadFromCache({
                url: context.cache.textURL,
                responseType: 'text',
            }, function(error, result) {
                expect(error).toBe(null);
                expect(result.response).toEqual(SAMPLE_CONTENTS);
                done();
            });
        });
        it('.responseType=arraybuffer', function(done) {
            expect(context.cache.bufferURL).toBeDefined();
            Assets.loadFromCache({
                url: context.cache.bufferURL,
                responseType: 'arraybuffer',
            }, function(error, result) {
                expect(error).toBe(null);
                expect(result.response.byteLength).toEqual(SAMPLE_BUFFER.byteLength);
                var expected = [].slice.call(new Float32Array(SAMPLE_BUFFER)).join(',');
                var actual = [].slice.call(new Float32Array(result.response)).join(',');
                expect(actual).toEqual(expected);
                done();
            });
        });
        it('.responseType=json', function(done) {
            expect(context.cache.jsonURL).toBeDefined();
            Assets.loadFromCache({
                url: context.cache.jsonURL,
                responseType: 'json',
            }, function(error, result) {
                expect(error).toBe(null);
                expect(result.response.content).toEqual(SAMPLE_CONTENTS);
                done();
            });
        });
    });

    // existing newly-mapped APIs
    it('.getATPUrl', function() {
        expect(Assets.getATPUrl(SAMPLE_FILE_PATH)).toEqual('atp:' + SAMPLE_FILE_PATH);
        expect(Assets.getATPUrl(NULL_HASH)).toEqual('atp:' + NULL_HASH);
        expect(Assets.getATPUrl("/file.ext?a=b#c=d")).toEqual('atp:/file.ext?a=b#c=d');
        expect(Assets.getATPUrl("atp:xxx")).toEqual('');
        expect(Assets.getATPUrl("xxx")).toEqual('');
    });
    xit('.isValidPath', function(done) { pending(); });
    xit('.isValidFilePath', function() { pending(); });
    xit('.isValidHash', function() { pending(); });
    xit('.setBakingEnabled', function() { pending(); });

    it("Assets.uploadData(data, {callback(url, hash)})", function (done) {
        Assets.uploadData(SAMPLE_CONTENTS, function(url, hash) {
            expect(url).toMatch(IS_ASSET_URL);
            expect(hash).toMatch(IS_ASSET_HASH_REGEX);
            context.definedHash = hash; // used in later tests
            context.definedContent = SAMPLE_CONTENTS;
            //print('context.definedHash = ' + context.definedHash);
            //print('context.definedContent = ' + context.definedContent);
            done();
        });
    });

    it("Assets.setMapping", function (done) {
        expect(context.definedHash).toMatch(IS_ASSET_HASH_REGEX);
        Assets.setMapping(SAMPLE_FILE_PATH, context.definedHash, function(error) {
            if (error) error += ' ('+JSON.stringify([SAMPLE_FILE_PATH, context.definedHash])+')';
            expect(error).toBe(null);
            context.definedPath = SAMPLE_FILE_PATH;
            //print('context.definedPath = ' + context.definedPath);
            done();
        });
    });

    it("Assets.getMapping", function (done) {
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

    it('.getMapping(nullHash)', function(done) {
        // FIXME: characterization test -- current behavior is that downloadData silently fails
        // when given an asset that doesn't exist
        Assets.downloadData(NULL_HASH, function(result) {
            throw new Error("this callback 'should' not be triggered");
        });
        setTimeout(function() { done(); }, ATP_TIMEOUT_MS);
    });

    it('.downloadData(hash)', function(done) {
        expect(context.definedHash).toMatch(IS_ASSET_HASH_REGEX, 'asfasdf');
        Assets.downloadData('atp:' + context.definedHash, function(result) {
            expect(result).toEqual(context.definedContent);
            done();
        });
    });

    it('.downloadData(nullHash)', function(done) {
        // FIXME: characterization test -- current behavior is that downloadData silently fails
        // when given an asset doesn't exist
        Assets.downloadData(NULL_HASH, function(result) {
            throw new Error("this callback 'should' not be triggered");
        });
        setTimeout(function() { done(); }, ATP_TIMEOUT_MS);
    });

    describe('exceptions', function() {
        describe('.getAsset', function() {
            it('-- invalid data.gz', function(done) {
                expect(context.memo.jsonPath).toBeDefined();
                Assets.getAsset({
                    url: context.memo.jsonPath,
                    responseType: 'json',
                    decompress: true,
                }, function(error, result) {
                    expect(error).toMatch(/gunzip error/);
                    done();
                });
            });
        });
        describe('.setMapping', function() {
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
        describe('.getMapping', function() {
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
        var oldPrint = print;
        window = new OverlayWebWindow({
            title: 'assetUnitTests.js',
            width: 640,
            height: 480,
            source: 'about:blank',
        });
        Script.scriptEnding.connect(window, 'close');
        window.closed.connect(Script, 'stop');
        // wait for window (ready) and test runner (ran) both to initialize
        var ready = false;
        var ran = false;
        window.webEventReceived.connect(function() { ready = true; maybeRun(); });
        run = function() { ran = true; maybeRun(); };

        window.setURL([
            'data:text/html;text,',
            '<body style="height:100%;width:100%;background:#eee;whitespace:pre;font-size:8px;">',
            '<pre id=output></pre><div style="height:2px;"></div>',
            '<body>',
            '<script>('+function(){
                window.addEventListener("DOMContentLoaded",function(){
                    setTimeout(function() {
                        EventBridge.scriptEventReceived.connect(function(msg){
                            output.innerHTML += msg;
                            window.scrollTo(0, 1e10);
                            document.body.scrollTop = 1e10;
                        });
                        EventBridge.emitWebEvent('ready');
                    }, 1000);
                });
            }+')();</script>',
        ].join('\n'));
        print = function() {
            var str = [].slice.call(arguments).join(' ');
            if (ready) {
                window.emitScriptEvent(str + '\n');
            } else {
                oldPrint('!ready', str);
            }
        };

        // Include testing library
        Script.include('../../libraries/jasmine/jasmine.js');
        Script.include('../../libraries/jasmine/hifi-boot.js');

        function maybeRun() {
            if (!ran || !ready) {
                return oldPrint('doRun -- waiting for both script and web window to become available');
            }
            if (!force) {
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
