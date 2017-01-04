(function() {
    var BALLOT_X = '&#10007;';
    var CHECKMARK = '&#10003;';
    var DOWN_RIGHT_ARROW = '&#8627;';
    var PASSED = 'passed';
    var lastSpecStartTime;
    function ConsoleReporter(options) {
        var startTime = new Date().getTime();
        var errorCount = 0;
        this.jasmineStarted = function (obj) {
            print('Jasmine started with ' + obj.totalSpecsDefined + ' tests.');
        };
        this.jasmineDone = function (obj) {
            var ERROR = errorCount === 1 ? 'error' : 'errors';
            var endTime = new Date().getTime();
            print('<hr />');
            if (errorCount === 0) {
                print ('<span style="color:green">All tests passed!</span>');
            } else {
                print('<span style="color:red">Tests completed with ' +
                    errorCount + ' ' + ERROR + '.<span>');
            }
            print('Tests completed in ' + (endTime - startTime) + 'ms.');
        };
        this.suiteStarted = function(obj) {
            print(obj.fullName);
        };
        this.suiteDone = function(obj) {
            print('');
        };
        this.specStarted = function(obj) {
            lastSpecStartTime = new Date().getTime();
        };
        this.specDone = function(obj) {
            var specEndTime = new Date().getTime();
            var symbol = obj.status === PASSED ?
                '<span style="color:green">' + CHECKMARK + '</span>' :
                '<span style="color:red">' + BALLOT_X + '</span>';
            print('... ' + obj.fullName + ' ' + symbol + ' ' + '<span style="color:orange">[' +
                (specEndTime - lastSpecStartTime) + 'ms]</span>');

            var specErrors = obj.failedExpectations.length;
            errorCount += specErrors;
            for (var i = 0; i < specErrors; i++) {
                print('<span style="margin-right:0.5em"></span>' + DOWN_RIGHT_ARROW +
                    '<span style="color:red">  ' +
                    obj.failedExpectations[i].message + '</span>');
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
            if (source.hasOwnProperty(property)) {
                destination[property] = source[property];
            }
        }
        return destination;
    }
}());

