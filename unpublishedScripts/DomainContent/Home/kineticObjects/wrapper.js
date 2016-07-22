print('HOME KINETIC INCLUDING WRAPPER');

var BOOKS_URL = "atp:/kineticObjects/books.json";
var UPPER_BOOKSHELF_URL = "atp:/kineticObjects/upperBookShelf.json";
var LOWER_BOOKSHELF_URL = "atp:/kineticObjects/lowerBookShelf.json";
var CHAIR_URL = 'atp:/kineticObjects/deskChair.json';
var FRUIT_BOWL_URL = "atp:/kineticObjects/fruit.json"
var LIVING_ROOM_LAMP_URL = "atp:/kineticObjects/deskLamp.json";
var TRASHCAN_URL = "atp:/kineticObjects/trashcan.json"
var BLOCKS_URL = "atp:/kineticObjects/blocks.json";
var PLAYA_POSTER_URL = "atp:/kineticObjects/postersPlaya.json";
var CELL_POSTER_URL = "atp:/kineticObjects/postersCell.json";
var STUFF_ON_SHELVES_URL = "atp:/kineticObjects/stuff_on_shelves.json";
var JUNK_URL = "atp:/kineticObjects/junk.json";
var BRICABRAC_URL = "atp:/kineticObjects/dressingRoomBricabrac.json";
var BENCH_URL = "atp:/kineticObjects/bench.json";
var UMBRELLA_URL = "atp:/kineticObjects/umbrella.json";

FruitBowl = function(spawnLocation, spawnRotation) {
    print('CREATE FRUIT BOWL');
    var created = [];

    function create() {
        var success = Clipboard.importEntities(FRUIT_BOWL_URL);
        if (success === true) {
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
    print('CREATE LIVING ROOM LAMP');
    var created = [];

    function create() {
        var success = Clipboard.importEntities(LIVING_ROOM_LAMP_URL);
        if (success === true) {
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
    print('CREATE UPPER SHELF');
    var created = [];

    function create() {
        var success = Clipboard.importEntities(UPPER_BOOKSHELF_URL);
        if (success === true) {
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
    print('CREATE LOWER SHELF');
    var created = [];

    function create() {
        var success = Clipboard.importEntities(LOWER_BOOKSHELF_URL);
        if (success === true) {
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
    print('CREATE CHAIR');
    var created = [];

    function create() {
        var success = Clipboard.importEntities(CHAIR_URL);
        if (success === true) {
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
    print('CREATE TRASHCAN');
    var created = [];

    function create() {
        var success = Clipboard.importEntities(TRASHCAN_URL);
        if (success === true) {
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
    print('CREATE BOOKS');
    var created = [];

    function create() {
        var success = Clipboard.importEntities(BOOKS_URL);
        if (success === true) {
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

Blocks = function(spawnLocation, spawnRotation) {
    print('EBL CREATE BLOCKS');
    var created = [];

    function create() {
        var success = Clipboard.importEntities(BLOCKS_URL);
        if (success === true) {
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

PosterCell = function(spawnLocation, spawnRotation) {
    print('CREATE CELL POSTER');
    var created = [];

    function create() {
        var success = Clipboard.importEntities(CELL_POSTER_URL);
        if (success === true) {
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

PosterPlaya = function(spawnLocation, spawnRotation) {
    print('CREATE PLAYA POSTER');
    var created = [];

    function create() {
        var success = Clipboard.importEntities(PLAYA_POSTER_URL);
        if (success === true) {
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

StuffOnShelves = function(spawnLocation, spawnRotation) {
    print('CREATE STUFF ON SHELVES');
    var created = [];


    function create() {
        var success = Clipboard.importEntities(STUFF_ON_SHELVES_URL);
        if (success === true) {
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

HomeJunk = function(spawnLocation, spawnRotation) {
    print('HOME CREATE JUNK');
    var created = [];
    function addVelocityDown() {
        print('HOME ADDING DOWN VELOCITY TO SHELF  ITEMS')
        created.forEach(function(obj) {
            Entities.editEntity(obj, {
                velocity: {
                    x: 0,
                    y: -0.1,
                    z: 0
                }
            });
        })
    }
    function create() {
        var success = Clipboard.importEntities(JUNK_URL);
        if (success === true) {
            created = Clipboard.pasteEntities(spawnLocation)
            print('created ' + created);
            addVelocityDown();
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

// Bricabrac = function(spawnLocation, spawnRotation) {
//     print('HOME CREATE BRICABRAC');
//     var created = [];

//     function addVelocityDown() {
//         print('HOME ADDING DOWN VELOCITY TO DRESSING ROOM ITEMS')
//         created.forEach(function(obj) {
//             Entities.editEntity(obj, {
//                 velocity: {
//                     x: 0,
//                     y: -0.1,
//                     z: 0
//                 }
//             });
//         })
//     }

//     function create() {
//         var success = Clipboard.importEntities(BRICABRAC_URL);
//         if (success === true) {
//             created = Clipboard.pasteEntities(spawnLocation)
//             print('created ' + created);
//             addVelocityDown();

//         }
//     };

//     function cleanup() {
//         created.forEach(function(obj) {
//             Entities.deleteEntity(obj);
//         })
//     };

//     create();

//     this.cleanup = cleanup;
// }

Bench = function(spawnLocation, spawnRotation) {
    print('HOME CREATE BENCH');
    var created = [];

    function create() {
        var success = Clipboard.importEntities(BENCH_URL);
        if (success === true) {
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

Umbrella = function(spawnLocation, spawnRotation) {
    print('HOME CREATE Umbrella');
    var created = [];

    function create() {
        var success = Clipboard.importEntities(UMBRELLA_URL);
        if (success === true) {
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