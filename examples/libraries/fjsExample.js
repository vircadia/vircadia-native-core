Script.include('fjs.js');
var fjs = loadFJS();

var concatenate = fjs.curry(function(word1, word2) {
    return word1 + " " + word2;
});
var concatenateHello = concatenate("Hello");
var hi = concatenateHello("World");
print('anyone listening? ' + hi)