//
//  brokenUrlReport.js
//
//  Created by Alezia Kurdis on February 22, 2021.
//  Copyright 2021 Vircadia contributors.
//
//  This script add reports functionalities to the Create Application.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var brokenUrlReportHttpRequest;
var brokenUrlReportUrlList = [];
var brokenUrlReportInvalideUrlList = [];
var brokenUrlReportProcessedUrlNo = 0;
var brokenUrlReportUrlEntry;
var brokenUrlReportMessageBox;
var brokenUrlReportOverlayWebWindow;
var BROKEN_URL_REPORT_YES_BUTTON = 0x4000;
var BROKEN_URL_REPORT_NO_BUTTON = 0x10000;
var MAX_URL_BEFORE_WARNING_FOR_LONG_PROCESS = 20;

function brokenUrlReportRequestUrlValidityCheck(no) {
    brokenUrlReportHttpRequest = new XMLHttpRequest();
    brokenUrlReportHttpRequest.requestComplete.connect(brokenUrlReportGetResponseStatus);
    brokenUrlReportHttpRequest.open("GET", brokenUrlReportUrlList[no].url);
    brokenUrlReportHttpRequest.send();
}

function brokenUrlReportGetResponseStatus() {
    if (brokenUrlReportHttpRequest.status === 0 || brokenUrlReportHttpRequest.status > 299) {
        brokenUrlReportUrlList[brokenUrlReportProcessedUrlNo].validity = brokenUrlReportHttpRequest.status;
        brokenUrlReportInvalideUrlList.push(brokenUrlReportUrlList[brokenUrlReportProcessedUrlNo]);
    }
    brokenUrlReportHttpRequest.requestComplete.disconnect(brokenUrlReportGetResponseStatus);
    brokenUrlReportHttpRequest = null;
    brokenUrlReportProcessedUrlNo = brokenUrlReportProcessedUrlNo + 1;
    if (brokenUrlReportProcessedUrlNo === brokenUrlReportUrlList.length) {
        brokenUrlReportGenerateFormatedReport(brokenUrlReportInvalideUrlList);
        brokenUrlReportUrlList = [];
        brokenUrlReportInvalideUrlList = [];
        brokenUrlReportProcessedUrlNo = 0;
    } else {
        brokenUrlReportRequestUrlValidityCheck(brokenUrlReportProcessedUrlNo);
    }
}

