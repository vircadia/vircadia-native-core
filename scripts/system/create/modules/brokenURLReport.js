//
//  brokenURLReport.js
//
//  Created by Alezia Kurdis on February 22, 2021.
//  Copyright 2021 Vircadia contributors.
//
//  This script reports broken URLs to the Create Application.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var brokenURLReportHttpRequest;
var brokenURLReportUrlList = [];
var brokenURLReportInvalideUrlList = [];
var brokenURLReportProcessedUrlNo = 0;
var brokenURLReportUrlEntry;
var brokenURLReportMessageBox;
var brokenURLReportOverlayWebWindow;
var BROKEN_URL_REPORT_YES_BUTTON = 0x4000;
var BROKEN_URL_REPORT_NO_BUTTON = 0x10000;
var MAX_URL_BEFORE_WARNING_FOR_LONG_PROCESS = 20;

function brokenURLReportRequestUrlValidityCheck(no) {
    brokenURLReportHttpRequest = new XMLHttpRequest();
    brokenURLReportHttpRequest.requestComplete.connect(brokenURLReportGetResponseStatus);
    brokenURLReportHttpRequest.open("GET", brokenURLReportUrlList[no].url);
    brokenURLReportHttpRequest.send();
}

function brokenURLReportGetResponseStatus() {
    if (brokenURLReportHttpRequest.status === 0 || brokenURLReportHttpRequest.status > 299) {
        if (brokenURLReportHttpRequest.status === 0) {
            brokenURLReportUrlList[brokenURLReportProcessedUrlNo].validity = "0 - Server not found";
        } else {
            brokenURLReportUrlList[brokenURLReportProcessedUrlNo].validity = brokenURLReportHttpRequest.status + " - " + brokenURLReportHttpRequest.statusText;
        }
        brokenURLReportInvalideUrlList.push(brokenURLReportUrlList[brokenURLReportProcessedUrlNo]);
    }
    brokenURLReportHttpRequest.requestComplete.disconnect(brokenURLReportGetResponseStatus);
    brokenURLReportHttpRequest = null;
    brokenURLReportProcessedUrlNo = brokenURLReportProcessedUrlNo + 1;
    if (brokenURLReportProcessedUrlNo === brokenURLReportUrlList.length) {
        brokenURLReportGenerateFormatedReport(brokenURLReportInvalideUrlList);
        brokenURLReportUrlList = [];
        brokenURLReportInvalideUrlList = [];
        brokenURLReportProcessedUrlNo = 0;
    } else {
        brokenURLReportRequestUrlValidityCheck(brokenURLReportProcessedUrlNo);
    }
}

