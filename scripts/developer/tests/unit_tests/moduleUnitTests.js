/* eslint-env jasmine, node */
/* global print:true, Script:true, global:true, require:true */
/* eslint-disable comma-dangle */
var isNode = instrumentTestrunner(),
    runInterfaceTests = !isNode,
    runNetworkTests = true;

// describe wrappers (note: `xdescribe` indicates a disabled or "pending" jasmine test)
var INTERFACE = { describe: runInterfaceTests ? describe : xdescribe },
    NETWORK = { describe: runNetworkTests ? describe : xdescribe };

describe('require', function() {
    describe('resolve', function() {
        it('should resolve relative filenames', function() {
            var expected = Script.resolvePath('./moduleTests/example.json');
            expect(require.resolve('./moduleTests/example.json')).toEqual(expected);
        });
        describe('exceptions', function() {
            it('should reject blank "" module identifiers', function() {
                expect(function() {
                    require.resolve('');
                }).toThrowError(/Cannot find/);
            });
            it('should reject excessive identifier sizes', function() {
                expect(function() {
                    require.resolve(new Array(8193).toString());
                }).toThrowError(/Cannot find/);
            });
            it('should reject implicitly-relative filenames', function() {
                expect(function() {
                    var mod = require.resolve('example.js');
                    mod.exists;
                }).toThrowError(/Cannot find/);
            });
            it('should reject unanchored, existing filenames with advice', function() {
                expect(function() {
                    var mod = require.resolve('moduleTests/example.json');
                    mod.exists;
                }).toThrowError(/use '.\/moduleTests\/example\.json'/);
            });
            it('should reject unanchored, non-existing filenames', function() {
                expect(function() {
                    var mod = require.resolve('asdfssdf/example.json');
                    mod.exists;
                }).toThrowError(/Cannot find.*system module not found/);
            });
            it('should reject non-existent filenames', function() {
                expect(function() {
                    require.resolve('./404error.js');
                }).toThrowError(/Cannot find/);
            });
            it('should reject identifiers resolving to a directory', function() {
                expect(function() {
                    var mod = require.resolve('.');
                    mod.exists;
                    // console.warn('resolved(.)', mod);
                }).toThrowError(/Cannot find/);
                expect(function() {
                    var mod = require.resolve('..');
                    mod.exists;
                    // console.warn('resolved(..)', mod);
                }).toThrowError(/Cannot find/);
                expect(function() {
                    var mod = require.resolve('../');
                    mod.exists;
                    // console.warn('resolved(../)', mod);
                }).toThrowError(/Cannot find/);
            });
            (isNode ? xit : it)('should reject non-system, extensionless identifiers', function() {
                expect(function() {
                    require.resolve('./example');
                }).toThrowError(/Cannot find/);
            });
        });
    });

    describe('JSON', function() {
        it('should import .json modules', function() {
            var example = require('./moduleTests/example.json');
            expect(example.name).toEqual('Example JSON Module');
        });
        // noet: support for loading JSON via content type workarounds reverted
        // (leaving these tests intact in case ever revisited later)
        // INTERFACE.describe('interface', function() {
        //     NETWORK.describe('network', function() {
        //         xit('should import #content-type=application/json modules', function() {
        //             var results = require('https://jsonip.com#content-type=application/json');
        //             expect(results.ip).toMatch(/^[.0-9]+$/);
        //         });
        //         xit('should import content-type: application/json modules', function() {
        //             var scope = { 'content-type': 'application/json' };
        //             var results = require.call(scope, 'https://jsonip.com');
        //             expect(results.ip).toMatch(/^[.0-9]+$/);
        //         });
        //     });
        // });

    });

    INTERFACE.describe('system', function() {
        it('require("vec3")', function() {
            expect(require('vec3')).toEqual(jasmine.any(Function));
        });
        it('require("vec3").method', function() {
            expect(require('vec3')().isValid).toEqual(jasmine.any(Function));
        });
        it('require("vec3") as constructor', function() {
            var vec3 = require('vec3');
            var v = vec3(1.1, 2.2, 3.3);
            expect(v).toEqual(jasmine.any(Object));
            expect(v.isValid).toEqual(jasmine.any(Function));
            expect(v.isValid()).toBe(true);
            expect(v.toString()).toEqual('[Vec3 (1.100,2.200,3.300)]');
        });
    });

    describe('cache', function() {
        it('should cache modules by resolved module id', function() {
            var value = new Date;
            var example = require('./moduleTests/example.json');
            // earmark the module object with a unique value
            example['.test'] = value;
            var example2 = require('../../tests/unit_tests/moduleTests/example.json');
            expect(example2).toBe(example);
            // verify earmark is still the same after a second require()
            expect(example2['.test']).toBe(example['.test']);
        });
        it('should reload cached modules set to null', function() {
            var value = new Date;
            var example = require('./moduleTests/example.json');
            example['.test'] = value;
            require.cache[require.resolve('../../tests/unit_tests/moduleTests/example.json')] = null;
            var example2 = require('../../tests/unit_tests/moduleTests/example.json');
            // verify the earmark is *not* the same as before
            expect(example2).not.toBe(example);
            expect(example2['.test']).not.toBe(example['.test']);
        });
        it('should reload when module property is deleted', function() {
            var value = new Date;
            var example = require('./moduleTests/example.json');
            example['.test'] = value;
            delete require.cache[require.resolve('../../tests/unit_tests/moduleTests/example.json')];
            var example2 = require('../../tests/unit_tests/moduleTests/example.json');
            // verify the earmark is *not* the same as before
            expect(example2).not.toBe(example);
            expect(example2['.test']).not.toBe(example['.test']);
        });
    });

    describe('cyclic dependencies', function() {
        describe('should allow lazy-ref cyclic module resolution', function() {
            var main;
            beforeEach(function() {
                // eslint-disable-next-line
                try { this._print = print; } catch (e) {}
                // during these tests print() is no-op'd so that it doesn't disrupt the reporter output
                print = function() {};
                Script.resetModuleCache();
            });
            afterEach(function() {
                print = this._print;
            });
            it('main is requirable', function() {
                main = require('./moduleTests/cycles/main.js');
                expect(main).toEqual(jasmine.any(Object));
            });
            it('transient a and b done values', function() {
                expect(main.a['b.done?']).toBe(true);
                expect(main.b['a.done?']).toBe(false);
            });
            it('ultimate a.done?', function() {
                expect(main['a.done?']).toBe(true);
            });
            it('ultimate b.done?', function() {
                expect(main['b.done?']).toBe(true);
            });
        });
    });

    describe('JS', function() {
        it('should throw catchable local file errors', function() {
            expect(function() {
                require('file:///dev/null/non-existent-file.js');
            }).toThrowError(/path not found|Cannot find.*non-existent-file/);
        });
        it('should throw catchable invalid id errors', function() {
            expect(function() {
                require(new Array(4096 * 2).toString());
            }).toThrowError(/invalid.*size|Cannot find.*,{30}/);
        });
        it('should throw catchable unresolved id errors', function() {
            expect(function() {
                require('foobar:/baz.js');
            }).toThrowError(/could not resolve|Cannot find.*foobar:/);
        });

        NETWORK.describe('network', function() {
            // note: depending on retries these tests can take up to 60 seconds each to timeout
            var timeout = 75 * 1000;
            it('should throw catchable host errors', function() {
                expect(function() {
                    var mod = require('http://non.existent.highfidelity.io/moduleUnitTest.js');
                    print("mod", Object.keys(mod));
                }).toThrowError(/error retrieving script .ServerUnavailable.|Cannot find.*non.existent/);
            }, timeout);
            it('should throw catchable network timeouts', function() {
                expect(function() {
                    require('http://ping.highfidelity.io:1024');
                }).toThrowError(/error retrieving script .Timeout.|Cannot find.*ping.highfidelity/);
            }, timeout);
        });
    });

    INTERFACE.describe('entity', function() {
        var sampleScripts = [
            'entityConstructorAPIException.js',
            'entityConstructorModule.js',
            'entityConstructorNested2.js',
            'entityConstructorNested.js',
            'entityConstructorRequireException.js',
            'entityPreloadAPIError.js',
            'entityPreloadRequire.js',
        ].filter(Boolean).map(function(id) {
            return Script.require.resolve('./moduleTests/entity/'+id);
        });

        var uuids = [];
        function cleanup() {
            uuids.splice(0,uuids.length).forEach(function(uuid) {
                Entities.deleteEntity(uuid);
            });
        }
        afterAll(cleanup);
        // extra sanity check to avoid lingering entities
        Script.scriptEnding.connect(cleanup);

        for (var i=0; i < sampleScripts.length; i++) {
            maketest(i);
        }

        function maketest(i) {
            var script = sampleScripts[ i % sampleScripts.length ];
            var shortname = '['+i+'] ' + script.split('/').pop();
            var position = MyAvatar.position;
            position.y -= i/2;
            // define a unique jasmine test for the current entity script
            it(shortname, function(done) {
                var uuid = Entities.addEntity({
                    text: shortname,
                    description: Script.resolvePath('').split('/').pop(),
                    type: 'Text',
                    position: position,
                    rotation: MyAvatar.orientation,
                    script: script,
                    scriptTimestamp: +new Date,
                    lifetime: 20,
                    lineHeight: 1/8,
                    dimensions: { x: 2, y: 0.5, z: 0.01 },
                    backgroundColor: { red: 0, green: 0, blue: 0 },
                    color: { red: 0xff, green: 0xff, blue: 0xff },
                }, !Entities.serversExist() || !Entities.canRezTmp());
                uuids.push(uuid);
                function stopChecking() {
                    if (ii) {
                        Script.clearInterval(ii);
                        ii = 0;
                    }
                }
                var ii = Script.setInterval(function() {
                    Entities.queryPropertyMetadata(uuid, "script", function(err, result) {
                        if (err) {
                            stopChecking();
                            throw new Error(err);
                        }
                        if (result.success) {
                            stopChecking();
                            if (/Exception/.test(script)) {
                                expect(result.status).toMatch(/^error_(loading|running)_script$/);
                            } else {
                                expect(result.status).toEqual("running");
                            }
                            Entities.deleteEntity(uuid);
                            done();
                        } else {
                            print('!result.success', JSON.stringify(result));
                        }
                    });
                }, 100);
                Script.setTimeout(stopChecking, 4900);
            }, 5000 /* jasmine async timeout */);
        }
    });
});

