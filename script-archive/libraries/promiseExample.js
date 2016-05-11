Script.include('promise.js');
var Promise = loadPromise();
var prom = new Promise(function(resolve, reject) {
    print('making a promise')
        // do a thing, possibly async, then…
    var thing = true;
    if (thing) {
        resolve("Stuff worked!");
    } else {
        print('ERROR')
        reject(new Error("It broke"));
    }
});

// Do something when async done
prom.then(function(result) {
    print('result ' + result);
});