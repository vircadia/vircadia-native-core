//
//  createElBoppo.js
//
//  Created by Thijs Wenker on 3/17/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* globals SCRIPT_IMPORT_PROPERTIES */

var WANT_CLEANUP_ON_SCRIPT_ENDING = false;

var getScriptPath = function(localPath) {
    if (this.isCleanupAndSpawnScript) {
        return Script.getExternalPath(Script.ExternalPaths.HF_Content, 'DomainContent/Welcome%20Area/Scripts/boppo/' + localPath);
    }
    return Script.resolvePath(localPath);
};

var getCreatePosition = function() {
    // can either return position defined by resetScript or avatar position
    if (this.isCleanupAndSpawnScript) {
        return SCRIPT_IMPORT_PROPERTIES.rootPosition;
    }
    return Vec3.sum(MyAvatar.position, {x: 1, z: -2});
};

var boxingRing = Entities.addEntity({
    dimensions: {
        x: 4.0584001541137695,
        y: 4.0418000221252441,
        z: 3.0490000247955322
    },
    modelURL: Script.getExternalPath(Script.ExternalPaths.HF_Content, 'DomainContent/Welcome%20Area/production/models/boxingRing/assembled/boppoBoxingRingAssembly.fbx'),
    name: 'Boxing Ring Assembly',
    rotation: {
        w: 0.9996337890625,
        x: -1.52587890625e-05,
        y: -0.026230275630950928,
        z: -4.57763671875e-05
    },
    position: getCreatePosition(),
    scriptTimestamp: 1489612158459,
    serverScripts: getScriptPath('boppoServer.js'),
    shapeType: 'static-mesh',
    type: 'Model',
    userData: JSON.stringify({
        Boppo: {
            type: 'boxingring',
            playTimeSeconds: 15
        }
    })
});