function brokenUrlReportGenerateFormatedReport(brokenUrlReportInvalideUrlList) {
    var brokenUrlReportContent = "";
    if (brokenUrlReportInvalideUrlList.length === 0) {
        brokenUrlReportContent = "<h1>Broken Url Report<br><br>" + brokenUrlReportUrlList.length + " URL tested.<br><font style='color:#00FF00;'>NO ISSUE HAS BEEN FOUND.</font></h1><br><br><br>";
        brokenUrlReportContent = brokenUrlReportContent + "<div style='width: 100%; text-align: left;'><hr><font style='font-size: 11px; font-weight: 500;'><i>This report ignores URL from the Asset Server (atp://), local drive paths or any string not starting by 'http'.</i></font></div>";
        Script.setTimeout(function () {
            brokenUrlReportOverlayWebWindow.emitScriptEvent(brokenUrlReportContent);
        }, 3000);
        return;
    }
    brokenUrlReportContent = "        <h1>Broken Url Report</h1>\n";
    brokenUrlReportContent = brokenUrlReportContent + "        <table>\n";
    brokenUrlReportContent = brokenUrlReportContent + "            <tr>\n";
    brokenUrlReportContent = brokenUrlReportContent + "                <td class='superheader'>&nbsp;</td>\n";
    brokenUrlReportContent = brokenUrlReportContent + "                <td class='superheader' colspan='3'>Entity</td>\n";
    brokenUrlReportContent = brokenUrlReportContent + "                <td class='superheader' colspan='3'>Broken Url</td>\n";
    brokenUrlReportContent = brokenUrlReportContent + "            </tr>\n";
    brokenUrlReportContent = brokenUrlReportContent + "            <tr>\n";
    brokenUrlReportContent = brokenUrlReportContent + "                <td class='header'>No</td>\n";
    brokenUrlReportContent = brokenUrlReportContent + "                <td class='header'>Type</td>\n";
    brokenUrlReportContent = brokenUrlReportContent + "                <td class='header'>Name &amp; ID</td>\n";
    brokenUrlReportContent = brokenUrlReportContent + "                <td class='header'>&nbsp;</td>\n";    
    brokenUrlReportContent = brokenUrlReportContent + "                <td class='header'>Property</td>\n";
    brokenUrlReportContent = brokenUrlReportContent + "                <td class='header'>Status</td>\n";
    brokenUrlReportContent = brokenUrlReportContent + "                <td class='header'>Current URL</td>\n";
    brokenUrlReportContent = brokenUrlReportContent + "            </tr>\n";
    for (var i = 0; i < brokenUrlReportInvalideUrlList.length; i++ ){
        brokenUrlReportContent = brokenUrlReportContent + "            <tr>\n";
        brokenUrlReportContent = brokenUrlReportContent + "                <td style='color: #999999; width: 5%;'>" + (i + 1) + "</td>\n";
        brokenUrlReportContent = brokenUrlReportContent + "                <td style='width: 10%;'>" + brokenUrlReportInvalideUrlList[i].type + "</td>\n";
        brokenUrlReportContent = brokenUrlReportContent + "                <td style='width: 40%;'>" + brokenUrlReportInvalideUrlList[i].name + "<br><font color='#999999'>" + brokenUrlReportInvalideUrlList[i].id + "</font></td>\n";
        brokenUrlReportContent = brokenUrlReportContent + "                <td style='width: 2%;'><a href='' onclick='selectEntity(" + '"' + brokenUrlReportInvalideUrlList[i].id + '"' + "); return false;'>&#9998;</a></td>\n";        
        brokenUrlReportContent = brokenUrlReportContent + "                <td style='color: " + brokenUrlReportGetUrlTypeColor(brokenUrlReportInvalideUrlList[i].urlType) + "; width: 10%;'>" + brokenUrlReportInvalideUrlList[i].urlType + "</td>\n";
        brokenUrlReportContent = brokenUrlReportContent + "                <td style='background-color: #FF0000; color: #FFFFFF; width: 8%;'>" + brokenUrlReportInvalideUrlList[i].validity + "</td>\n";
        brokenUrlReportContent = brokenUrlReportContent + "                <td style='word-wrap: break-word; width:200px;'>" + brokenUrlReportInvalideUrlList[i].url + "</td>\n";
        brokenUrlReportContent = brokenUrlReportContent + "            </tr>\n";
    }
    brokenUrlReportContent = brokenUrlReportContent + "        </table>\n";
    brokenUrlReportContent = brokenUrlReportContent + "<div style='width: 100%; text-align: left;'><br>" + brokenUrlReportUrlList.length + " URL tested.<br><br>";
    brokenUrlReportContent = brokenUrlReportContent + "<hr><font style='font-size: 11px; font-weight: 500;'><i>This report ignores URL from the Asset Server (atp://), local drive paths or any string not starting by 'http'.</i></font></div>";
    
    Script.setTimeout(function () {
        brokenUrlReportOverlayWebWindow.emitScriptEvent(brokenUrlReportContent);
    }, 3000);
}

function brokenUrlReportGetUrlTypeColor(urlType) {
    var hexaColor = "#FFFFFF";
    switch (urlType) {
        case "script":
            hexaColor = "#00FF00";
            break;
        case "serverScripts":
            hexaColor = "#FF00FF";
            break;
        case "imageURL":
            hexaColor = "#00FFFF";
            break;
        case "materialURL":
            hexaColor = "#FF6600";
            break;
        case "modelURL":
            hexaColor = "#FFFF00";
            break;
        case "compoundShapeURL":
            hexaColor = "#6666FF";
            break;
        case "animation.url":
            hexaColor = "#6699FF";
            break;
        case "textures":
            hexaColor = "#FF0066";
            break;
        case "xTextureURL":
            hexaColor = "#0000FF";
            break;
        case "yTextureURL":
            hexaColor = "#009966";
            break;
        case "zTextureURL":
            hexaColor = "#993366";
            break;
        case "font":
            hexaColor = "#FFFFFF";
            break;
        case "sourceUrl":
            hexaColor = "#BBFF00";
            break;
        case "scriptURL":
            hexaColor = "#FFBBBB";
            break;
        case "filterURL":
            hexaColor = "#BBBBFF";
            break;
        case "skybox.url":
            hexaColor = "#BBFFFF";
            break;
        case "ambientLight.ambientURL":
            hexaColor = "#FF3300";
        }
    return hexaColor;
}

