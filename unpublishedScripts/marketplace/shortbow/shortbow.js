//
//  Created by Ryan Huffman on 1/10/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* globals SHORTBOW_ENTITIES:true */

// This is a copy of the data in shortbow.json, which is an export of the shortbow
// scene.
//
// Because .json can't be Script.include'd directly, the contents are copied over
// to here and exposed as a global variable.
//

SHORTBOW_ENTITIES =
{
    "Entities": [
        {
            "clientOnly": 0,
            "collidesWith": "",
            "collisionMask": 0,
            "collisionless": 1,
            "color": {
                "blue": 0,
                "green": 0,
                "red": 255
            },
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 0.24400754272937775,
                "y": 0.24400754272937775,
                "z": 0.24400754272937775
            },
            "id": "{02f39515-cab4-41d5-b315-5fb41613f844}",
            "ignoreForCollisions": 1,
            "lastEdited": 1487892708750446,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "name": "SB.BowSpawn",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": -5.1684012413024902,
                "y": 0.54034698009490967,
                "z": -11.257695198059082
            },
            "queryAACube": {
                "scale": 0.42263346910476685,
                "x": -1.3604279756546021,
                "y": -1803.830078125,
                "z": -27.592727661132812
            },
            "rotation": {
                "w": 0.17366324365139008,
                "x": 4.9033405957743526e-07,
                "y": -0.98480510711669922,
                "z": -2.9563087082351558e-05
            },
            "shape": "Cube",
            "type": "Box",
            "visible": 0
        },
        {
            "backgroundColor": {
                "blue": 65,
                "green": 78,
                "red": 82
            },
            "clientOnly": 0,
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 2,
                "y": 0.69999998807907104,
                "z": 0.0099999997764825821
            },
            "id": "{3eae601e-3c6e-49ab-8f40-dedee32f7573}",
            "lastEdited": 1487894036038423,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "lineHeight": 0.5,
            "name": "SB.DisplayScore",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": -8.0707607269287109,
                "y": 1.5265679359436035,
                "z": -9.5913219451904297
            },
            "queryAACube": {
                "scale": 2.118985652923584,
                "x": -5.1109838485717773,
                "y": -1803.69189453125,
                "z": -26.774648666381836
            },
            "rotation": {
                "w": 0.70708787441253662,
                "x": -1.52587890625e-05,
                "y": 0.70708787441253662,
                "z": -1.52587890625e-05
            },
            "text": "0",
            "type": "Text",
            "userData": "{\"displayType\":\"score\"}"
        },
        {
            "clientOnly": 0,
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 0.71920669078826904,
                "y": 3.3160061836242676,
                "z": 2.2217941284179688
            },
            "id": "{04288f77-64df-4323-ac38-9c1960a393a5}",
            "lastEdited": 1487893058314990,
            "lastEditedBy": "{fce8028a-4bac-43e8-96ff-4c7286ea4ab3}",
            "modelURL": "file:///c:/Users/ryanh/dev/hifi/unpublishedScripts/marketplace/shortbow/models/shortbow-button.baked.fbx",
            "name": "SB.StartButton",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": -9.8358345031738281,
                "y": 0.45674961805343628,
                "z": -13.044205665588379
            },
            "queryAACube": {
                "scale": 4.0558013916015625,
                "x": -7.844393253326416,
                "y": -1805.730224609375,
                "z": -31.195960998535156
            },
            "rotation": {
                "w": 1,
                "x": 1.52587890625e-05,
                "y": 1.52587890625e-05,
                "z": 1.52587890625e-05
            },
            "script": "file:///c:/Users/ryanh/dev/hifi/unpublishedScripts/marketplace/shortbow/startGameButtonClientEntity.js",
            "shapeType": "static-mesh",
            "type": "Model",
            "userData": "{\"grabbableKey\":{\"wantsTrigger\":true}}"
        },
        {
            "backgroundColor": {
                "blue": 65,
                "green": 78,
                "red": 82
            },
            "clientOnly": 0,
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 2,
                "y": 0.69999998807907104,
                "z": 0.0099999997764825821
            },
            "id": "{1196096f-bcc9-4b19-970d-605113474c1b}",
            "lastEdited": 1487894037900323,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "lineHeight": 0.5,
            "name": "SB.DisplayHighScore",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": -8.0707607269287109,
                "y": 0.26189804077148438,
                "z": -9.5913219451904297
            },
            "queryAACube": {
                "scale": 2.118985652923584,
                "x": -5.11102294921875,
                "y": -1804.95654296875,
                "z": -26.77461051940918
            },
            "rotation": {
                "w": 0.70708787441253662,
                "x": -1.52587890625e-05,
                "y": 0.70708787441253662,
                "z": -1.52587890625e-05
            },
            "text": "0",
            "type": "Text",
            "userData": "{\"displayType\":\"highscore\"}"
        },
        {
            "backgroundColor": {
                "blue": 65,
                "green": 78,
                "red": 82
            },
            "clientOnly": 0,
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 1.4120937585830688,
                "y": 0.71569448709487915,
                "z": 0.0099999997764825821
            },
            "id": "{293c294d-1df5-461e-82a3-66abee852d44}",
            "lastEdited": 1487894033695485,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "lineHeight": 0.5,
            "name": "SB.DisplayWave",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": -8.0707607269287109,
                "y": 1.5265679359436035,
                "z": -7.2889409065246582
            },
            "queryAACube": {
                "scale": 1.5831384658813477,
                "x": -4.8431310653686523,
                "y": -1803.4239501953125,
                "z": -24.204343795776367
            },
            "rotation": {
                "w": 0.70708787441253662,
                "x": -1.52587890625e-05,
                "y": 0.70708787441253662,
                "z": -1.52587890625e-05
            },
            "text": "0",
            "type": "Text",
            "userData": "{\"displayType\":\"wave\"}"
        },
        {
            "backgroundColor": {
                "blue": 65,
                "green": 78,
                "red": 82
            },
            "clientOnly": 0,
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 1.4120937585830688,
                "y": 0.71569448709487915,
                "z": 0.0099999997764825821
            },
            "id": "{379afa7b-c668-4c4e-b290-e5c37fb3440c}",
            "lastEdited": 1487893055310428,
            "lastEditedBy": "{fce8028a-4bac-43e8-96ff-4c7286ea4ab3}",
            "lineHeight": 0.5,
            "name": "SB.DisplayLives",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": -8.0707607269287109,
                "y": 0.26189804077148438,
                "z": -7.2889409065246582
            },
            "queryAACube": {
                "scale": 1.5831384658813477,
                "x": -4.8431692123413086,
                "y": -1804.6885986328125,
                "z": -24.204303741455078
            },
            "rotation": {
                "w": 0.70708787441253662,
                "x": -1.52587890625e-05,
                "y": 0.70708787441253662,
                "z": -1.52587890625e-05
            },
            "text": "0",
            "type": "Text",
            "userData": "{\"displayType\":\"lives\"}"
        },
        {
            "clientOnly": 0,
            "collidesWith": "",
            "collisionMask": 0,
            "collisionless": 1,
            "color": {
                "blue": 171,
                "green": 50,
                "red": 62
            },
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 0.24400754272937775,
                "y": 0.24400754272937775,
                "z": 0.24400754272937775
            },
            "id": "{760e81a1-a804-4f5e-9769-393d021fc8fe}",
            "ignoreForCollisions": 1,
            "lastEdited": 1487892440234633,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "name": "SB.EnemySpawn",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": -1.89238440990448,
                "y": -5.3368110656738281,
                "z": 11.512755393981934
            },
            "queryAACube": {
                "scale": 0.42263346910476685,
                "x": 1.9147146940231323,
                "y": -1809.7066650390625,
                "z": -4.8219971656799316
            },
            "rotation": {
                "w": 1,
                "x": -1.52587890625e-05,
                "y": -1.52587890625e-05,
                "z": -1.52587890625e-05
            },
            "shape": "Cube",
            "type": "Box",
            "visible": 0
        },
        {
            "clientOnly": 0,
            "collidesWith": "",
            "collisionMask": 0,
            "collisionless": 1,
            "color": {
                "blue": 171,
                "green": 50,
                "red": 62
            },
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 0.24400754272937775,
                "y": 0.24400754272937775,
                "z": 0.24400754272937775
            },
            "id": "{0a76d0ac-6353-467b-8edc-56417d5a987c}",
            "ignoreForCollisions": 1,
            "lastEdited": 1487892440235124,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "name": "SB.EnemySpawn",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": 3.6569130420684814,
                "y": -5.3365960121154785,
                "z": 10.01292610168457
            },
            "queryAACube": {
                "scale": 0.42263346910476685,
                "x": 7.4640579223632812,
                "y": -1809.7066650390625,
                "z": -6.3216567039489746
            },
            "rotation": {
                "w": 1,
                "x": -1.52587890625e-05,
                "y": -1.52587890625e-05,
                "z": -1.52587890625e-05
            },
            "shape": "Cube",
            "type": "Box",
            "visible": 0
        },
        {
            "clientOnly": 0,
            "collidesWith": "",
            "collisionMask": 0,
            "collisionless": 1,
            "color": {
                "blue": 171,
                "green": 50,
                "red": 62
            },
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 0.24400754272937775,
                "y": 0.24400754272937775,
                "z": 0.24400754272937775
            },
            "id": "{f8549c8a-e646-4feb-bbaf-70e7d5be755a}",
            "ignoreForCollisions": 1,
            "lastEdited": 1487892440235339,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "name": "SB.EnemySpawn",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": 8.8902750015258789,
                "y": -5.3364419937133789,
                "z": 10.195274353027344
            },
            "queryAACube": {
                "scale": 0.42263346910476685,
                "x": 12.697414398193359,
                "y": -1809.7066650390625,
                "z": -6.1391491889953613
            },
            "rotation": {
                "w": 1,
                "x": -1.52587890625e-05,
                "y": -1.52587890625e-05,
                "z": -1.52587890625e-05
            },
            "shape": "Cube",
            "type": "Box",
            "visible": 0
        },
        {
            "clientOnly": 0,
            "collidesWith": "",
            "collisionMask": 0,
            "collisionless": 1,
            "color": {
                "blue": 0,
                "green": 0,
                "red": 255
            },
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 0.24400754272937775,
                "y": 0.24400754272937775,
                "z": 0.24400754272937775
            },
            "id": "{f3aea4ae-4445-4a2d-8d61-e9fd72f04008}",
            "ignoreForCollisions": 1,
            "lastEdited": 1487892708751269,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "name": "SB.BowSpawn",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": -2.5027251243591309,
                "y": 0.54042834043502808,
                "z": -11.257777214050293
            },
            "queryAACube": {
                "scale": 0.42263346910476685,
                "x": 1.3052481412887573,
                "y": -1803.830078125,
                "z": -27.592727661132812
            },
            "rotation": {
                "w": 0.17366324365139008,
                "x": 4.9033405957743526e-07,
                "y": -0.98480510711669922,
                "z": -2.9563087082351558e-05
            },
            "shape": "Cube",
            "type": "Box",
            "visible": 0
        },
        {
            "clientOnly": 0,
            "collidesWith": "",
            "collisionMask": 0,
            "collisionless": 1,
            "color": {
                "blue": 0,
                "green": 0,
                "red": 255
            },
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 0.24400754272937775,
                "y": 0.24400754272937775,
                "z": 0.24400754272937775
            },
            "id": "{cc1ac907-124b-4372-8c4c-82d175546725}",
            "ignoreForCollisions": 1,
            "lastEdited": 1487892708751135,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "name": "SB.BowSpawn",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": 2.7972855567932129,
                "y": 0.54059004783630371,
                "z": -11.257938385009766
            },
            "queryAACube": {
                "scale": 0.42263346910476685,
                "x": 6.6052589416503906,
                "y": -1803.830078125,
                "z": -27.592727661132812
            },
            "rotation": {
                "w": 0.17366324365139008,
                "x": 4.9033405957743526e-07,
                "y": -0.98480510711669922,
                "z": -2.9563087082351558e-05
            },
            "shape": "Cube",
            "type": "Box",
            "visible": 0
        },
        {
            "clientOnly": 0,
            "collidesWith": "",
            "collisionMask": 0,
            "collisionless": 1,
            "color": {
                "blue": 0,
                "green": 0,
                "red": 255
            },
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 0.24400754272937775,
                "y": 0.24400754272937775,
                "z": 0.24400754272937775
            },
            "id": "{e25ce690-e267-4c51-80a0-f63a72474b8a}",
            "ignoreForCollisions": 1,
            "lastEdited": 1487892708751527,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "name": "SB.BowSpawn",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": 0.17114110291004181,
                "y": 0.54050993919372559,
                "z": -11.257858276367188
            },
            "queryAACube": {
                "scale": 0.42263346910476685,
                "x": 3.979114294052124,
                "y": -1803.830078125,
                "z": -27.592727661132812
            },
            "rotation": {
                "w": 0.17366324365139008,
                "x": 4.9033405957743526e-07,
                "y": -0.98480510711669922,
                "z": -2.9563087082351558e-05
            },
            "shape": "Cube",
            "type": "Box",
            "visible": 0
        },
        {
            "clientOnly": 0,
            "collidesWith": "",
            "collisionMask": 0,
            "collisionless": 1,
            "color": {
                "blue": 0,
                "green": 0,
                "red": 255
            },
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 0.24400754272937775,
                "y": 0.24400754272937775,
                "z": 0.24400754272937775
            },
            "id": "{91ee2285-38f8-4795-b6bc-7abc8dcde07c}",
            "ignoreForCollisions": 1,
            "lastEdited": 1487892708750806,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "name": "SB.BowSpawn",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": 5.4656705856323242,
                "y": 0.54067152738571167,
                "z": -11.258020401000977
            },
            "queryAACube": {
                "scale": 0.42263346910476685,
                "x": 9.2736434936523438,
                "y": -1803.830078125,
                "z": -27.592727661132812
            },
            "rotation": {
                "w": 0.17366324365139008,
                "x": 4.9033405957743526e-07,
                "y": -0.98480510711669922,
                "z": -2.9563087082351558e-05
            },
            "shape": "Cube",
            "type": "Box",
            "visible": 0
        },
        {
            "clientOnly": 0,
            "collidesWith": "",
            "collisionMask": 0,
            "collisionless": 1,
            "color": {
                "blue": 0,
                "green": 0,
                "red": 255
            },
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 0.24400754272937775,
                "y": 0.24400754272937775,
                "z": 0.24400754272937775
            },
            "id": "{d81e5fae-8a8d-4186-bbd2-0c3ae737b0f2}",
            "ignoreForCollisions": 1,
            "lastEdited": 1487892552671000,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "name": "SB.BowSpawn",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": 9.6099967956542969,
                "y": 0.64012420177459717,
                "z": -9.9802846908569336
            },
            "queryAACube": {
                "scale": 0.42263346910476685,
                "x": 13.417934417724609,
                "y": -1803.730712890625,
                "z": -26.314868927001953
            },
            "rotation": {
                "w": 0.22495110332965851,
                "x": -2.9734959753113799e-05,
                "y": 0.97437006235122681,
                "z": 2.9735869247815572e-05
            },
            "shape": "Cube",
            "type": "Box",
            "visible": 0
        },
        {
            "clientOnly": 0,
            "collidesWith": "",
            "collisionMask": 0,
            "collisionless": 1,
            "color": {
                "blue": 0,
                "green": 0,
                "red": 255
            },
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 0.24400754272937775,
                "y": 0.24400754272937775,
                "z": 0.24400754272937775
            },
            "id": "{7056e21e-bce6-4c4b-bbca-36bea1dce303}",
            "ignoreForCollisions": 1,
            "lastEdited": 1487892708750993,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "name": "SB.BowSpawn",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": 8.1799373626708984,
                "y": 0.54075431823730469,
                "z": -11.258102416992188
            },
            "queryAACube": {
                "scale": 0.42263346910476685,
                "x": 11.987911224365234,
                "y": -1803.830078125,
                "z": -27.592727661132812
            },
            "rotation": {
                "w": 0.17366324365139008,
                "x": 4.9033405957743526e-07,
                "y": -0.98480510711669922,
                "z": -2.9563087082351558e-05
            },
            "shape": "Cube",
            "type": "Box",
            "visible": 0
        },
        {
            "clientOnly": 0,
            "collidesWith": "",
            "collisionMask": 0,
            "collisionless": 1,
            "color": {
                "blue": 171,
                "green": 50,
                "red": 62
            },
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 0.24400754272937775,
                "y": 0.24400754272937775,
                "z": 0.24400754272937775
            },
            "id": "{ed073620-e304-4b8e-b12a-5371b595bbf6}",
            "ignoreForCollisions": 1,
            "lastEdited": 1487892440234415,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "name": "SB.EnemySpawn",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": -2.3618791103363037,
                "y": -2.0691573619842529,
                "z": 11.254574775695801
            },
            "queryAACube": {
                "scale": 0.42263346910476685,
                "x": 1.4453276395797729,
                "y": -1806.43896484375,
                "z": -5.0802912712097168
            },
            "rotation": {
                "w": 1,
                "x": -1.52587890625e-05,
                "y": -1.52587890625e-05,
                "z": -1.52587890625e-05
            },
            "shape": "Cube",
            "type": "Box",
            "visible": 0
        },
        {
            "clientOnly": 0,
            "collidesWith": "",
            "collisionMask": 0,
            "collisionless": 1,
            "color": {
                "blue": 171,
                "green": 50,
                "red": 62
            },
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 0.24400754272937775,
                "y": 0.24400754272937775,
                "z": 0.24400754272937775
            },
            "id": "{32ed7820-c386-4da1-b676-7e63762861a3}",
            "ignoreForCollisions": 1,
            "lastEdited": 1487892440234854,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "name": "SB.EnemySpawn",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": 0.64757472276687622,
                "y": -2.5217375755310059,
                "z": 10.08248233795166
            },
            "queryAACube": {
                "scale": 0.42263346910476685,
                "x": 4.454803466796875,
                "y": -1806.8917236328125,
                "z": -6.2522788047790527
            },
            "rotation": {
                "w": 1,
                "x": -1.52587890625e-05,
                "y": -1.52587890625e-05,
                "z": -1.52587890625e-05
            },
            "shape": "Cube",
            "type": "Box",
            "visible": 0
        },
        {
            "clientOnly": 0,
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 26.619264602661133,
                "y": 14.24090576171875,
                "z": 39.351066589355469
            },
            "id": "{d4c8f577-944d-4d50-ac85-e56387c0ef0a}",
            "lastEdited": 1487892440231278,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "modelURL": "file:///c:/Users/ryanh/dev/hifi/unpublishedScripts/marketplace/shortbow/models/shortbow-platform.baked.fbx",
            "name": "SB.Platform",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": 0.097909502685070038,
                "y": -1.0163799524307251,
                "z": 2.0321114063262939
            },
            "queryAACube": {
                "scale": 49.597328186035156,
                "x": -20.681917190551758,
                "y": -1829.9739990234375,
                "z": -38.890060424804688
            },
            "rotation": {
                "w": 1,
                "x": -1.52587890625e-05,
                "y": -1.52587890625e-05,
                "z": -1.52587890625e-05
            },
            "shapeType": "static-mesh",
            "type": "Model"
        },
        {
            "clientOnly": 0,
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 23.341892242431641,
                "y": 12.223045349121094,
                "z": 32.012016296386719
            },
            "friction": 1,
            "id": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "lastEdited": 1487892440231832,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "modelURL": "file:///c:/Users/ryanh/dev/hifi/unpublishedScripts/marketplace/shortbow/models/shortbow-scoreboard.baked.fbx",
            "name": "SB.Scoreboard",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "queryAACube": {
                "scale": 41.461017608642578,
                "x": -20.730508804321289,
                "y": -20.730508804321289,
                "z": -20.730508804321289
            },
            "rotation": {
                "w": 1,
                "x": -1.52587890625e-05,
                "y": -1.52587890625e-05,
                "z": -1.52587890625e-05
            },
            "serverScripts": "file:///c:/Users/ryanh/dev/hifi/unpublishedScripts/marketplace/shortbow/shortbowServerEntity.js",
            "shapeType": "static-mesh",
            "type": "Model"
        },
        {
            "clientOnly": 0,
            "color": {
                "blue": 0,
                "green": 0,
                "red": 255
            },
            "created": "2017-02-23T23:28:32Z",
            "dimensions": {
                "x": 15.710711479187012,
                "y": 4.7783288955688477,
                "z": 1.6129581928253174
            },
            "id": "{84cdff6e-a68d-4bbf-8660-2d6a8c2f1fd0}",
            "lastEdited": 1487892440231522,
            "lastEditedBy": "{91f193dd-829a-4b33-ab27-e9a26160634a}",
            "name": "SB.GateCollider",
            "owningAvatarID": "{00000000-0000-0000-0000-000000000000}",
            "parentID": "{0cd1f1f7-53b9-4c15-bf25-42c0760d16f0}",
            "position": {
                "x": 0.31728419661521912,
                "y": -4.3002614974975586,
                "z": -12.531644821166992
            },
            "queryAACube": {
                "scale": 16.50031852722168,
                "x": -3.913693904876709,
                "y": -1816.709716796875,
                "z": -36.905204772949219
            },
            "rotation": {
                "w": 1,
                "x": -1.52587890625e-05,
                "y": -1.52587890625e-05,
                "z": -1.52587890625e-05
            },
            "shape": "Cube",
            "type": "Box",
            "visible": 0
        }
    ],
    "Version": 68
};

// Add LocalPosition to entity data if parent properties are available
var entities = SHORTBOW_ENTITIES.Entities;
var entitiesByID = {};
var i, entity;
for (i = 0; i < entities.length; ++i) {
    entity = entities[i];
    entitiesByID[entity.id] = entity;
}
for (i = 0; i < entities.length; ++i) {
    entity = entities[i];
    if (entity.parentID !== undefined) {
        var parent = entitiesByID[entity.parentID];
        if (parent !== undefined) {
            entity.localPosition = Vec3.subtract(entity.position, parent.position);
            delete entity.position;
        }
    }
}
