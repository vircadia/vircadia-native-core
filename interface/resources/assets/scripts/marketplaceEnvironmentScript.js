(function(){
    var TABLET = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var MARKETPLACE_PAGE = "https://highfidelity.com/marketplace?category=environments";
    var MARKETPLACES_INJECT_URL = ScriptDiscoveryService.defaultScriptsPath + "/system/html/js/marketplacesInject.js"; 
    function MarketplaceOpenTrigger(){

    }
    function openMarketplaceCategory(){
        TABLET.gotoWebScreen(MARKETPLACE_PAGE, MARKETPLACES_INJECT_URL);
    }
    MarketplaceOpenTrigger.prototype = {
        mousePressOnEntity : function() {
            openMarketplaceCategory();
        },
        startFarTrigger: function() {
            openMarketplaceCategory();
        },
        startNearTrigger: function() {
            openMarketplaceCategory();
        }
    };
    return new MarketplaceOpenTrigger();

});