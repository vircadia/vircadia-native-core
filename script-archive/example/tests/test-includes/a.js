// a.js:
Script.include('b.js');
if (a === undefined) {
a = 0;
}
a++;
print('script a:' + a);
