function loaded() {
bindExploreButtons();
}

function bindExploreButtons() {
    $('#exploreClaraMarketplace').on('click', function() {
        window.location = "http://www.clara.io"
    })
    $('#exploreHifiMarketplace').on('click', function() {
        window.location = "http://www.highfidelity.com/marketplace"
    })
}