function brokenURLReportGenerateFormatedReport(brokenURLReportInvalideUrlList) {
    var brokenURLReportContent = "";
    if (brokenURLReportInvalideUrlList.length === 0) {
        brokenURLReportContent = "<h1>Broken URL Report<br><br>" + brokenURLReportUrlList.length + " URL tested.<br><font style='color:#00FF00;'>NO ISSUES HAVE BEEN FOUND.</font></h1><br><br><br>";
        brokenURLReportContent += "<div style='width: 100%; text-align: left;'><hr><font style='font-size: 11px; font-weight: 500;'><i>This report ignores Asset Server URLs (atp://), local drive paths, and any string not starting with 'http'.</i></font></div>";
        Script.setTimeout(function () {
            brokenURLReportOverlayWebWindow.emitScriptEvent(brokenURLReportContent);
        }, 3000);
        return;
    }
    brokenURLReportContent = "        <h1>Broken URL Report</h1>\n";
    brokenURLReportContent += "        <table>\n";
    brokenURLReportContent += "            <tr>\n";
    brokenURLReportContent += "                <td class='superheader'>&nbsp;</td>\n";
    brokenURLReportContent += "                <td class='superheader' colspan='3'>Entity</td>\n";
    brokenURLReportContent += "                <td class='superheader' colspan='3'>Broken Url</td>\n";
    brokenURLReportContent += "            </tr>\n";
    brokenURLReportContent += "            <tr>\n";
    brokenURLReportContent += "                <td class='header'>No</td>\n";
    brokenURLReportContent += "                <td class='header'>Type</td>\n";
    brokenURLReportContent += "                <td class='header'>Name &amp; ID</td>\n";
    brokenURLReportContent += "                <td class='header'>&nbsp;</td>\n";    
    brokenURLReportContent += "                <td class='header'>Property</td>\n";
    brokenURLReportContent += "                <td class='header'>Status</td>\n";
    brokenURLReportContent += "                <td class='header'>Current URL</td>\n";
    brokenURLReportContent += "            </tr>\n";
    for (var i = 0; i < brokenURLReportInvalideUrlList.length; i++ ){
        brokenURLReportContent += "            <tr>\n";
        brokenURLReportContent += "                <td style='color: #999999; width: 5%;'>" + (i + 1) + "</td>\n";
        brokenURLReportContent += "                <td style='width: 10%;'>" + brokenURLReportInvalideUrlList[i].type + "</td>\n";
        brokenURLReportContent += "                <td style='width: 40%;'>" + brokenURLReportInvalideUrlList[i].name + "<br><font color='#999999'>" + brokenURLReportInvalideUrlList[i].id + "</font></td>\n";
        brokenURLReportContent += "                <td style='width: 2%;'><a href='' onclick='selectEntity(" + '"' + brokenURLReportInvalideUrlList[i].id + '"' + "); return false;'>&#9998;</a></td>\n";        
        brokenURLReportContent += "                <td style='color: " + brokenURLReportGetUrlTypeColor(brokenURLReportInvalideUrlList[i].urlType) + "; width: 10%;'>" + brokenURLReportInvalideUrlList[i].urlType + "</td>\n";
        brokenURLReportContent += "                <td style='background-color: #FF0000; color: #FFFFFF; width: 8%;'>" + brokenURLReportInvalideUrlList[i].validity + "</td>\n";
        brokenURLReportContent += "                <td style='word-wrap: break-word; width:200px;'>" + brokenURLReportInvalideUrlList[i].url + "</td>\n";
        brokenURLReportContent += "            </tr>\n";
    }
    brokenURLReportContent += "        </table>\n";
    brokenURLReportContent += "<div style='width: 100%; text-align: left;'><br>" + brokenURLReportUrlList.length + " URL tested.<br><br>";
    brokenURLReportContent += "<hr><font style='font-size: 11px; font-weight: 500;'><i>This report ignores Asset Server URLs (atp://), local drive paths, and any string not starting with 'http'.</i></font></div>";
    
    Script.setTimeout(function () {
        brokenURLReportOverlayWebWindow.emitScriptEvent(brokenURLReportContent);
    }, 3000);
}

function brokenURLReportGetUrlTypeColor(urlType) {
    var color = "#FFFFFF";
    switch (urlType) {
        case "script":
            color = "#00FF00";
            break;
        case "serverScripts":
            color = "#FF00FF";
            break;
        case "imageURL":
            color = "#00FFFF";
            break;
        case "materialURL":
            color = "#FF6600";
            break;
        case "modelURL":
            color = "#FFFF00";
            break;
        case "compoundShapeURL":
            color = "#6666FF";
            break;
        case "animation.url":
            color = "#6699FF";
            break;
        case "textures":
            color = "#FF0066";
            break;
        case "xTextureURL":
            color = "#0000FF";
            break;
        case "yTextureURL":
            color = "#009966";
            break;
        case "zTextureURL":
            color = "#993366";
            break;
        case "font":
            color = "#FFFFFF";
            break;
        case "sourceUrl":
            color = "#BBFF00";
            break;
        case "scriptURL":
            color = "#FFBBBB";
            break;
        case "filterURL":
            color = "#BBBBFF";
            break;
        case "skybox.url":
            color = "#BBFFFF";
            break;
        case "ambientLight.ambientURL":
            color = "#FF3300";
        }
    return color;
}

