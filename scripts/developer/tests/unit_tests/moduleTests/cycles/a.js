/* eslint-env node */
var a = exports;
a.done = false;
var b = require('./b.js');
a.done = true;
a.name = 'a';
a['a.done?'] = a.done;
a['b.done?'] = b.done;

print('from a.js a.done =', a.done, '/ b.done =', b.done);
