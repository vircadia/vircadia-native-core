/* eslint-env jasmine */

instrument_testrunner();

describe('Script', function () {
    // get the current filename without calling Script.resolvePath('')
    try {
        throw new Error('stack');
    } catch(e) {
        var filename = e.fileName;
        var dirname = filename.split('/').slice(0, -1).join('/') + '/';
        var parentdir = dirname.split('/').slice(0, -2).join('/') + '/';
    }

    // characterization tests
    // initially these are just to capture how the app works currently
    var testCases = {
        // special relative resolves
                                '': filename,
                               '.': dirname,
                              '..': parentdir,

        // local file "magic" tilde path expansion
            '/~/defaultScripts.js': ScriptDiscoveryService.defaultScriptsPath + '/defaultScripts.js',

        // these schemes appear to always get resolved to empty URLs
                      'qrc://test': '',
                'about:Entities 1': '',
            'ftp://host:port/path': '',
         'data:text/html;text,foo': '',

                      'Entities 1': dirname + 'Entities 1',
                       './file.js': dirname + 'file.js',
                        'c:/temp/': 'file:///c:/temp/',
                         'c:/temp': 'file:///c:/temp',
                          '/temp/': 'file:///temp/',
                             'c:/': 'file:///c:/',
                              'c:': 'file:///c:',
        'file:///~/libraries/a.js': 'file:///~/libraries/a.js',
         '/temp/tested/../file.js': 'file:///temp/tested/../file.js',
           '/~/libraries/utils.js': 'file:///~/libraries/utils.js',
                   '/temp/file.js': 'file:///temp/file.js',
                             '/~/': 'file:///~/',

        // these schemes appear to always get resolved to the same URL again
         'http://highfidelity.com': 'http://highfidelity.com',
               'atp:/highfidelity': 'atp:/highfidelity',
        'atp:c2d7e3a48cadf9ba75e4f8d9f4d80e75276774880405a093fdee36543aa04f':
          'atp:c2d7e3a48cadf9ba75e4f8d9f4d80e75276774880405a093fdee36543aa04f',
    };
    describe('resolvePath', function () {
        Object.keys(testCases).forEach(function(input) {
            it('(' + JSON.stringify(input) + ')', function () {
                expect(Script.resolvePath(input)).toEqual(testCases[input]);
            });
        });
    });

    describe('include', function () {
        var old_cache_buster;
        var cache_buster = '#' + new Date().getTime().toString(36);
        beforeAll(function() {
            old_cache_buster = Settings.getValue('cache_buster');
            Settings.setValue('cache_buster', cache_buster);
        });
        afterAll(function() {
            Settings.setValue('cache_buster', old_cache_buster);
        });
        beforeEach(function() {
            vec3toStr = undefined;
        });
        it('file:///~/system/libraries/utils.js' + cache_buster, function() {
            Script.include('file:///~/system/libraries/utils.js' + cache_buster);
            expect(vec3toStr).toEqual(jasmine.any(Function));
        });
        it('nested' + cache_buster, function() {
            Script.include('./scriptTests/top-level.js' + cache_buster);
            expect(outer).toEqual(jasmine.any(Object));
            expect(inner).toEqual(jasmine.any(Object));
            expect(sibling).toEqual(jasmine.any(Object));
            expect(outer.inner).toEqual(inner.lib);
            expect(outer.sibling).toEqual(sibling.lib);
        });
        describe('errors' + cache_buster, function() {
            var finishes, oldFinishes;
            beforeAll(function() {
                oldFinishes = (1,eval)('this').$finishes;
            });
            afterAll(function() {
                (1,eval)('this').$finishes = oldFinishes;
            });
            beforeEach(function() {
                finishes = (1,eval)('this').$finishes = [];
            });
            it('error', function() {
                // a thrown Error in top-level include aborts that include, but does not throw the error back to JS
                expect(function() {
                    Script.include('./scriptTests/error.js' + cache_buster);
                }).not.toThrowError("error.js");
                expect(finishes.length).toBe(0);
            });
            it('top-level-error', function() {
                // an organice Error in a top-level include aborts that include, but does not throw the error
                expect(function() {
                    Script.include('./scriptTests/top-level-error.js' + cache_buster);
                }).not.toThrowError(/Undefined_symbol/);
                expect(finishes.length).toBe(0);
            });
            it('nested', function() {
                // a thrown Error in a nested include aborts the nested include, but does not abort the top-level script
                expect(function() {
                    Script.include('./scriptTests/nested-error.js' + cache_buster);
                }).not.toThrowError("nested/error.js");
                expect(finishes.length).toBe(1);
            });
            it('nested-syntax-error', function() {
                // a SyntaxError in a nested include breaks only that include (the main script should finish unimpeded)
                expect(function() {
                    Script.include('./scriptTests/nested-syntax-error.js' + cache_buster);
                }).not.toThrowError(/SyntaxEror/);
                expect(finishes.length).toBe(1);
            });
        });
    });
});

// enable scriptUnitTests to be loaded directly
function run() {}
function instrument_testrunner() {
    if (typeof describe === 'undefined') {
        print('instrumenting jasmine', Script.resolvePath(''));
        Script.include('../../libraries/jasmine/jasmine.js');
        Script.include('../../libraries/jasmine/hifi-boot.js');
        jasmine.getEnv().addReporter({ jasmineDone: Script.stop });
        run = function() {
            print('executing jasmine', Script.resolvePath(''));
            jasmine.getEnv().execute();
        };
    }
}
run();