function brokenUrlReport(entityIds) {
    if (entityIds.length === 0) {
        audioFeedback.rejection();
        Window.alert("You have nothing selected.");
        return;
    } else {        
        var properties;
        for (var i = 0; i < entityIds.length; i++ ){
            properties = Entities.getEntityProperties(entityIds[i]);    
            if (properties.script.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "script",
                    url: properties.script,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if (properties.serverScripts.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "serverScripts",
                    url: properties.serverScripts,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if (properties.type === "Image" && properties.imageURL.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "imageURL",
                    url: properties.imageURL,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if (properties.type === "Material" && properties.materialURL.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "materialURL",
                    url: properties.materialURL,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if (properties.type === "Model" && properties.modelURL.toLowerCase().startsWith("http")) {   
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "modelURL",
                    url: properties.modelURL,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if ((properties.type === "Zone" || properties.type === "Model" || properties.type === "ParticleEffect") && properties.compoundShapeURL.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "compoundShapeURL",
                    url: properties.compoundShapeURL,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            } 
            if (properties.type === "Model" && properties.animation.url.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "animation.url",
                    url: properties.animation.url,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if (properties.type === "ParticleEffect" && properties.textures.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "textures",
                    url: properties.textures,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if (properties.type === "PolyVox" && properties.xTextureURL.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "xTextureURL",
                    url: properties.xTextureURL,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if (properties.type === "PolyVox" && properties.yTextureURL.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "yTextureURL",
                    url: properties.yTextureURL,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if (properties.type === "PolyVox" && properties.zTextureURL.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "zTextureURL",
                    url: properties.zTextureURL,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if (properties.type === "Text" && properties.font.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "font",
                    url: properties.font,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if (properties.type === "Web" && properties.sourceUrl.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "sourceUrl",
                    url: properties.sourceUrl,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if (properties.type === "Web" && properties.scriptURL.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "scriptURL",
                    url: properties.scriptURL,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if (properties.type === "Zone" && properties.filterURL.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "filterURL",
                    url: properties.filterURL,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if (properties.type === "Zone" && properties.skybox.url.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "skybox.url",
                    url: properties.skybox.url,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
            if (properties.type === "Zone" && properties.ambientLight.ambientURL.toLowerCase().startsWith("http")) {
                brokenUrlReportUrlEntry = {
                    id: entityIds[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "ambientLight.ambientURL",
                    url: properties.ambientLight.ambientURL,
                    validity: "NOT_TESTED"
                };
                brokenUrlReportUrlList.push(brokenUrlReportUrlEntry);
            }
        }
        if (brokenUrlReportUrlList.length === 0) {
            audioFeedback.confirmation();
            Window.alert("No 'http' url has been found within the current selection.");
            return;
        } else {
            if (brokenUrlReportUrlList.length > MAX_URL_BEFORE_WARNING_FOR_LONG_PROCESS) {
                var message = "Number of http URL found: " + brokenUrlReportUrlList.length + "\n The analysis may take time. Do you want to proceed?";
                var answer = Window.confirm(message);
                if (!answer) { 
                    return; 
                }
            }
            if (brokenUrlReportOverlayWebWindow !== undefined) {
                brokenUrlReportOverlayWebWindow.close();
            }
            brokenUrlReportOverlayWebWindow = new OverlayWebWindow({
                title: "Broken Url Report",
                source: Script.resolvePath("brokenUrlReport.html"),
                width: 1000,
                height: 600
            });            
            brokenUrlReportContent = "";
            brokenUrlReportRequestUrlValidityCheck(brokenUrlReportProcessedUrlNo);
        }
    }
    
    brokenUrlReportOverlayWebWindow.webEventReceived.connect(function (message) {
        try {
            var data = JSON.parse(message);
        } catch(e) {
            print("brokenUrlReport.js: Error parsing JSON");
            return;
        }
        if (data.action === "select") {
            selectionManager.setSelections([data.entityID], this);
        }
    });
}
