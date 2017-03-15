(function() {
    var _this;
    function DoubleClickExample() {
        _this = this;
        return;
    }

    DoubleClickExample.prototype = {
        clickDownOnEntity: function() {
            print("clickDownOnEntity");
        },

        doubleclickOnEntity: function() {
            print("doubleclickOnEntity");
        }

    };
    return new DoubleClickExample();
});