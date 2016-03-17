print('KINETIC INCLUDING WRAPPER')

var BOOKS_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/kineticObjects/books.json'
var UPPER_BOOKSHELF_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/kineticObjects/upperBookShelf.json';
var LOWER_BOOKSHELF_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/kineticObjects/lowerBookShelf.json';
var RIGHT_DESK_DRAWER_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/kineticObjects/rightDeskDrawer.json';
var LEFT_DESK_DRAWER_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/kineticObjects/leftDeskDrawer.json';
var CHAIR_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/kineticObjects/chair.json'
var DESK_DRAWERS_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/kineticObjects/deskDrawers.json'
var FRUIT_BOWL_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/kineticObjects/fruit.json'
var LAB_LAMP_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/kineticObjects/labLamp.json'
var LIVING_ROOM_LAMP_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/kineticObjects/livingRoomLamp.json'
var TRASHCAN_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/kineticObjects/trashcan.json'

FruitBowl = function(spawnLocation, spawnRotation) {
    print('CREATE FRUIT BOWL')
    var created = [];

    function create() {
        var success = Clipboard.importEntities(FRUIT_BOWL_URL);
        if (success === true) {
            hasBow = true;
            created = Clipboard.pasteEntities(spawnLocation)
            print('created ' + created);
        }
    };

    function cleanup() {
        created.forEach(function(obj) {
            Entities.deleteEntity(obj);
        })
    };

    create();

    this.cleanup = cleanup;

}

LabLamp = function(spawnLocation, spawnRotation) {
    var created = [];

    function create() {
        var success = Clipboard.importEntities(FRUIT_BOWL_URL);
        if (success === true) {
            hasBow = true;
            created = Clipboard.pasteEntities(spawnLocation)
            print('created ' + created);
        }
    };

    function cleanup() {
        created.forEach(function(obj) {
            Entities.deleteEntity(obj);
        })
    };

    create();

    this.cleanup = cleanup;
}

LivingRoomLamp = function(spawnLocation, spawnRotation) {
    var created = [];

    function create() {
        var success = Clipboard.importEntities(FRUIT_BOWL_URL);
        if (success === true) {
            hasBow = true;
            created = Clipboard.pasteEntities(spawnLocation)
            print('created ' + created);
        }
    };

    function cleanup() {
        created.forEach(function(obj) {
            Entities.deleteEntity(obj);
        })
    };

    create();

    this.cleanup = cleanup;
}

UpperBookShelf = function(spawnLocation, spawnRotation) {
    print('CREATE Bookshelves')
    var created = [];

    function create() {
        var success = Clipboard.importEntities(UPPER_BOOKSHELF_URL);
        if (success === true) {
            hasBow = true;
            created = Clipboard.pasteEntities(spawnLocation)
            print('created ' + created);
        }
    };

    function cleanup() {
        created.forEach(function(obj) {
            Entities.deleteEntity(obj);
        })
    };

    create();

    this.cleanup = cleanup;
}


LowerBookShelf = function(spawnLocation, spawnRotation) {
    print('CREATE Bookshelves')
    var created = [];

    function create() {
        var success = Clipboard.importEntities(LOWER_BOOKSHELF_URL);
        if (success === true) {
            hasBow = true;
            created = Clipboard.pasteEntities(spawnLocation)
            print('created ' + created);
        }
    };

    function cleanup() {
        created.forEach(function(obj) {
            Entities.deleteEntity(obj);
        })
    };

    create();

    this.cleanup = cleanup;
}

RightDeskDrawer = function(spawnLocation, spawnRotation) {
    var created = [];

    function create() {
        var success = Clipboard.importEntities(RIGHT_DESK_DRAWER_URL);
        if (success === true) {
            hasBow = true;
            created = Clipboard.pasteEntities(spawnLocation)
            print('created ' + created);
        }
    };

    function cleanup() {
        created.forEach(function(obj) {
            Entities.deleteEntity(obj);
        })
    };

    create();

    this.cleanup = cleanup;
}

LeftDeskDrawer = function(spawnLocation, spawnRotation) {
    print('CREATE Bookshelves')
    var created = [];

    function create() {
        var success = Clipboard.importEntities(LEFT_DESK_DRAWER_URL);
        if (success === true) {
            hasBow = true;
            created = Clipboard.pasteEntities(spawnLocation)
            print('created ' + created);
        }
    };

    function cleanup() {
        created.forEach(function(obj) {
            Entities.deleteEntity(obj);
        })
    };

    create();

    this.cleanup = cleanup;
}

Chair = function(spawnLocation, spawnRotation) {
    var created = [];

    function create() {
        var success = Clipboard.importEntities(CHAIR_URL);
        if (success === true) {
            hasBow = true;
            created = Clipboard.pasteEntities(spawnLocation)
            print('created ' + created);
        }
    };

    function cleanup() {
        created.forEach(function(obj) {
            Entities.deleteEntity(obj);
        })
    };

    create();

    this.cleanup = cleanup;
}

Trashcan = function(spawnLocation, spawnRotation) {
    var created = [];

    function create() {
        var success = Clipboard.importEntities(TRASHCAN_URL);
        if (success === true) {
            hasBow = true;
            created = Clipboard.pasteEntities(spawnLocation)
            print('created ' + created);
        }
    };

    function cleanup() {
        created.forEach(function(obj) {
            Entities.deleteEntity(obj);
        })
    };

    create();

    this.cleanup = cleanup;

}

Books = function(spawnLocation, spawnRotation) {
    var created = [];

    function create() {
        var success = Clipboard.importEntities(BOOKS_URL);
        if (success === true) {
            hasBow = true;
            created = Clipboard.pasteEntities(spawnLocation)
            print('created ' + created);
        }
    };

    function cleanup() {
        created.forEach(function(obj) {
            Entities.deleteEntity(obj);
        })
    };

    create();

    this.cleanup = cleanup;
}