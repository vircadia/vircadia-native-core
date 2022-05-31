//  materialAssistant.js
//
//  Created by Alezia Kurdis on May 19th, 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

let maMaterialData = {};

let maClose,
    maName,
    maAlbedoIsActive,
    maAlbedoColorPicker,
    maAlbedoColorPickerSel,
    maAlbedoMapIsActive,
    maAlbedoMap,
    maMetallicIsActive,
    maMetallic,
    maMetallicSlider,
    maMetallicMap,
    maRoughnessIsActive,
    maRoughness,
    maRoughnessSlider,
    maRoughnessMap,
    maNormalMapIsActive,
    maNormalMap,
    maOpacityIsActive,
    maOpacity,
    maOpacitySlider,
    maOpacityMapModeDont,
    maOpacityMapModeMask,
    maOpacityMapModeBlend,
    maOpacityCutoff,
    maOpacityCutoffSlider,
    maEmissiveIsActive,
    maEmissiveColorPicker,
    maEmissiveColorPickerSel,
    maBloom,
    maBloomSlider,
    maUnlit,
    maEmissiveMap,
    maScatteringIsActive,
    maScattering,
    maScatteringSlider,
    maScatteringMap,
    maOcclusionMapIsActive,
    maOcclusionMap,
    maCullFaceModeBack,
    maCullFaceModeFront,
    maCullFaceModeNone;

let DEFAULT_ALBEDO = [1,1,1];
let DEFAULT_EMISSIVE = [0,0,0];
let DEFAULT_BLOOM_FACTOR = 1;
let DEFAULT_ROUGHNESS = 0.9;
let DEFAULT_METALLIC_FOR_MA_UI = 0.001;
let DEFAULT_METALLIC = 0;
let DEFAULT_OPACITY = 1;
let DEFAULT_OPACITY_CUTOFF = 0.5;
let DEFAULT_SCATTERING = 0;

