// start.js:
var a, b;
print('initially: a:' + a + ' b:' + b);
Script.include(['a.js', '../test-includes/a.js', 'b.js', 'a.js']);
print('finally a:' + a + ' b:' + b);