var boppoEntities = [
    {
        dimensions: {
            x: 0.36947935819625854,
            y: 0.25536194443702698,
            z: 0.059455446898937225
        },
        modelURL: Script.getExternalPath(Script.ExternalPaths.HF_Content, 'DomainContent/Welcome%20Area/production/models/boxingGameSign/boppoSignFrame.fbx'),
        parentID: boxingRing,
        localPosition: {
            x: -1.0251024961471558,
            y: 0.51661628484725952,
            z: -1.1176263093948364
        },
        rotation: {
            w: 0.996856689453125,
            x: 0.013321161270141602,
            y: 0.0024566650390625,
            z: 0.078049898147583008
        },
        shapeType: 'box',
        type: 'Model'
    },
    {
        dimensions: {
            x: 0.33255371451377869,
            y: 0.1812121719121933,
            z: 0.0099999997764825821
        },
        lineHeight: 0.125,
        name: 'Boxing Ring - High Score Board',
        parentID: boxingRing,
        localPosition: {
            x: -1.0239436626434326,
            y: 0.52212876081466675,
            z: -1.0971509218215942
        },
        rotation: {
            w: 0.9876401424407959,
            x: 0.013046503067016602,
            y: 0.0012359619140625,
            z: 0.15605401992797852
        },
        text: '0:00',
        textColor: {
            blue: 0,
            green: 0,
            red: 255
        },
        type: 'Text',
        userData: JSON.stringify({
            Boppo: {
                type: 'timer'
            }
        })
    },
    {
        dimensions: {
            x: 0.50491130352020264,
            y: 0.13274604082107544,
            z: 0.0099999997764825821
        },
        lineHeight: 0.090000003576278687,
        name: 'Boxing Ring - Score Board',
        parentID: boxingRing,
        localPosition: {
            x: -0.77596306800842285,
            y: 0.37797555327415466,
            z: -1.0910623073577881
        },
        rotation: {
            w: 0.9518122673034668,
            x: 0.004237703513354063,
            y: -0.0010041374480351806,
            z: 0.30455198884010315
        },
        text: 'SCORE: 0',
        textColor: {
            blue: 0,
            green: 0,
            red: 255
        },
        type: 'Text',
        userData: JSON.stringify({
            Boppo: {
                type: 'score'
            }
        })
    },
    {
        dimensions: {
            x: 0.58153259754180908,
            y: 0.1884911060333252,
            z: 0.059455446898937225
        },
        modelURL: Script.getExternalPath(Script.ExternalPaths.HF_Content, 'DomainContent/Welcome%20Area/production/models/boxingGameSign/boppoSignFrame.fbx'),
        parentID: boxingRing,
        localPosition: {
            x: -0.78200173377990723,
            y: 0.35684797167778015,
            z: -1.108180046081543
        },
        rotation: {
            w: 0.97814905643463135,
            x: 0.0040436983108520508,
            y: -0.0005645751953125,
            z: 0.20778214931488037
        },
        shapeType: 'box',
        type: 'Model'
    },
    {
        dimensions: {
            x: 4.1867804527282715,
            y: 3.5065803527832031,
            z: 5.6845207214355469
        },
        name: 'El Boppo the Clown boxing area & glove maker',
        parentID: boxingRing,
        localPosition: {
            x: -0.012308252975344658,
            y: 0.054641719907522202,
            z: 0.98782551288604736
        },
        rotation: {
            w: 1,
            x: -1.52587890625e-05,
            y: -1.52587890625e-05,
            z: -1.52587890625e-05
        },
        script: getScriptPath('clownGloveDispenser.js'),
        shapeType: 'box',
        type: 'Zone',
        visible: false
    },
    {
        color: {
            blue: 255,
            green: 5,
            red: 255
        },
        dimensions: {
            x: 0.20000000298023224,
            y: 0.20000000298023224,
            z: 0.20000000298023224
        },
        name: 'LookAtBox',
        parentID: boxingRing,
        localPosition: {
            x: -0.1772226095199585,
            y: -1.7072629928588867,
            z: 1.3122396469116211
        },
        rotation: {
            w: 0.999969482421875,
            x: 1.52587890625e-05,
            y: 0.0043793916702270508,
            z: 1.52587890625e-05
        },
        shape: 'Cube',
        type: 'Box',
        userData: JSON.stringify({
            Boppo: {
                type: 'lookAtThis'
            }
        })
    },
    {
        color: {
            blue: 209,
            green: 157,
            red: 209
        },
        dimensions: {
            x: 1.6913000345230103,
            y: 1.2124500274658203,
            z: 0.2572999894618988
        },
        name: 'boppoBackBoard',
        parentID: boxingRing,
        localPosition: {
            x: -0.19500596821308136,
            y: -1.1044719219207764,
            z: -0.55993378162384033
        },
        rotation: {
            w: 0.9807126522064209,
            x: -0.19511711597442627,
            y: 0.0085297822952270508,
            z: 0.0016937255859375
        },
        shape: 'Cube',
        type: 'Box',
        visible: false
    },
    {
        color: {
            blue: 0,
            green: 0,
            red: 255
        },
        dimensions: {
            x: 1.8155574798583984,
            y: 0.92306196689605713,
            z: 0.51203572750091553
        },
        name: 'boppoBackBoard',
        parentID: boxingRing,
        localPosition: {
            x: -0.11036647111177444,
            y: -0.051978692412376404,
            z: -0.79054081439971924
        },
        rotation: {
            w: 0.9807431697845459,
            x: 0.19505608081817627,
            y: 0.0085602998733520508,
            z: -0.0017547607421875
        },
        shape: 'Cube',
        type: 'Box',
        visible: false
    },
    {
        color: {
            blue: 209,
            green: 157,
            red: 209
        },
        dimensions: {
            x: 1.9941408634185791,
            y: 1.2124500274658203,
            z: 0.2572999894618988
        },
        name: 'boppoBackBoard',
        localPosition: {
            x: 0.69560068845748901,
            y: -1.3840068578720093,
            z: 0.059689953923225403
        },
        rotation: {
            w: 0.73458456993103027,
            x: -0.24113833904266357,
            y: -0.56545358896255493,
            z: -0.28734266757965088
        },
        shape: 'Cube',
        type: 'Box',
        visible: false
    },
    {
        color: {
            blue: 82,
            green: 82,
            red: 82
        },
        dimensions: {
            x: 8.3777303695678711,
            y: 0.87573593854904175,
            z: 7.9759469032287598
        },
        parentID: boxingRing,
        localPosition: {
            x: -0.38302639126777649,
            y: -2.121284008026123,
            z: 0.3699878454208374
        },
        rotation: {
            w: 0.70711839199066162,
            x: -7.62939453125e-05,
            y: 0.70705735683441162,
            z: -1.52587890625e-05
        },
        shape: 'Triangle',
        type: 'Shape'
    },
    {
        color: {
            blue: 209,
            green: 157,
            red: 209
        },
        dimensions: {
            x: 1.889795184135437,
            y: 0.86068248748779297,
            z: 0.2572999894618988
        },
        name: 'boppoBackBoard',
        parentID: boxingRing,
        localPosition: {
            x: -0.95167744159698486,
            y: -1.4756947755813599,
            z: -0.042313352227210999
        },
        rotation: {
            w: 0.74004733562469482,
            x: -0.24461740255355835,
            y: 0.56044864654541016,
            z: 0.27998781204223633
        },
        shape: 'Cube',
        type: 'Box',
        visible: false
    },
    {
        color: {
            blue: 0,
            green: 0,
            red: 255
        },
        dimensions: {
            x: 4.0720257759094238,
            y: 0.50657749176025391,
            z: 1.4769613742828369
        },
        name: 'boppo-stepsRamp',
        parentID: boxingRing,
        localPosition: {
            x: -0.002939039608463645,
            y: -1.9770187139511108,
            z: 2.2165381908416748
        },
        rotation: {
            w: 0.99252307415008545,
            x: 0.12184333801269531,
            y: -1.52587890625e-05,
            z: -1.52587890625e-05
        },
        shape: 'Cube',
        type: 'Box',
        visible: false
    },
    {
        color: {
            blue: 150,
            green: 150,
            red: 150
        },
        cutoff: 90,
        dimensions: {
            x: 5.2220535278320312,
            y: 5.2220535278320312,
            z: 5.2220535278320312
        },
        falloffRadius: 2,
        intensity: 15,
        name: 'boxing ring light',
        parentID: boxingRing,
        localPosition: {
            x: -1.4094564914703369,
            y: -0.36021926999092102,
            z: 0.81797939538955688
        },
        rotation: {
            w: 0.9807431697845459,
            x: 1.52587890625e-05,
            y: -0.19520866870880127,
            z: -1.52587890625e-05
        },
        type: 'Light'
    }
];

boppoEntities.forEach(function(entityProperties) {
    entityProperties['parentID'] = boxingRing;
    Entities.addEntity(entityProperties);
});

if (WANT_CLEANUP_ON_SCRIPT_ENDING) {
    Script.scriptEnding.connect(function() {
        Entities.deleteEntity(boxingRing);
    });
}