// support for isomorphic Node.js / Interface unit testing
// note: run `npm install` from unit_tests/ and then `node moduleUnitTests.js`
function run() {}
function instrumentTestrunner() {
    var isNode = typeof process === 'object' && process.title === 'node';
    if (typeof describe === 'function') {
        // already running within a test runner; assume jasmine is ready-to-go
        return isNode;
    }
    if (isNode) {
        /* eslint-disable no-console */
        // Node.js test mode
        // to keep things consistent Node.js uses the local jasmine.js library (instead of an npm version)
        var jasmineRequire = require('../../libraries/jasmine/jasmine.js');
        var jasmine = jasmineRequire.core(jasmineRequire);
        var env = jasmine.getEnv();
        var jasmineInterface = jasmineRequire.interface(jasmine, env);
        for (var p in jasmineInterface) {
            global[p] = jasmineInterface[p];
        }
        env.addReporter(new (require('jasmine-console-reporter')));
        // testing mocks
        Script = {
            resetModuleCache: function() {
                module.require.cache = {};
            },
            setTimeout: setTimeout,
            clearTimeout: clearTimeout,
            resolvePath: function(id) {
                // this attempts to accurately emulate how Script.resolvePath works
                var trace = {}; Error.captureStackTrace(trace);
                var base = trace.stack.split('\n')[2].replace(/^.*[(]|[)].*$/g,'').replace(/:[0-9]+:[0-9]+.*$/,'');
                if (!id) {
                    return base;
                }
                var rel = base.replace(/[^\/]+$/, id);
                console.info('rel', rel);
                return require.resolve(rel);
            },
            require: function(mod) {
                return require(Script.require.resolve(mod));
            },
        };
        Script.require.cache = require.cache;
        Script.require.resolve = function(mod) {
            if (mod === '.' || /^\.\.($|\/)/.test(mod)) {
                throw new Error("Cannot find module '"+mod+"' (is dir)");
            }
            var path = require.resolve(mod);
            // console.info('node-require-reoslved', mod, path);
            try {
                if (require('fs').lstatSync(path).isDirectory()) {
                    throw new Error("Cannot find module '"+path+"' (is directory)");
                }
                // console.info('!path', path);
            } catch (e) {
                console.error(e);
            }
            return path;
        };
        print = console.info.bind(console, '[print]');
        /* eslint-enable no-console */
    } else {
        // Interface test mode
        global = this;
        Script.require('../../../system/libraries/utils.js');
        this.jasmineRequire = Script.require('../../libraries/jasmine/jasmine.js');
        Script.require('../../libraries/jasmine/hifi-boot.js');
        require = Script.require;
        // polyfill console
        /* global console:true */
        console = {
            log: print,
            info: print.bind(this, '[info]'),
            warn: print.bind(this, '[warn]'),
            error: print.bind(this, '[error]'),
            debug: print.bind(this, '[debug]'),
        };
    }
    // eslint-disable-next-line
    run = function() { global.jasmine.getEnv().execute(); };
    return isNode;
}
run();
