
(function() {
    function ConsoleReporter(options) {
        this.jasmineStarted = function (obj) {
            print("jasmineStarted: numSpecs = " + obj.totalSpecsDefined);
        };
        this.jasmineDone = function (obj) {
            print("jasmineDone");
        };
        this.suiteStarted = function(obj) {
            print("suiteStarted: \"" + obj.fullName + "\"");
        };
        this.suiteDone = function(obj) {
            print("suiteDone: \"" + obj.fullName + "\" " + obj.status);
        };
        this.specStarted = function(obj) {
            print("specStarted: \"" + obj.fullName + "\"");
        };
        this.specDone = function(obj) {
            print("specDone: \"" + obj.fullName + "\" " + obj.status);

            var i, l = obj.failedExpectations.length;
            for (i = 0; i < l; i++) {
                print("  " + obj.failedExpectations[i].message);
            }
        };
        return this;
    }

    setTimeout = Script.setTimeout;
    setInterval = Script.setInterval;
    clearTimeout = Script.clearTimeout;
    clearInterval = Script.clearInterval;

    var jasmine = jasmineRequire.core(jasmineRequire);

    var env = jasmine.getEnv();

    env.addReporter(new ConsoleReporter());

    var jasmineInterface = jasmineRequire.interface(jasmine, env);

    extend(this, jasmineInterface);

    function extend(destination, source) {
        for (var property in source) {
            destination[property] = source[property];
        }
        return destination;
    }

}());

