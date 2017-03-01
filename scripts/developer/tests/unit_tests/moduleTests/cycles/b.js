/* eslint-env node */
var b = exports;
b.done = false;
var a = require('./a.js');
b.done = true;
b.name = 'b';
b['a.done?'] = a.done;
b['b.done?'] = b.done;

print('from b.js a.done =', a.done, '/ b.done =', b.done);
