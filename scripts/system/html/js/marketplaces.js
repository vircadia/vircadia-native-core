function loaded() {
    bindExploreButtons();
}

function bindExploreButtons() {
    $('#exploreClaraMarketplace').on('click', function () {
        window.location = "https://clara.io/library?public=true"
    })
    $('#exploreHifiMarketplace').on('click', function () {
        window.location = "http://www.highfidelity.com/marketplace"
    })
}