function initiateMaUi() {
    maClose = document.getElementById("uiMaterialAssistant-closeButton");
    maName = document.getElementById("ma-name");
    maAlbedoIsActive = document.getElementById("ma-albedo-isActive");
    maAlbedoColorPicker = document.getElementById("ma-albedo-colorPicker");
    maAlbedoMapIsActive = document.getElementById("ma-albedoMap-isActive");
    maAlbedoMap = document.getElementById("ma-albedoMap");
    maMetallicIsActive = document.getElementById("ma-metallic-isActive");
    maMetallic = document.getElementById("ma-metallic");
    maMetallicSlider = document.getElementById("ma-metallic-slider");
    maMetallicMap = document.getElementById("ma-metallicMap");
    maRoughnessIsActive = document.getElementById("ma-roughness-isActive");
    maRoughness = document.getElementById("ma-roughness");
    maRoughnessSlider = document.getElementById("ma-roughness-slider");
    maRoughnessMap = document.getElementById("ma-roughnessMap");
    maNormalMapIsActive = document.getElementById("ma-normalMap-isActive");
    maNormalMap = document.getElementById("ma-normalMap");
    maOpacityIsActive = document.getElementById("ma-opacity-isActive");
    maOpacity = document.getElementById("ma-opacity");
    maOpacitySlider = document.getElementById("ma-opacity-slider");
    maOpacityMapModeDont = document.getElementById("ma-opacityMapMode-dont");
    maOpacityMapModeMask = document.getElementById("ma-opacityMapMode-mask");
    maOpacityMapModeBlend = document.getElementById("ma-opacityMapMode-blend");
    maOpacityCutoff = document.getElementById("ma-opacityCutoff");
    maOpacityCutoffSlider = document.getElementById("ma-opacityCutoff-slider");
    maEmissiveIsActive = document.getElementById("ma-emissive-isActive");
    maEmissiveColorPicker = document.getElementById("ma-emissive-colorPicker");
    maBloom = document.getElementById("ma-bloom");
    maBloomSlider = document.getElementById("ma-bloom-slider");
    maUnlit = document.getElementById("ma-unlit");
    maEmissiveMap = document.getElementById("ma-emissiveMap");
    maScatteringIsActive = document.getElementById("ma-scattering-isActive");
    maScattering = document.getElementById("ma-scattering");
    maScatteringSlider = document.getElementById("ma-scattering-slider");
    maScatteringMap = document.getElementById("ma-scatteringMap");
    maOcclusionMapIsActive = document.getElementById("ma-occlusionMap-isActive");
    maOcclusionMap = document.getElementById("ma-occlusionMap");
    maCullFaceModeBack = document.getElementById("ma-cullFaceMode-back");
    maCullFaceModeFront = document.getElementById("ma-cullFaceMode-front");
    maCullFaceModeNone = document.getElementById("ma-cullFaceMode-none");
    
    maClose.onclick = function() {
        closeMaterialAssistant();
    };
    maAlbedoIsActive.onclick = function() {
        if (maMaterialData.albedoIsActive) {
            maMaterialData.albedoIsActive = false;
            maAlbedoIsActive.className = "uiMaterialAssistant-inactive";
            maAlbedoColorPicker.style.pointerEvents = 'none';
            maAlbedoColorPicker.style.backgroundColor = "#555555";
        } else {
            maMaterialData.albedoIsActive = true;
            maAlbedoIsActive.className = "uiMaterialAssistant-active";
            maAlbedoColorPicker.style.pointerEvents = 'auto';
            maAlbedoColorPicker.style.backgroundColor = maGetRGB(maMaterialData.albedo);
        }
        maGenerateJsonAndSave();
    };
    maAlbedoMapIsActive.onclick = function() {
        if (maMaterialData.albedoMapIsActive) {
            maMaterialData.albedoMapIsActive = false;
            maAlbedoMapIsActive.className = "uiMaterialAssistant-inactive";
            maAlbedoMap.disabled = true;
        } else {
            maMaterialData.albedoMapIsActive = true;
            maAlbedoMapIsActive.className = "uiMaterialAssistant-active";
            maAlbedoMap.disabled = false;
        }
        maGenerateJsonAndSave();        
    };
    maMetallicIsActive.onclick = function() {
        if (maMaterialData.metallicIsActive) {
            maMaterialData.metallicIsActive = false;
            maMetallicIsActive.className = "uiMaterialAssistant-inactive";
            maMetallic.disabled = true;
            maMetallicSlider.disabled = true;
            maMetallicMap.disabled = true;
        } else {
            maMaterialData.metallicIsActive = true;
            maMetallicIsActive.className = "uiMaterialAssistant-active";
            maMetallic.disabled = false;
            maMetallicSlider.disabled = false;
            maMetallicMap.disabled = false;            
        }
        maGenerateJsonAndSave();  
    };
    maRoughnessIsActive.onclick = function() {
        if (maMaterialData.roughnessIsActive) {
            maMaterialData.roughnessIsActive = false;
            maRoughnessIsActive.className = "uiMaterialAssistant-inactive";
            maRoughness.disabled = true;
            maRoughnessSlider.disabled = true;
            maRoughnessMap.disabled = true;
        } else {
            maMaterialData.roughnessIsActive = true;
            maRoughnessIsActive.className = "uiMaterialAssistant-active";
            maRoughness.disabled = false;
            maRoughnessSlider.disabled = false;
            maRoughnessMap.disabled = false;            
        }
        maGenerateJsonAndSave();  
    };
    maNormalMapIsActive.onclick = function() {
        if (maMaterialData.normalMapIsActive) {
            maMaterialData.normalMapIsActive = false;
            maNormalMapIsActive.className = "uiMaterialAssistant-inactive";
            maNormalMap.disabled = true;
        } else {
            maMaterialData.normalMapIsActive = true;
            maNormalMapIsActive.className = "uiMaterialAssistant-active";
            maNormalMap.disabled = false;
        }
        maGenerateJsonAndSave();
    };
    maOpacityIsActive.onclick = function() {
        if (maMaterialData.opacityIsActive) {
            maMaterialData.opacityIsActive = false;
            maOpacityIsActive.className = "uiMaterialAssistant-inactive";
            maOpacity.disabled = true;
            maOpacitySlider.disabled = true;
            maOpacityMapModeDont.disabled = true;
            maOpacityMapModeMask.disabled = true;
            maOpacityMapModeBlend.disabled = true;
            maOpacityCutoff.disabled = true;
            maOpacityCutoffSlider.disabled = true;
        } else {
            maMaterialData.opacityIsActive = true;
            maOpacityIsActive.className = "uiMaterialAssistant-active";
            maOpacity.disabled = false;
            maOpacitySlider.disabled = false;
            maOpacityMapModeDont.disabled = false;
            maOpacityMapModeMask.disabled = false;
            maOpacityMapModeBlend.disabled = false;
            maOpacityCutoff.disabled = false;
            maOpacityCutoffSlider.disabled = false;
        }
        maGenerateJsonAndSave(); 
    }; 
    maEmissiveIsActive.onclick = function() {
        if (maMaterialData.emissiveIsActive) {
            maMaterialData.emissiveIsActive = false;
            maEmissiveIsActive.className = "uiMaterialAssistant-inactive";
            maBloom.disabled = true;
            maBloomSlider.disabled = true;
            maEmissiveMap.disabled = true;
            maEmissiveColorPicker.style.pointerEvents = 'none';
            maEmissiveColorPicker.style.backgroundColor = "#555555";
        } else {
            maMaterialData.emissiveIsActive = true;
            maEmissiveIsActive.className = "uiMaterialAssistant-active";
            maBloom.disabled = false;
            maBloomSlider.disabled = false;
            maEmissiveMap.disabled = false;
            maEmissiveColorPicker.style.pointerEvents = 'auto';
            maEmissiveColorPicker.style.backgroundColor = maGetRGB(maMaterialData.emissive);
        }
        maGenerateJsonAndSave(); 
    };
    maScatteringIsActive.onclick = function() {
        if (maMaterialData.scatteringIsActive) {
            maMaterialData.scatteringIsActive = false;
            maScatteringIsActive.className = "uiMaterialAssistant-inactive";
            maScattering.disabled = true;
            maScatteringSlider.disabled = true;
            maScatteringMap.disabled = true;
        } else {
            maMaterialData.scatteringIsActive = true;
            maScatteringIsActive.className = "uiMaterialAssistant-active";
            maScattering.disabled = false;
            maScatteringSlider.disabled = false;
            maScatteringMap.disabled = false;            
        }
        maGenerateJsonAndSave(); 
    }; 
    maOcclusionMapIsActive.onclick = function() {
        if (maMaterialData.occlusionMapIsActive) {
            maMaterialData.occlusionMapIsActive = false;
            maOcclusionMapIsActive.className = "uiMaterialAssistant-inactive";
            maOcclusionMap.disabled = true;
        } else {
            maMaterialData.occlusionMapIsActive = true;
            maOcclusionMapIsActive.className = "uiMaterialAssistant-active";
            maOcclusionMap.disabled = false;
        }
        maGenerateJsonAndSave();
    };
    maName.oninput = function() {
        maMaterialData.name = maName.value;
        maGenerateJsonAndSave();        
    };
    maAlbedoMap.oninput = function() {
        maMaterialData.albedoMap = maAlbedoMap.value;
        maGenerateJsonAndSave();   
    };
    maMetallicSlider.oninput = function() {
        maMetallic.value = maMetallicSlider.value/1000;
        maMaterialData.metallic = maMetallic.value;
        maGenerateJsonAndSave();
    };
    maMetallicMap.oninput = function() {
        maMaterialData.metallicMap = maMetallicMap.value;
        maGenerateJsonAndSave();  
    };
    maRoughnessSlider.oninput = function() {
        maRoughness.value = maRoughnessSlider.value/1000;
        maMaterialData.roughness = maRoughness.value;
        maGenerateJsonAndSave();
    };
    maRoughnessMap.oninput = function() {
        maMaterialData.roughnessMap = maRoughnessMap.value;
        maGenerateJsonAndSave();  
    };
    maNormalMap.oninput = function() {
        maMaterialData.normalMap = maNormalMap.value;
        maGenerateJsonAndSave(); 
    };
    maOpacitySlider.oninput = function() {
        maOpacity.value = maOpacitySlider.value/1000;
        maMaterialData.opacity = maOpacity.value;
        maGenerateJsonAndSave();
    };
    maOpacityMapModeDont.oninput = function() {
        maMaterialData.opacityMapMode = maOpacityMapModeDont.value;
        maGenerateJsonAndSave();        
    };
    maOpacityMapModeMask.oninput = function() {
        maMaterialData.opacityMapMode = maOpacityMapModeMask.value;
        maGenerateJsonAndSave();         
    };
    maOpacityMapModeBlend.oninput = function() {
        maMaterialData.opacityMapMode = maOpacityMapModeBlend.value;
        maGenerateJsonAndSave(); 
    };    
    maOpacityCutoffSlider.oninput = function() {
        maOpacityCutoff.value = maOpacityCutoffSlider.value/1000;
        maMaterialData.opacityCutoff = maOpacityCutoff.value;
        maGenerateJsonAndSave();
    };
    maBloomSlider.oninput = function() {
        maBloom.value = maBloomSlider.value/100;
        maMaterialData.bloom = maBloom.value;
        maGenerateJsonAndSave();
    };
    maUnlit.oninput = function() {
        maMaterialData.unlit = maUnlit.checked;
        maGenerateJsonAndSave();
    };
    maEmissiveMap.oninput = function() {
        maMaterialData.emissiveMap = maEmissiveMap.value;
        maGenerateJsonAndSave(); 
    };
    maScatteringSlider.oninput = function() {
        maScattering.value = maScatteringSlider.value/1000;
        maMaterialData.scattering = maScattering.value;
        maGenerateJsonAndSave();
    };
    maScatteringMap.oninput = function() {
        maMaterialData.scatteringMap = maScatteringMap.value;
        maGenerateJsonAndSave(); 
    };
    maOcclusionMap.oninput = function() {
        maMaterialData.occlusionMap = maOcclusionMap.value;
        maGenerateJsonAndSave(); 
    };
    maCullFaceModeBack.oninput = function() {
        maMaterialData.cullFaceMode = maCullFaceModeBack.value;
        maGenerateJsonAndSave(); 
    };
    maCullFaceModeFront.oninput = function() {
        maMaterialData.cullFaceMode = maCullFaceModeFront.value;
        maGenerateJsonAndSave(); 
    };
    maCullFaceModeNone.oninput = function() {
        maMaterialData.cullFaceMode = maCullFaceModeNone.value;
        maGenerateJsonAndSave();
    };
    
    var maAlbedoColorPickerID = "#ma-albedo-colorPicker";
    maAlbedoColorPickerSel = $(maAlbedoColorPickerID).colpick({
        colorScheme: 'dark',
        layout: 'rgbhex',
        color: '000000',
        submit: false,
        onShow: function(colpick) {
            $(maAlbedoColorPickerID).colpickSetColor({
                "r": maMaterialData.albedo[0] * 255,
                "g": maMaterialData.albedo[1] * 255,
                "b": maMaterialData.albedo[2] * 255,
            });
            $(maAlbedoColorPickerID).attr('active', 'true');
        },
        onHide: function(colpick) {
            $(maAlbedoColorPickerID).attr('active', 'false');
        },
        onChange: function(hsb, hex, rgb, el) {
            $(el).css('background-color', '#' + hex);
            if ($(maAlbedoColorPickerID).attr('active') === 'true') {
                maMaterialData.albedo = [(rgb.r/255), (rgb.g/255), (rgb.b/255)];
                maGenerateJsonAndSave();
            }
        }
    });
    
    var maEmissiveColorPickerID = "#ma-emissive-colorPicker";
    maEmissiveColorPickerSel = $(maEmissiveColorPickerID).colpick({
        colorScheme: 'dark',
        layout: 'rgbhex',
        color: '000000',
        submit: false,
        onShow: function(colpick) {
            $(maEmissiveColorPickerID).colpickSetColor({
                "r": maMaterialData.emissive[0] * 255,
                "g": maMaterialData.emissive[1] * 255,
                "b": maMaterialData.emissive[2] * 255,
            });
            $(maEmissiveColorPickerID).attr('active', 'true');
        },
        onHide: function(colpick) {
            $(maEmissiveColorPickerID).attr('active', 'false');
        },
        onChange: function(hsb, hex, rgb, el) {
            $(el).css('background-color', '#' + hex);
            if ($(maEmissiveColorPickerID).attr('active') === 'true') {
                maMaterialData.emissive = [(rgb.r/255), (rgb.g/255), (rgb.b/255)];
                maGenerateJsonAndSave();
            }
        }
    });
}

