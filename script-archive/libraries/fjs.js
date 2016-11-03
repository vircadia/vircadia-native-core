/*

http://functionaljs.com/

https://github.com/leecrossley/functional-js/

The MIT License (MIT)
Copyright © 2015 Lee Crossley <leee@hotmail.co.uk>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

 loadFJS = function(){
    return fjs();
}

var fjs = function() {
    "use strict";

    var fjs = {},
        hardReturn = "hardReturn;";

    var lambda = function(exp) {
        if (!fjs.isString(exp)) {
            return;
        }

        var parts = exp.match(/(.*)\s*[=-]>\s*(.*)/);
        parts.shift();

        var params = parts.shift()
            .replace(/^\s*|\s(?=\s)|\s*$|,/g, "").split(" ");
        var body = parts.shift();

        parts = ((!/\s*return\s+/.test(body)) ? "return " : "") + body;
        params.push(parts);

        return Function.apply({}, params);
    };

    var sliceArgs = function(args) {
        return args.length > 0 ? [].slice.call(args, 0) : [];
    };

    fjs.isFunction = function(obj) {
        return !!(obj && obj.constructor && obj.call && obj.apply);
    };

    fjs.isObject = function(obj) {
        return fjs.isFunction(obj) || (!!obj && typeof(obj) === "object");
    };

    fjs.isArray = function(obj) {
        return Object.prototype.toString.call(obj) === "[object Array]";
    };

    var checkFunction = function(func) {
        if (!fjs.isFunction(func)) {
            func = lambda(func);
            if (!fjs.isFunction(func)) {
                throw "fjs Error: Invalid function";
            }
        }
        return func;
    };

    fjs.curry = function(func) {
        func = checkFunction(func);
        return function inner() {
            var _args = sliceArgs(arguments);
            if (_args.length === func.length) {
                return func.apply(null, _args);
            } else if (_args.length > func.length) {
                var initial = func.apply(null, _args);
                return fjs.fold(func, initial, _args.slice(func.length));
            } else {
                return function() {
                    var args = sliceArgs(arguments);
                    return inner.apply(null, _args.concat(args));
                };
            }
        };
    };

    fjs.each = fjs.curry(function(iterator, items) {
        iterator = checkFunction(iterator);
        if (!fjs.exists(items) || !fjs.isArray(items)) {
            return;
        }
        for (var i = 0; i < items.length; i += 1) {
            if (iterator.call(null, items[i], i) === hardReturn) {
                return;
            }
        }
    });

    fjs.map = fjs.curry(function(iterator, items) {
        iterator = checkFunction(iterator);
        var mapped = [];
        fjs.each(function() {
            mapped.push(iterator.apply(null, arguments));
        }, items);
        return mapped;
    });

    fjs.fold = fjs.foldl = fjs.curry(function(iterator, cumulate, items) {
        iterator = checkFunction(iterator);
        fjs.each(function(item, i) {
            cumulate = iterator.call(null, cumulate, item, i);
        }, items);
        return cumulate;
    });

    fjs.reduce = fjs.reducel = fjs.foldll = fjs.curry(function(iterator, items) {
        iterator = checkFunction(iterator);
        var cumulate = items[0];
        items.shift();
        return fjs.fold(iterator, cumulate, items);
    });

    fjs.clone = function(items) {
        var clone = [];
        fjs.each(function(item) {
            clone.push(item);
        }, items);
        return clone;
    };

    fjs.first = fjs.head = fjs.take = fjs.curry(function(iterator, items) {
        iterator = checkFunction(iterator);
        var first;
        fjs.each(function(item) {
            if (iterator.call(null, item)) {
                first = item;
                return hardReturn;
            }
        }, items);
        return first;
    });

    fjs.rest = fjs.tail = fjs.drop = fjs.curry(function(iterator, items) {
        var result = fjs.select(iterator, items);
        result.shift();
        return result;
    });

    fjs.last = fjs.curry(function(iterator, items) {
        var itemsClone = fjs.clone(items);
        return fjs.first(iterator, itemsClone.reverse());
    });

    fjs.every = fjs.all = fjs.curry(function(iterator, items) {
        iterator = checkFunction(iterator);
        var isEvery = true;
        fjs.each(function(item) {
            if (!iterator.call(null, item)) {
                isEvery = false;
                return hardReturn;
            }
        }, items);
        return isEvery;
    });

    fjs.any = fjs.contains = fjs.curry(function(iterator, items) {
        iterator = checkFunction(iterator);
        var isAny = false;
        fjs.each(function(item) {
            if (iterator.call(null, item)) {
                isAny = true;
                return hardReturn;
            }
        }, items);
        return isAny;
    });

    fjs.select = fjs.filter = fjs.curry(function(iterator, items) {
        iterator = checkFunction(iterator);
        var filtered = [];
        fjs.each(function(item) {
            if (iterator.call(null, item)) {
                filtered.push(item);
            }
        }, items);
        return filtered;
    });

    fjs.best = fjs.curry(function(iterator, items) {
        iterator = checkFunction(iterator);
        var compare = function(arg1, arg2) {
            return iterator.call(this, arg1, arg2) ?
                arg1 : arg2;
        };
        return fjs.reduce(compare, items);
    });

    fjs._while = fjs.curry(function(iterator, items) {
        iterator = checkFunction(iterator);
        var result = [];
        fjs.each(function(item) {
            if (iterator.call(null, item)) {
                result.push(item);
            } else {
                return hardReturn;
            }
        }, items);
        return result;
    });

    fjs.compose = function(funcs) {
        var anyInvalid = fjs.any(function(func) {
            return !fjs.isFunction(func);
        });
        funcs = sliceArgs(arguments).reverse();
        if (anyInvalid(funcs)) {
            throw "fjs Error: Invalid function to compose";
        }
        return function() {
            var args = arguments;
            var applyEach = fjs.each(function(func) {
                args = [func.apply(null, args)];
            });
            applyEach(funcs);
            return args[0];
        };
    };

    fjs.partition = fjs.curry(function(iterator, items) {
        iterator = checkFunction(iterator);
        var truthy = [],
            falsy = [];
        fjs.each(function(item) {
            (iterator.call(null, item) ? truthy : falsy).push(item);
        }, items);
        return [truthy, falsy];
    });

    fjs.group = fjs.curry(function(iterator, items) {
        iterator = checkFunction(iterator);
        var result = {};
        var group;
        fjs.each(function(item) {
            group = iterator.call(null, item);
            result[group] = result[group] || [];
            result[group].push(item);
        }, items);
        return result;
    });

    fjs.shuffle = function(items) {
        var j, t;
        fjs.each(function(item, i) {
            j = Math.floor(Math.random() * (i + 1));
            t = items[i];
            items[i] = items[j];
            items[j] = t;
        }, items);
        return items;
    };

    fjs.toArray = function(obj) {
        return fjs.map(function(key) {
            return [key, obj[key]];
        }, Object.keys(obj));
    };

    fjs.apply = fjs.curry(function(func, items) {
        var args = [];
        if (fjs.isArray(func)) {
            args = [].slice.call(func, 1);
            func = func[0];
        }
        return fjs.map(function(item) {
            return item[func].apply(item, args);
        }, items);
    });

    fjs.assign = fjs.extend = fjs.curry(function(obj1, obj2) {
        fjs.each(function(key) {
            obj2[key] = obj1[key];
        }, Object.keys(obj1));
        return obj2;
    });

    fjs.prop = function(prop) {
        return function(obj) {
            return obj[prop];
        };
    };

    fjs.pluck = fjs.curry(function(prop, items) {
        return fjs.map(fjs.prop(prop), items);
    });

    fjs.nub = fjs.unique = fjs.distinct = fjs.curry(function(comparator, items) {
        var unique = items.length > 0 ? [items[0]] : [];

        fjs.each(function(item) {
            if (!fjs.any(fjs.curry(comparator)(item), unique)) {
                unique[unique.length] = item;
            }
        }, items);

        return unique;
    });

    fjs.exists = function(obj) {
        return obj != null; // jshint ignore:line
    };

    fjs.truthy = function(obj) {
        return fjs.exists(obj) && obj !== false;
    };

    fjs.falsy = function(obj) {
        return !fjs.truthy(obj);
    };

    fjs.each(function(type) {
        fjs["is" + type] = function(obj) {
            return Object.prototype.toString.call(obj) === "[object " + type + "]";
        };
    }, ["Arguments", "Date", "Number", "RegExp", "String"]);

    return fjs;
}