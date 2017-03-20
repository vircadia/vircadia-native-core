/* eslint-env node */
/* global print */
/* eslint-disable comma-dangle */

print('main.js');
var a = require('./a.js'),
    b = require('./b.js');

print('from main.js a.done =', a.done, 'and b.done =', b.done);

module.exports = {
    name: 'main',
    a: a,
    b: b,
    'a.done?': a.done,
    'b.done?': b.done,
};