function brokenURLReport(entityIDs) {
    if (entityIDs.length === 0) {
        audioFeedback.rejection();
        Window.alert("You have nothing selected.");
        return;
    } else {        
        var properties;
        for (var i = 0; i < entityIDs.length; i++ ){
            properties = Entities.getEntityProperties(entityIDs[i]);    
            if (properties.script.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "script",
                    url: properties.script,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (properties.serverScripts.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "serverScripts",
                    url: properties.serverScripts,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (properties.type === "Image" && properties.imageURL.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "imageURL",
                    url: properties.imageURL,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (properties.type === "Material" && properties.materialURL.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "materialURL",
                    url: properties.materialURL,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (properties.type === "Model" && properties.modelURL.toLowerCase().startsWith("http")) {   
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "modelURL",
                    url: properties.modelURL,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (
                (properties.type === "Zone" || properties.type === "Model" || properties.type === "ParticleEffect") 
                && properties.compoundShapeURL.toLowerCase().startsWith("http")
            ) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "compoundShapeURL",
                    url: properties.compoundShapeURL,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            } 
            if (properties.type === "Model" && properties.animation.url.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "animation.url",
                    url: properties.animation.url,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (properties.type === "ParticleEffect" && properties.textures.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "textures",
                    url: properties.textures,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (properties.type === "PolyVox" && properties.xTextureURL.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "xTextureURL",
                    url: properties.xTextureURL,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (properties.type === "PolyVox" && properties.yTextureURL.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "yTextureURL",
                    url: properties.yTextureURL,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (properties.type === "PolyVox" && properties.zTextureURL.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "zTextureURL",
                    url: properties.zTextureURL,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (properties.type === "Text" && properties.font.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "font",
                    url: properties.font,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (properties.type === "Web" && properties.sourceUrl.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "sourceUrl",
                    url: properties.sourceUrl,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (properties.type === "Web" && properties.scriptURL.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "scriptURL",
                    url: properties.scriptURL,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (properties.type === "Zone" && properties.filterURL.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "filterURL",
                    url: properties.filterURL,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (properties.type === "Zone" && properties.skybox.url.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "skybox.url",
                    url: properties.skybox.url,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
            if (properties.type === "Zone" && properties.ambientLight.ambientURL.toLowerCase().startsWith("http")) {
                brokenURLReportUrlEntry = {
                    id: entityIDs[i],
                    name: properties.name,
                    type: properties.type,
                    urlType: "ambientLight.ambientURL",
                    url: properties.ambientLight.ambientURL,
                    validity: "NOT_TESTED"
                };
                brokenURLReportUrlList.push(brokenURLReportUrlEntry);
            }
        }
        if (brokenURLReportUrlList.length === 0) {
            audioFeedback.confirmation();
            Window.alert("No 'http' URL has been found within the current selection.");
            return;
        } else {
            if (brokenURLReportUrlList.length > MAX_URL_BEFORE_WARNING_FOR_LONG_PROCESS) {
                var message = "Number of http URLs found: " + brokenURLReportUrlList.length + "\n The analysis may take time. Do you want to proceed?";
                var answer = Window.confirm(message);
                if (!answer) { 
                    return; 
                }
            }
            if (brokenURLReportOverlayWebWindow !== undefined) {
                brokenURLReportOverlayWebWindow.close();
            }
            brokenURLReportOverlayWebWindow = new OverlayWebWindow({
                title: "Broken URL Report",
                source: Script.resolvePath("brokenURLReport.html"),
                width: 1000,
                height: 600
            });            
            brokenURLReportContent = "";
            brokenURLReportRequestUrlValidityCheck(brokenURLReportProcessedUrlNo);
        }
    }
    
    brokenURLReportOverlayWebWindow.webEventReceived.connect(function (message) {
        try {
            var data = JSON.parse(message);
        } catch(e) {
            print("brokenURLReport.js: Error parsing JSON");
            return;
        }
        if (data.action === "select") {
            selectionManager.setSelections([data.entityID], this);
        }
    });
}
