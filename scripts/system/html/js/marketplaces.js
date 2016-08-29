function loaded() {
    bindClaraImage();
    bindClaraButton();
}

function hideList() {
    $('.marketplace-tile').hide();
}

function showList() {
    $('.marketplace-tile').show();
}

function showClaraHowTo() {
    $('.claraHowTo').show();
}

function hideClaraHowTo() {
    $('.claraHowTo').hide();
}

var claraVisible = false;

function bindClaraImage() {
    $('.clara-image').on('click', function() {
        showClaraHowTo();
       // hideList();
    })
}

function bindClaraButton() {
    $('#goToClara').on('click', function() {
        window.location = "http://www.clara.io"
    })
}