function loadDataInMaUi(materialDataPropertyValue) {
    var entityMaterialData, materialDefinition;
    try {
        entityMaterialData = JSON.parse(materialDataPropertyValue);
    }
    catch(e) {
        entityMaterialData = {
                "materials":{}
            };
    }

    if (entityMaterialData.materials.length === undefined) {
        materialDefinition = entityMaterialData.materials;
    } else {
        materialDefinition = entityMaterialData.materials[0];
    }

    //MODEL (value other than "hifi_pbr" are not supposed to get the button, so we can assume.)
    maMaterialData.model === "hifi_pbr";

    //NAME
    if (materialDefinition.name !== undefined) {
        maMaterialData.name = materialDefinition.name;
    } else {
        maMaterialData.name = "";
    }
    maName.value = maMaterialData.name;
    
    //ALBEDO
    if (materialDefinition.defaultFallthrough === true && materialDefinition.albedo === undefined) {
        maMaterialData.albedoIsActive = false;
        maAlbedoIsActive.className = "uiMaterialAssistant-inactive";
        maAlbedoColorPicker.style.pointerEvents = 'none';
        maAlbedoColorPicker.style.backgroundColor = "#555555";
    } else {
        maMaterialData.albedoIsActive = true;
        maAlbedoIsActive.className = "uiMaterialAssistant-active";
        maAlbedoColorPicker.style.pointerEvents = 'auto';
        maAlbedoColorPicker.style.backgroundColor = maGetRGB(maMaterialData.albedo);
    }        
    
    if (materialDefinition.albedo !== undefined) {
        maMaterialData.albedo = materialDefinition.albedo;
    } else {
        maMaterialData.albedo = DEFAULT_ALBEDO;
    }
    maAlbedoColorPicker.style.backgroundColor = maGetRGB(maMaterialData.albedo);

    //ALBEDO MAP
    if (materialDefinition.defaultFallthrough === true && materialDefinition.albedoMap === undefined) {
        maMaterialData.albedoMapIsActive = false;
        maAlbedoMapIsActive.className = "uiMaterialAssistant-inactive";
        maAlbedoMap.disabled = true;
    } else {
        maMaterialData.albedoMapIsActive = true;
        maAlbedoMapIsActive.className = "uiMaterialAssistant-active";
        maAlbedoMap.disabled = false;
    } 

    if (materialDefinition.albedoMap !== undefined) {
        maMaterialData.albedoMap = materialDefinition.albedoMap;
    } else {
        maMaterialData.albedoMap = "";
    }
    maAlbedoMap.value = maMaterialData.albedoMap;
    
    //METALLIC
    if (materialDefinition.defaultFallthrough === true 
            && materialDefinition.metallic === undefined 
            && materialDefinition.metallicMap === undefined) {
        maMaterialData.metallicIsActive = false;
        maMetallicIsActive.className = "uiMaterialAssistant-inactive";
        maMetallic.disabled = true;
        maMetallicSlider.disabled = true;
        maMetallicMap.disabled = true;
    } else {
        maMaterialData.metallicIsActive = true;
        maMetallicIsActive.className = "uiMaterialAssistant-active";
        maMetallic.disabled = false;
        maMetallicSlider.disabled = false;
        maMetallicMap.disabled = false;
    } 

    if (materialDefinition.metallic !== undefined) {
        maMaterialData.metallic = materialDefinition.metallic;
    } else {
        maMaterialData.metallic = DEFAULT_METALLIC_FOR_MA_UI;
    }
    maMetallic.value = maMaterialData.metallic;
    maMetallicSlider.value = Math.floor(maMaterialData.metallic * 1000);
    
    if (materialDefinition.metallicMap !== undefined) {
        maMaterialData.metallicMap = materialDefinition.metallicMap;
    } else {
        maMaterialData.metallicMap = "";
    }
    maMetallicMap.value = maMaterialData.metallicMap;

    //ROUGHNESS
    if (materialDefinition.defaultFallthrough === true 
            && materialDefinition.roughness === undefined 
            && materialDefinition.roughnessMap === undefined) {
        maMaterialData.roughnessIsActive = false;
        maRoughnessIsActive.className = "uiMaterialAssistant-inactive";
        maRoughness.disabled = true;
        maRoughnessSlider.disabled = true;
        maRoughnessMap.disabled = true;
    } else {
        maMaterialData.roughnessIsActive = true;
        maRoughnessIsActive.className = "uiMaterialAssistant-active";
        maRoughness.disabled = false;
        maRoughnessSlider.disabled = false;
        maRoughnessMap.disabled = false;
    } 

    if (materialDefinition.roughness !== undefined) {
        maMaterialData.roughness = materialDefinition.roughness;
    } else {
        maMaterialData.roughness = DEFAULT_ROUGHNESS;
    }
    maRoughness.value = maMaterialData.roughness;
    maRoughnessSlider.value = Math.floor(maMaterialData.roughness * 1000);
    
    if (materialDefinition.roughnessMap !== undefined) {
        maMaterialData.roughnessMap = materialDefinition.roughnessMap;
    } else {
        maMaterialData.roughnessMap = "";
    }
    maRoughnessMap.value = maMaterialData.roughnessMap;

    //NORMAL MAP
    if (materialDefinition.defaultFallthrough === true && materialDefinition.normalMap === undefined) {
        maMaterialData.normalMapIsActive = false;
        maNormalMapIsActive.className = "uiMaterialAssistant-inactive";
        maNormalMap.disabled = true;
    } else {
        maMaterialData.normalMapIsActive = true;
        maNormalMapIsActive.className = "uiMaterialAssistant-active";
        maNormalMap.disabled = false;
    }

    if (materialDefinition.normalMap !== undefined) {
        maMaterialData.normalMap = materialDefinition.normalMap;
    } else {
        maMaterialData.normalMap = "";
    }
    maNormalMap.value = maMaterialData.normalMap;

    //OPACITY
    if (materialDefinition.defaultFallthrough === true 
            && materialDefinition.opacity === undefined 
            && materialDefinition.opacityMapMode === undefined) {
        maMaterialData.opacityIsActive = false;
        maOpacityIsActive.className = "uiMaterialAssistant-inactive";
        maOpacity.disabled = true;
        maOpacitySlider.disabled = true;
        maOpacityMapModeDont.disabled = true;
        maOpacityMapModeMask.disabled = true;
        maOpacityMapModeBlend.disabled = true;
        maOpacityCutoff.disabled = true;
        maOpacityCutoffSlider.disabled = true;
    } else {
        maMaterialData.opacityIsActive = true;
        maOpacityIsActive.className = "uiMaterialAssistant-active";
        maOpacity.disabled = false;
        maOpacitySlider.disabled = false;
        maOpacityMapModeDont.disabled = false;
        maOpacityMapModeMask.disabled = false;
        maOpacityMapModeBlend.disabled = false;
        maOpacityCutoff.disabled = false;
        maOpacityCutoffSlider.disabled = false;
    }
    
    if (materialDefinition.opacity !== undefined) {
        maMaterialData.opacity = materialDefinition.opacity;
    } else {
        maMaterialData.opacity = DEFAULT_OPACITY;
    }
    maOpacity.value = maMaterialData.opacity;
    maOpacitySlider.value = Math.floor(maMaterialData.opacity * 1000);

    if (materialDefinition.opacityMapMode !== undefined) {
        maMaterialData.opacityMapMode = materialDefinition.opacityMapMode;
    } else {
        maMaterialData.opacityMapMode = "OPACITY_MAP_OPAQUE";
    }
    switch (maMaterialData.opacityMapMode) {
        case "OPACITY_MAP_OPAQUE":
            maOpacityMapModeDont.checked = true;
            break;
        case "OPACITY_MAP_MASK":
            maOpacityMapModeMask.checked = true;
            break;
        case "OPACITY_MAP_BLEND":
            maOpacityMapModeBlend.checked = true;
            break;
        default:
            alert("ERROR: opacityMapMode = '" + maMaterialData.opacityMapMode + "'. Something has been broken in the code.");
    }

    if (materialDefinition.opacityCutoff !== undefined) {
        maMaterialData.opacityCutoff = materialDefinition.opacityCutoff;
    } else {
        maMaterialData.opacityCutoff = DEFAULT_OPACITY_CUTOFF;
    }
    maOpacityCutoff.value = maMaterialData.opacityCutoff;
    maOpacityCutoffSlider.value = Math.floor(maMaterialData.opacityCutoff * 1000);

    //EMISSIVE
    if (materialDefinition.defaultFallthrough === true 
            && materialDefinition.emissive === undefined 
            && materialDefinition.emissiveMap === undefined) {
        maMaterialData.emissiveIsActive = false;
        maEmissiveIsActive.className = "uiMaterialAssistant-inactive";
        maBloom.disabled = true;
        maBloomSlider.disabled = true;
        maEmissiveMap.disabled = true;
        maEmissiveColorPicker.style.pointerEvents = 'none';
        maEmissiveColorPicker.style.backgroundColor = "#555555";
    } else {
        maMaterialData.emissiveIsActive = true;
        maEmissiveIsActive.className = "uiMaterialAssistant-active";
        maBloom.disabled = false;
        maBloomSlider.disabled = false;
        maEmissiveMap.disabled = false;
        maEmissiveColorPicker.style.pointerEvents = 'auto';
        maEmissiveColorPicker.style.backgroundColor = maGetRGB(maMaterialData.emissive);
    }
    
    if (materialDefinition.emissive !== undefined) {
        maMaterialData.emissive = maGetColorValueFromEmissive(materialDefinition.emissive);
        maMaterialData.bloom = maGetBloomFactorFromEmissive(materialDefinition.emissive);
    } else {
        maMaterialData.emissive = DEFAULT_EMISSIVE;
        maMaterialData.bloom = DEFAULT_BLOOM_FACTOR;
    }
    maEmissiveColorPicker.style.backgroundColor = maGetRGB(maMaterialData.emissive);
    maBloom.value = maMaterialData.bloom;
    maBloomSlider.value = Math.floor(maMaterialData.bloom * 100);
    
    if (materialDefinition.emissiveMap !== undefined) {
        maMaterialData.emissiveMap = materialDefinition.emissiveMap;
    } else {
        maMaterialData.emissiveMap = "";
    }
    maEmissiveMap.value = maMaterialData.emissiveMap;
    
    //UNLIT
    if (materialDefinition.unlit !== undefined) {
        maMaterialData.unlit = materialDefinition.unlit;
    } else {
        maMaterialData.unlit = false;
    }
    maUnlit.checked = maMaterialData.unlit;
    
    //SCATTERING
    if (materialDefinition.defaultFallthrough === true 
            && materialDefinition.scattering === undefined 
            && materialDefinition.scatteringMap === undefined) {
        maMaterialData.scatteringIsActive = false;
        maScatteringIsActive.className = "uiMaterialAssistant-inactive";
        maScattering.disabled = true;
        maScatteringSlider.disabled = true;
        maScatteringMap.disabled = true;
    } else {
        maMaterialData.scatteringIsActive = true;
        maScatteringIsActive.className = "uiMaterialAssistant-active";
        maScattering.disabled = false;
        maScatteringSlider.disabled = false;
        maScatteringMap.disabled = false;
    } 

    if (materialDefinition.scattering !== undefined) {
        maMaterialData.scattering = materialDefinition.scattering;
    } else {
        maMaterialData.scattering = DEFAULT_SCATTERING;
    }
    maScattering.value = maMaterialData.scattering;
    maScatteringSlider.value = Math.floor(maMaterialData.scattering * 1000);
    
    if (materialDefinition.scatteringMap !== undefined) {
        maMaterialData.scatteringMap = materialDefinition.scatteringMap;
    } else {
        maMaterialData.scatteringMap = "";
    }
    maScatteringMap.value = maMaterialData.scatteringMap;

    //OCCLUSION MAP
    if (materialDefinition.defaultFallthrough === true && materialDefinition.occlusionMap === undefined) {
        maMaterialData.occlusionMapIsActive = false;
        maOcclusionMapIsActive.className = "uiMaterialAssistant-inactive";
        maOcclusionMap.disabled = true;
    } else {
        maMaterialData.occlusionMapIsActive = true;
        maOcclusionMapIsActive.className = "uiMaterialAssistant-active";
        maOcclusionMap.disabled = false;
    }

    if (materialDefinition.occlusionMap !== undefined) {
        maMaterialData.occlusionMap = materialDefinition.occlusionMap;
    } else {
        maMaterialData.occlusionMap = "";
    }
    maOcclusionMap.value = maMaterialData.occlusionMap;
    
    //CULL FACE MODE
    if (materialDefinition.cullFaceMode !== undefined) {
        maMaterialData.cullFaceMode = materialDefinition.cullFaceMode;
    } else {
        maMaterialData.cullFaceMode = "CULL_BACK";
    }
    switch (maMaterialData.cullFaceMode) {
        case "CULL_BACK":
            maCullFaceModeBack.checked = true;
            break;
        case "CULL_FRONT":
            maCullFaceModeFront.checked = true;
            break;
        case "CULL_NONE":
            maCullFaceModeNone.checked = true;
            break;
        default:
            alert("ERROR: cullFaceMode = '" + maMaterialData.cullFaceMode + "'. Something has been broken in the code.");
    }
}

