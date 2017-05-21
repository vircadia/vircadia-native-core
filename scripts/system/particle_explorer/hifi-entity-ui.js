/* global window, document, print, alert, console,setTimeout, clearTimeout, _ $ */

/**
UI Builder V1.0

This can eventually be expanded to all of Edit, for now, starting
with Particles Only.

This is created for the sole purpose of streamliming the bridge, and to simplify
the logic between an inputfield in WebView and Entities in High Fidelity.

We also do not need anything as heavy as jquery or any other platform,
as we are mostly only building for QT (while, all the other JS frameworks usually do alot of polyfilling)

Available Types:

    JSONInputField - Accepts JSON input, once one presses Save, it will be propegated.
    Button- A Button that listens for a custom event as defined by callback
    Boolean - Creates a checkbox that the user can either check or uncheck
    SliderFloat - Creates a slider (with input) that has Float values from min to max.
        Default is min 0, max 1
    SliderInteger - Creates a slider (with input) that has a Integer value from min to max.
        Default is min 1, max 10000
    SliderRadian - Creates a slider (with input)  that has Float values in degrees,
        that are converted to radians. default is min 0, max Math.PI.
    Texture - Creates a Image with an url input field that points to texture.
        If image cannot form, show "cannot find image"
    VecQuaternion - Creates a 3D Vector field that converts to quaternions.
        Checkbox exists to show quaternions instead.
    Color - Create field color button, that when pressed, opens the color picker.
    Vector - Create a 3D Vector field that has one to one correspondence.

The  script will use this structure to build a UI that is connected The
id fields within High Fidelity

This should make editing, and everything related much more simpler to maintain,
and If there is any changes to either the Entities or properties of

**/

var RADIAN = Math.PI/180;
function HifiEntityUI(parent, structure){
    this.parent = parent;
    this.structure = structure;
    var self = this;
    this.webBridgeSync = _.debounce(function(id, val){
        if (self.EventBridge){
            var sendPackage = {};
            sendPackage[id] = val;
            var message = {
                messageType: "settings_update",
                updateSettings: sendPackage
            };
            self.EventBridge.emitWebEvent(JSON.stringify(message));
        }
    }, 125);
}

