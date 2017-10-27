/* eslint-env jasmine */

Script.include('../../../system/libraries/utils.js');

describe('Bind', function() {
    it('exists for functions', function() {
        var FUNC = 'function';
        expect(typeof(function() {}.bind)).toEqual(FUNC);
    });

    it('should allow for setting context of this', function() {
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
        
        expect(instance.doSomething()).toEqual(foo);
    });
});
