Script.include('../libraries/jasmine/jasmine.js');
Script.include('../libraries/jasmine/hifi-boot.js');
Script.include('../../system/libraries/utils.js');

describe('Bind', function() {
    it('functions should have bind available', function() {
        var foo = 'bar';

        function callAnotherFn(anotherFn) {
            return anotherFn();
        }

        function TestConstructor() {
            this.foo = foo;
        }

        TestConstructor.prototype.doSomething = function() {
            return callAnotherFn(function() {
                return this.foo;
            }.bind(this));
        };

        var instance = new TestConstructor();
        
        expect(typeof(instance.doSomething.bind) !== 'undefined');
        expect(instance.doSomething()).toEqual(foo);
    });
});

jasmine.getEnv().execute();
Script.stop();