HifiEntityUI.prototype = {
    connect: function (EventBridge){
        this.EventBridge = EventBridge;
        var self = this;
        EventBridge.emitWebEvent(JSON.stringify({
            messageType: 'page_loaded'
        }));

        EventBridge.scriptEventReceived.connect(function(data){
            data = JSON.parse(data);
            if (data.messageType === 'particle_settings') {
                // Update settings
                var currentProperties = data.currentProperties;
                // Do expected property match with structure;
                Object.keys(currentProperties).forEach(function(value, index) {
                    var property = currentProperties[value];
                    var field = self.builtRows[value];
                    if (field) {

                        var el = document.getElementById(value);

                        console.log(value, property, field, el);
                        if (field.className.indexOf("radian") !== -1) {
                            el.value = property / RADIAN;
                            el.onchange({target: el});
                        } else if (field.className.indexOf("range") !== -1 || field.className.indexOf("texture") !== -1){
                            el.value = property;
                            el.onchange({target: el});
                        } else if (field.className.indexOf("checkbox") !== -1) {
                            if (property){
                                el.setAttribute("checked", property);
                            } else {
                                el.removeAttribute("checked");
                            }
                        } else if (field.className.indexOf("vector-section") !== -1) {
                            if (field.className.indexOf("rgb") !== -1) {
                                var red = document.getElementById(value+"-red");
                                var blue = document.getElementById(value+"-blue");
                                var green = document.getElementById(value+"-green");
                                red.value = property.red;
                                blue.value = property.blue;
                                green.value = property.green;
                                // crashes here.

                            } else if (field.className.indexOf("xyz")) {

                                var x = document.getElementById(value+"-x");
                                var y = document.getElementById(value+"-y");
                                var z = document.getElementById(value+"-z");
                                // crashes here.

                                if (value === "emitOrientation") {

                                } else {
                                    x.value = property.x;
                                    y.value = property.y;
                                    z.value = property.z;
                                }
                            }
                        }
                    }

                });
            } else if (data.messageType === 'particle_close') {
                // Legacy event on particle close. This webview actually gets removed now
            }
        });
    },
    build: function () {
        var self = this;
        var sections = Object.keys(this.structure);
        this.builtRows = {};
        sections.forEach(function(section, index){
            var properties = self.structure[section];
            self.addSection(self.parent,section,properties,index);
        });
    },
    addSection: function(parent, section, properties, index) {
        var self = this;
        var sectionDivHeader = document.createElement("div");
        var title = document.createElement("label");
        var dropDown = document.createElement("span");
        dropDown.className = "arrow";
        sectionDivHeader.className = "section-header";
        title.innerHTML = section;
        sectionDivHeader.appendChild(title);
        sectionDivHeader.appendChild(dropDown);
        var collapsed = index !== 0;

        dropDown.innerHTML = collapsed ? "L": "M";
        sectionDivHeader.setAttribute("collapsed", collapsed);
        parent.appendChild(sectionDivHeader);

        var sectionDivBody = document.createElement("div");
        sectionDivBody.className = "property-group";
        var animationWrapper = document.createElement("div");
        animationWrapper.className = "section-wrap";

        for (var property in properties) {

            var builtRow = self.addElement(animationWrapper, properties[property])
            var id = properties[property].id;
            if (id) {
                self.builtRows[id] = builtRow;
            }
        }
        sectionDivBody.appendChild(animationWrapper);
        parent.appendChild(sectionDivBody);
        _.defer(function() {
            var height = (animationWrapper.clientHeight) + "px";
            if (collapsed) {
                sectionDivBody.className = sectionDivBody.className
                    .replace("visible", "")
                    .replace(/\s{2,}/g, " ");

                sectionDivBody.style.maxHeight = "0px";
            } else {
                sectionDivBody.className += " visible";
                sectionDivBody.style.maxHeight = height;
            }

            sectionDivHeader.onclick = function( ) {
                collapsed = !collapsed;
                if (collapsed) {
                    sectionDivBody.className = sectionDivBody.className
                        .replace("visible", "")
                        .replace(/\s{2,}/g, " ");
                    sectionDivBody.style.maxHeight = "0px";
                } else {
                    sectionDivBody.className += " visible";
                    sectionDivBody.style.maxHeight = height;
                }
                // sectionDivBody.style.display = collapsed ? "none": "block";
                dropDown.innerHTML = collapsed ? "L": "M";
                sectionDivHeader.setAttribute("collapsed", collapsed);
            };
        });
    },
    addLabel: function(parent, group) {
        var label = document.createElement("label");
        label.innerHTML = group.name;
        parent.appendChild(label);
        if (group.unit) {
            var span = document.createElement("span");
            span.innerHTML = group.unit;
            span.className = "unit";
            label.appendChild(span);
        }
        return label;
    },
    addVector: function(parent, group){
        var self = this;
        var inputs = ["x","y","z"];
        var domArray = [];
        parent.id = group.id;
        for (var index in inputs) {
            var element = document.createElement("input");
            if (group.defaultColor) {
                element.value = group.defaultColor[inputs[index]];
            } else if (inputs[index] === "red"){
                element.value = 255;
            } else {
                element.value = 0;
            }
            element.setAttribute("type","number");
            element.className = inputs[index];
            element.id = group.id + "-" + inputs[index];
            element.oninput = function(event) {
                self.webBridgeSync(group.id, {x: domArray[0].value, y: domArray[1].value, z: domArray[2].value});
            };
            element.onchange = element.oninput;
            domArray.push(element);
        }

        this.addLabel(parent, group);
        parent.className += " property  vector-section xyz";

        // Add Tuple and the rest
        var tupleContainer = document.createElement("div");
        tupleContainer.className = "tuple";
        for (var domIndex in domArray) {
            var container = domArray[domIndex];
            var div = document.createElement("div");
            var label = document.createElement("label");
            label.innerHTML = inputs[domIndex] + ":";
            label.setAttribute("for", container.id);
            div.appendChild(container);
            div.appendChild(label);
            tupleContainer.appendChild(div);
        }
        parent.appendChild(tupleContainer);
    },
    addVectorQuaternion: function(parent, group) {
        this.addVector(parent,group);
    },
    addColorPicker: function(parent, group) {
        var self = this;
        var $colPickContainer = $('<div>', {
            id: group.id,
            class: "color-picker"
        });
        var updateColors = function(red, green, blue) {
            $colPickContainer.css('background-color', "rgb(" +
                red + "," +
                green + "," +
                blue + ")");
        };

        var inputs = ["red","green","blue"];
        var domArray = [];

        for (var index in inputs) {
            var element = document.createElement("input");
            if (group.defaultColor) {
                element.value = group.defaultColor[inputs[index]];
            } else if (inputs[index] === "red"){
                element.value = 255;
            } else {
                element.value = 0;
            }
            element.setAttribute("type","number");
            element.className = inputs[index];
            element.setAttribute("min",0);
            element.setAttribute("max",255);
            element.id = group.id + "-" + inputs[index];
            element.oninput = function(event) {
                $colPickContainer.colpickSetColor({r: domArray[0].value, g: domArray[1].value, b: domArray[2].value},
                    true);
            };
            element.onchange = element.oninput;
            domArray.push(element);
        }

        updateColors(domArray[0].value, domArray[1].value, domArray[2].value);

        // Could probably write a custom one for this to completely write out jquery,
        // but for now, using the same as earlier.

        /* Color Picker Logic Here */

        parent.appendChild($colPickContainer[0]);

        $colPickContainer.colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: { r: domArray[0].value, g: domArray[1].value, b: domArray[2].value},
            onChange: function(hsb, hex, rgb, el){
                updateColors(rgb.r, rgb.g, rgb.b);

                domArray[0].value = rgb.r;
                domArray[1].value = rgb.g;
                domArray[2].value = rgb.b;
                self.webBridgeSync(group.id, {red: rgb.r, green: rgb.g, blue: rgb.b} );
            },
            onSubmit: function (hsb, hex, rgb, el) {
                $(el).css('background-color', '#' + hex);
                $(el).colpickHide();
                domArray[0].value = rgb.r;
                domArray[1].value = rgb.g;
                domArray[2].value = rgb.b;
                self.webBridgeSync(group.id, {red: rgb.r, green: rgb.g, blue: rgb.b} );
            }
        });
        var li = document.createElement("li");
        li.className ="cr object color";


        this.addLabel(parent, group);
        parent.className += " property vector-section rgb";

        // Add Tuple and the rest
        var tupleContainer = document.createElement("div");
        tupleContainer.className = "tuple";
        for (var domIndex in domArray) {
            var container = domArray[domIndex];
            var div = document.createElement("div");
            var label = document.createElement("label");
            label.innerHTML = inputs[domIndex] + ":";
            label.setAttribute("for", container.id);
            div.appendChild(container);
            div.appendChild(label);
            tupleContainer.appendChild(div);
        }
        parent.appendChild(tupleContainer);


    },
    addTextureField: function(parent, group) {
        var self = this;
        this.addLabel(parent, group);
        parent.className += " property texture";
        var textureImage = document.createElement("div");
        var textureUrl = document.createElement("input");
        textureUrl.setAttribute("type", "text");
        textureUrl.id = group.id;
        textureImage.className = "texture-image no-texture";
        var image = document.createElement("img");
        var imageLoad = _.debounce(function(url){
            if (url.length > 0) {
                textureImage.className = textureImage.className.replace(' no-texture','');
                image.src = url;
                image.style.display = "block";
            } else {
                image.src = "";
                image.style.display = "none";
                textureImage.className += " no-texture";
            }
            self.webBridgeSync(group.id, url);
        }, 250);

        textureUrl.oninput = function (event) {
            // Add throttle
            var url = event.target.value;
            imageLoad(url);
        };
        textureUrl.onchange = textureUrl.oninput;
        textureImage.appendChild(image);
        parent.appendChild(textureImage);
        parent.appendChild(textureUrl);
    },
    addSlider: function(parent, group) {
        var self = this;
        this.addLabel(parent, group);
        parent.className += " property range";
        var container = document.createElement("div");
        container.className = "slider-wrapper";
        var slider = document.createElement("input");
        slider.setAttribute("type", "range");

        var inputField = document.createElement("input");
        inputField.setAttribute("type", "number");

        container.appendChild(slider);
        container.appendChild(inputField);
        parent.appendChild(container);

        if (group.type === "SliderInteger") {
            inputField.setAttribute("min", group.min !== undefined ? group.min: 0);
            inputField.setAttribute("step", 1);

            slider.setAttribute("min", group.min !== undefined ? group.min: 0);
            slider.setAttribute("max", group.max !== undefined ? group.max: 10000);
            slider.setAttribute("step", 1);

            inputField.oninput = function (event){
                slider.value = event.target.value;
                self.webBridgeSync(group.id, slider.value);
            };
            inputField.onchange = inputField.oninput;
            slider.oninput = function (event){
                inputField.value = event.target.value;
                self.webBridgeSync(group.id, slider.value);
            };

            inputField.id = group.id;
        } else if (group.type === "SliderRadian") {
            slider.setAttribute("min", group.min !== undefined ? group.min: 0);
            slider.setAttribute("max", group.max !== undefined ? group.max: 180);
            slider.setAttribute("step", 1);
            parent.className += " radian";
            inputField.setAttribute("min", (group.min !== undefined ? group.min: 0) );
            inputField.setAttribute("max", (group.max !== undefined ? group.max: 180));

            inputField.oninput = function (event){
                slider.value = event.target.value;
                self.webBridgeSync(group.id, slider.value * RADIAN );
            };
            inputField.onchange = inputField.oninput;

            inputField.id = group.id;
            slider.oninput = function (event){
                if (event.target.value > 0){
                    inputField.value = Math.floor(event.target.value);
                } else {
                    inputField.value = Math.ceil(event.target.value);
                }
                self.webBridgeSync(group.id, slider.value * RADIAN);
            };
            var degrees = document.createElement("label");
            degrees.innerHTML = "&#176;";
            degrees.style.fontSize = "1.4rem";
            degrees.style.display = "inline";
            degrees.style.verticalAlign = "top";
            degrees.style.paddingLeft = "0.4rem";
            container.appendChild(degrees);

        } else {
            // Must then be Float
            inputField.setAttribute("min", group.min !== undefined? group.min: 0);
            slider.setAttribute("step", 0.01);

            slider.setAttribute("min", group.min !== undefined ? group.min: 0);
            slider.setAttribute("max", group.max !== undefined ? group.max: 1);
            slider.setAttribute("step", 0.01);

            inputField.oninput = function (event){
                slider.value = event.target.value;
                self.webBridgeSync(group.id, slider.value);
                // bind web sock update here.
            };
            inputField.onchange = inputField.oninput;
            slider.oninput = function (event){
                inputField.value = event.target.value;
                self.webBridgeSync(group.id, inputField.value);
            };

            inputField.id = group.id;
        }

        // UpdateBinding
    },
    addCheckBox: function (parent,group) {
        var checkBox = document.createElement("input");
        checkBox.setAttribute("type", "checkbox");
        var self = this;
        checkBox.onchange = function(event) {
            self.webBridgeSync(group.id, event.target.checked);
        };
        checkBox.id = group.id;
        parent.appendChild(checkBox);
        var label = this.addLabel(parent, group);
        label.setAttribute("for", checkBox.id);
        parent.className += " property checkbox";
    },
    addElement: function (parent, group) {

        var self = this;
        var property = document.createElement("div");
        property.id = group.id;

        var row = document.createElement("div");
        switch (group.type){
            case "Button":
                var exportSettingsButton = document.createElement("input");
                exportSettingsButton.setAttribute("type", "button");
                exportSettingsButton.className = group.class;
                exportSettingsButton.value = group.name;
                parent.appendChild(exportSettingsButton);
                break;
            case "Row":
                var hr = document.createElement("hr");
                hr.className = "splitter";
                parent.appendChild(hr);
                break;
            case "Boolean":
                self.addCheckBox(row, group);
                parent.appendChild(row);
                break;
            case "SliderFloat":
            case "SliderInteger":
            case "SliderRadian":
                self.addSlider(row, group);
                parent.appendChild(row);
                break;
            case "Texture":
                self.addTextureField(row, group);
                parent.appendChild(row);
                break;
            case "Color":
                self.addColorPicker(row, group);
                parent.appendChild(row);
                break;
            case "Vector":
                self.addVector(row, group);
                parent.appendChild(row);
                break;
            case "VectorQuaternion":
                self.addVectorQuaternion(row, group);
                parent.appendChild(row);
                break;
            default:
                console.log("not defined");
        }
        return row;
    }
};
