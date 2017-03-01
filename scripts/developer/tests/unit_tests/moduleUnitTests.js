/* eslint-env jasmine */

var isNode = instrument_testrunner();

var NETWORK_describe   = xdescribe,
    INTERFACE_describe = !isNode ? describe : xdescribe,
    NODE_describe      = isNode ? describe : xdescribe;

print("DESCRIBING");
describe('require', function() {
    describe('resolve', function() {
        it('should resolve relative filenames', function() {
            var expected = Script.resolvePath('./moduleTests/example.json');
            expect(require.resolve('./moduleTests/example.json')).toEqual(expected);
        });
    });

    describe('JSON', function() {
        it('should import .json modules', function() {
            var example = require('./moduleTests/example.json');
            expect(example.name).toEqual('Example JSON Module');
        });
        INTERFACE_describe('inteface', function() {
            NETWORK_describe('network', function() {
                //xit('should import #content-type=application/json modules', function() {
                //    var results = require('https://jsonip.com#content-type=application/json');
                //    expect(results.ip).toMatch(/^[.0-9]+$/);
                //});
                it('should import content-type: application/json modules', function() {
                    var scope = { 'content-type': 'application/json' };
                    var results = require.call(scope, 'https://jsonip.com');
                    expect(results.ip).toMatch(/^[.0-9]+$/);
                });
            });
        });

    });

    INTERFACE_describe('system', function() {
        it('require(id)', function() {
            expect(require('vec3')).toEqual(jasmine.any(Function));
        });
        it('require(id).function', function() {
            expect(require('vec3')().isValid).toEqual(jasmine.any(Function));
        });
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
            }).toThrowError(/Cannot find/);
        });
        it('should reject non-existent filenames', function() {
            expect(function() {
                var mod = require.resolve('./404error.js');
            }).toThrowError(/Cannot find/);
        });
        it('should reject identifiers resolving to a directory', function() {
            expect(function() {
                var mod = require.resolve('.');
                //console.warn('resolved(.)', mod);
            }).toThrowError(/Cannot find/);
            expect(function() {
                var mod = require.resolve('..');
                //console.warn('resolved(..)', mod);
            }).toThrowError(/Cannot find/);
            expect(function() {
                var mod = require.resolve('../');
                //console.warn('resolved(../)', mod);
            }).toThrowError(/Cannot find/);
        });
        if (typeof MODE !== 'undefined' && MODE !== 'node') {
            it('should reject non-system, extensionless identifiers', function() {
                expect(function() {
                    require.resolve('./example');
                }).toThrowError(/Cannot find/);
            });
        }
    });

    describe('cache', function() {
        it('should cache modules by resolved module id', function() {
            var value = new Date;
            var example = require('./moduleTests/example.json');
            example['.test'] = value;
            var example2 = require('../../tests/unit_tests/moduleTests/example.json');
            expect(example2).toBe(example);
            expect(example2['.test']).toBe(example['.test']);
        });
        it('should reload cached modules set to null', function() {
            var value = new Date;
            var example = require('./moduleTests/example.json');
            example['.test'] = value;
            require.cache[require.resolve('../../tests/unit_tests/moduleTests/example.json')] = null;
            var example2 = require('../../tests/unit_tests/moduleTests/example.json');
            expect(example2).not.toBe(example);
            expect(example2['.test']).not.toBe(example['.test']);
        });
        it('should reload when module property is deleted', function() {
            var value = new Date;
            var example = require('./moduleTests/example.json');
            example['.test'] = value;
            delete require.cache[require.resolve('../../tests/unit_tests/moduleTests/example.json')];
            var example2 = require('../../tests/unit_tests/moduleTests/example.json');
            expect(example2).not.toBe(example);
            expect(example2['.test']).not.toBe(example['.test']);
        });
    });

    describe('cyclic dependencies', function() {
        describe('should allow lazy-ref cyclic module resolution', function() {
            const MODULE_PATH = './moduleTests/cycles/main.js';
            var main;
            beforeEach(function() {
                try { this._print = print; } catch(e) {}
                // for this test print is no-op'd so it doesn't disrupt the reporter output
                //console = typeof console === 'object' ? console : { log: function() {} };
                print = function() {};
                Script.resetModuleCache();
            });
            afterEach(function() {
                print = this._print;
            });
            it('main requirable', function() {
                main = require(MODULE_PATH);
                expect(main).toEqual(jasmine.any(Object));
            });
            it('main with both a and b', function() {
                expect(main.a['b.done?']).toBe(true);
                expect(main.b['a.done?']).toBe(false);
            });
            it('a.done?', function() {
                expect(main['a.done?']).toBe(true);
            });
            it('b.done?', function() {
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

        NETWORK_describe('network', function() {
            // note: with retries these tests can take up to 60 seconds each to timeout
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

    INTERFACE_describe('entity', function() {
        var sampleScripts = [
            'entityConstructorAPIException.js',
            'entityConstructorModule.js',
            'entityConstructorNested2.js',
            'entityConstructorNested.js',
            'entityConstructorRequireException.js',
            'entityPreloadAPIError.js',
            'entityPreloadRequire.js',
        ].filter(Boolean).map(function(id) { return Script.require.resolve('./moduleTests/entity/'+id); });

        var uuids = [];

        for(var i=0; i < sampleScripts.length; i++) {
            (function(i) {
                var script = sampleScripts[ i % sampleScripts.length ];
                var shortname = '['+i+'] ' + script.split('/').pop();
                var position = MyAvatar.position;
                position.y -= i/2;
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
                        dimensions: { x: 2, y: .5, z: .01 },
                        backgroundColor: { red: 0, green: 0, blue: 0 },
                        color: { red: 0xff, green: 0xff, blue: 0xff },
                    }, !Entities.serversExist() || !Entities.canRezTmp());
                    uuids.push(uuid);
                    var ii = Script.setInterval(function() {
                        Entities.queryPropertyMetadata(uuid, "script", function(err, result) {
                            if (err) {
                                throw new Error(err);
                            }
                            if (result.success) {
                                clearInterval(ii);
                                if (/Exception/.test(script))
                                    expect(result.status).toMatch(/^error_(loading|running)_script$/);
                                else
                                    expect(result.status).toEqual("running");
                                done();
                            } else {
                                print('!result.success', JSON.stringify(result));
                            }
                        });
                    }, 100);
                    Script.setTimeout(function() {
                        Script.clearInterval(ii);
                    }, 4900);
                }, 5000 /* timeout */);
            })(i);
        }
        Script.scriptEnding.connect(function() {
            uuids.forEach(function(uuid) { Entities.deleteEntity(uuid); });
        });
    });
});

function run() {}
function instrument_testrunner() {
    var isNode = typeof process === 'object' && process.title === 'node';
    if (isNode) {
        // for consistency this still uses the same local jasmine.js library
        var jasmineRequire = require('../../libraries/jasmine/jasmine.js');
        var jasmine = jasmineRequire.core(jasmineRequire);
        var env = jasmine.getEnv();
        var jasmineInterface = jasmineRequire.interface(jasmine, env);
        for (var p in jasmineInterface)
            global[p] = jasmineInterface[p];
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
                if (!id)
                    return base;
                var rel = base.replace(/[^\/]+$/, id);
                console.info('rel', rel);
                return require.resolve(rel);
            },
            require: function(mod) {
                return require(Script.require.resolve(mod));
            }
        };
        Script.require.cache = require.cache;
        Script.require.resolve = function(mod) {
            if (mod === '.' || /^\.\.($|\/)/.test(mod))
                throw new Error("Cannot find module '"+mod+"' (is dir)");
            var path = require.resolve(mod);
            //console.info('node-require-reoslved', mod, path);
            try {
                if (require('fs').lstatSync(path).isDirectory()) {
                    throw new Error("Cannot find module '"+path+"' (is directory)");
                }
                //console.info('!path', path);
            } catch(e) { console.info(e) }
            return path;
        };
        print = console.info.bind(console, '[print]');
    } else {
        global = this;
        // Interface Test mode
        Script.require('../../../system/libraries/utils.js');
        this.jasmineRequire = Script.require('../../libraries/jasmine/jasmine.js');
        Script.require('../../libraries/jasmine/hifi-boot.js')
        require = Script.require;
        // polyfill console
        console = {
            log: print,
            info: print.bind(this, '[info]'),
            warn: print.bind(this, '[warn]'),
            error: print.bind(this, '[error]'),
            debug: print.bind(this, '[debug]'),
        };
    }
    run = function() { global.jasmine.getEnv().execute(); };
    return isNode;
}
run();