function maGenerateJsonAndSave() {
    var newMaterial = {};
    var defaultFallthrough = false;
    
    //NAME
    if (maMaterialData.name != "") {
        newMaterial.name = maMaterialData.name;
    }
    
    //ALBEDO & ALBEDOMAP
    if (maMaterialData.albedoIsActive) {
        newMaterial.albedo = maMaterialData.albedo;
        if (maMaterialData.albedoMap !== "") {
            newMaterial.albedoMap = maMaterialData.albedoMap;
        }
    } else {
        defaultFallthrough = true;
    }
    
    //METALLIC & METALLICMAP
    if (maMaterialData.metallicIsActive) {
        if (maMaterialData.metallicMap === "") {
            newMaterial.metallic = maMaterialData.metallic;
        } else {
            newMaterial.metallicMap = maMaterialData.metallicMap;
        }
    } else {
        defaultFallthrough = true;
    }
    
    //ROUGHNESS & ROUGHNESSMAP
    if (maMaterialData.roughnessIsActive) {
        if (maMaterialData.roughnessMap === "") {
            newMaterial.roughness = maMaterialData.roughness;
        } else {
            newMaterial.roughnessMap = maMaterialData.roughnessMap;
        }
    } else {
        defaultFallthrough = true;
    }

    //NORMAL MAP
    if (maMaterialData.normalMapIsActive) {
        if (maMaterialData.normalMap !== "") {
            newMaterial.normalMap = maMaterialData.normalMap;
        }
    } else {
        defaultFallthrough = true;
    }

    //OPACITY
    if (maMaterialData.opacityIsActive) {
        switch (maMaterialData.opacityMapMode) {
            case "OPACITY_MAP_OPAQUE":
                newMaterial.opacity = maMaterialData.opacity;
                break;
            case "OPACITY_MAP_MASK":
                newMaterial.opacityMapMode = maMaterialData.opacityMapMode;
                newMaterial.opacityMap = maMaterialData.albedoMap;
                newMaterial.opacityCutoff = maMaterialData.opacityCutoff;
                break;
            case "OPACITY_MAP_BLEND":
                newMaterial.opacityMapMode = maMaterialData.opacityMapMode;
                newMaterial.opacityMap = maMaterialData.albedoMap;
                break;
            default:
                alert("ERROR: opacityMapMode = '" + maMaterialData.opacityMapMode + "'. Something has been broken in the code.");
        }
    } else {
        defaultFallthrough = true;
    }    

    //EMISSIVE
    if (maMaterialData.emissiveIsActive) {
        if (maMaterialData.emissiveMap === "") {
            newMaterial.emissive = scaleEmissiveByBloomFactor(maMaterialData.emissive, maMaterialData.bloom);
        } else {
            newMaterial.emissiveMap = maMaterialData.emissiveMap;
        }        
    } else {
        defaultFallthrough = true;
    } 

    //UNLIT
    if (maMaterialData.unlit) {
        newMaterial.unlit = maMaterialData.unlit;
    }

    //SCATTERING
    if (maMaterialData.scatteringIsActive) {
        if (maMaterialData.scatteringMap === "") {
            newMaterial.scattering = maMaterialData.scattering;
        } else {
            newMaterial.scatteringMap = maMaterialData.scatteringMap;
        }
    } else {
        defaultFallthrough = true;
    }
    
    //OCCLUSION MAP
    if (maMaterialData.occlusionMapIsActive) {
        if (maMaterialData.occlusionMap !== "") {
            newMaterial.occlusionMap = maMaterialData.occlusionMap;
        }
    } else {
        defaultFallthrough = true;
    }

    //CULL FACE MODE
    newMaterial.cullFaceMode = maMaterialData.cullFaceMode;
    
    //MODEL
    newMaterial.model = maMaterialData.model;
    
    //defaultFallthrough
    if (defaultFallthrough) {
        newMaterial.defaultFallthrough = true;
    }
    
    //insert newMaterial to materialData
    var materialDataForUpdate = {
            "materialVersion": 1,
            "materials": []
        };
    materialDataForUpdate.materials.push(newMaterial);
    
    //save to property
    EventBridge.emitWebEvent(
            JSON.stringify({
                ids: [...selectedEntityIDs],
                type: "saveMaterialData",
                properties: {
                    materialData: JSON.stringify(materialDataForUpdate)
                }
            })
        );
    materialEditor.set(materialDataForUpdate);
}

function maGetColorValueFromEmissive(colorArray) {
    if (Array.isArray(colorArray)) {
        var max = maGetHighestValue(colorArray);
        if (max > 1) {
            var normalizer = 1/max;
            return [colorArray[0] * normalizer, colorArray[1] * normalizer, colorArray[2] * normalizer];
        } else {
            return colorArray;
        }
    } else {
        return [0,0,0];
    }
}

function maGetBloomFactorFromEmissive(colorArray) {
    if (Array.isArray(colorArray)) {
        return maGetHighestValue(colorArray);
    } else {
        return 1;
    }    
}

function maGetHighestValue(colorArray) {
    var highest = colorArray[0];
    for (var i = 0; i < colorArray.length; i++) {
        if (colorArray[i] > highest) {
            highest = colorArray[i];
        }        
    }
    return highest;
}

function scaleEmissiveByBloomFactor(colorArray, bloomFactor) {
    return [colorArray[0] * bloomFactor, colorArray[1] * bloomFactor, colorArray[2] * bloomFactor];
}

function maGetRGB(colorArray){
    if (colorArray === undefined) {
        return "#000000";
    } else {
        return "rgb(" + Math.floor(colorArray[0] * 255) + ", " + Math.floor(colorArray[1] * 255) + ", " + Math.floor(colorArray[2] * 255) + ")";
    }
}

/**
 * @param {string or object} materialData - json of the materialData as a string or as an object.
 */
function maGetMaterialDataAssistantAvailability(materialData) {
    var materialDataJson, materialDataString;
    if (typeof materialData === "string") {
        materialDataJson = JSON.parse(materialData);
        materialDataString = materialData;
    } else {
        materialDataJson = materialData;
        materialDataString = JSON.stringify(materialData);
    }
    if (getPropertyInputElement("materialURL").value === "materialData" && materialDataString.indexOf("hifi_shader_simple") === -1 &&
            materialDataString.indexOf("glossMap") === -1 && materialDataString.indexOf("specularMap") === -1 && 
            materialDataString.indexOf("bumpMap") === -1 && materialDataString.indexOf("lightMap") === -1 && 
            materialDataString.indexOf("texCoordTransform0") === -1 && materialDataString.indexOf("texCoordTransform1") === -1 &&
            (materialDataJson.materials === undefined || materialDataJson.materials.length <= 1 || typeof materialDataJson.materials === "object")) {
        showMaterialAssistantButton();
    } else {
        hideMaterialAssistantButton();
    }
}
