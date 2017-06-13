/* global window, document, print, alert, console,setTimeout, clearTimeout, _ $ */
/* eslint no-console: 0 */

/**
UI Builder V1.0

Created by Matti 'Menithal' Lahtinen
24/5/2017
Copyright 2017 High Fidelity, Inc.

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

var RADIANS_PER_DEGREE = Math.PI / 180;

var roundFloat = function (input, round) {
    round = round ? round : 1000;
    var sanitizedInput;
    if (typeof input === "string") {
        sanitizedInput = parseFloat(input);
    } else {
        sanitizedInput = input;
    }
    return Math.round(sanitizedInput * round) / round;
};

function HifiEntityUI(parent) {
    this.parent = parent;

    var self = this;
    this.webBridgeSync = _.debounce(function (id, val) {
        if (self.EventBridge) {
            var sendPackage = {};
            sendPackage[id] = val;
            self.submitChanges(sendPackage);
        }
    }, 125);
}

HifiEntityUI.prototype = {
    setOnSelect: function (callback) {
        this.onSelect = callback;
    },
    submitChanges: function (structure) {
        var message = {
            messageType: "settings_update",
            updatedSettings: structure
        };
        this.EventBridge.emitWebEvent(JSON.stringify(message));
    },
    setUI: function (structure) {
        this.structure = structure;
    },
    disableFields: function () {
        var fields = document.getElementsByTagName("input");
        for (var i = 0; i < fields.length; i++) {
            if (fields[i].getAttribute("type") !== "button") {
                fields[i].value = "";
            }

            fields[i].setAttribute("disabled", true);
        }
        var textures = document.getElementsByTagName("img");
        for (i = 0; i < textures.length; i++) {
            textures[i].src = "";
        }

        textures = document.getElementsByClassName("with-texture");
        for (i = 0; i < textures.length; i++) {
            textures[i].classList.remove("with-textures");
            textures[i].classList.add("no-texture");
        }

        var textareas = document.getElementsByTagName("textarea");
        for (var x = 0; x < textareas.length; x++) {
            textareas[x].remove();
        }
    },
    getSettings: function () {
        var self = this;
        var json = {};
        var keys = Object.keys(self.builtRows);

        for (var i = 0; i < keys.length; i++) {
            var key = keys[i];
            var el = self.builtRows[key];
            if (el.className.indexOf("checkbox") !== -1) {
                json[key] = document.getElementById(key)
                    .checked ? true : false;
            } else if (el.className.indexOf("vector-section") !== -1) {
                var vector = {};
                if (el.className.indexOf("rgb") !== -1) {
                    var red = document.getElementById(key + "-red");
                    var blue = document.getElementById(key + "-blue");
                    var green = document.getElementById(key + "-green");
                    vector.red = red.value;
                    vector.blue = blue.value;
                    vector.green = green.value;
                } else if (el.className.indexOf("pyr") !== -1) {
                    var p = document.getElementById(key + "-Pitch");
                    var y = document.getElementById(key + "-Yaw");
                    var r = document.getElementById(key + "-Roll");
                    vector.x = p.value;
                    vector.y = y.value;
                    vector.z = r.value;
                } else {
                    var x = document.getElementById(key + "-x");
                    var ey = document.getElementById(key + "-y");
                    var z = document.getElementById(key + "-z");
                    vector.x = x.value;
                    vector.y = ey.value;
                    vector.z = z.value;
                }
                json[key] = vector;
            } else if (el.className.length > 0) {
                json[key] = document.getElementById(key)
                    .value;
            }
        }

        return json;
    },
    fillFields: function (currentProperties) {
        var self = this;
        var fields = document.getElementsByTagName("input");

        if (!currentProperties.locked) {
            for (var i = 0; i < fields.length; i++) {
                fields[i].removeAttribute("disabled");
            }
        }
        if (self.onSelect) {
            self.onSelect();
        }
        var keys = Object.keys(currentProperties);


        for (var e in keys) {
            if (keys.hasOwnProperty(e)) {
                var value = keys[e];

                var property = currentProperties[value];
                var field = self.builtRows[value];
                if (field) {
                    var el = document.getElementById(value);

                    if (field.className.indexOf("radian") !== -1) {
                        el.value = property / RADIANS_PER_DEGREE;
                        el.onchange({
                            target: el
                        });
                    } else if (field.className.indexOf("range") !== -1 || field.className.indexOf("texture") !== -1) {
                        el.value = property;
                        el.onchange({
                            target: el
                        });
                    } else if (field.className.indexOf("checkbox") !== -1) {
                        if (property) {
                            el.setAttribute("checked", property);
                        } else {
                            el.removeAttribute("checked");
                        }
                    } else if (field.className.indexOf("vector-section") !== -1) {
                        if (field.className.indexOf("rgb") !== -1) {
                            var red = document.getElementById(value + "-red");
                            var blue = document.getElementById(value + "-blue");
                            var green = document.getElementById(value + "-green");
                            red.value = parseInt(property.red);
                            blue.value = parseInt(property.blue);
                            green.value = parseInt(property.green);

                            red.oninput({
                                target: red
                            });
                        } else if (field.className.indexOf("xyz") !== -1) {
                            var x = document.getElementById(value + "-x");
                            var y = document.getElementById(value + "-y");
                            var z = document.getElementById(value + "-z");

                            x.value = roundFloat(property.x, 100);
                            y.value = roundFloat(property.y, 100);
                            z.value = roundFloat(property.z, 100);
                        } else if (field.className.indexOf("pyr") !== -1) {
                            var pitch = document.getElementById(value + "-Pitch");
                            var yaw = document.getElementById(value + "-Yaw");
                            var roll = document.getElementById(value + "-Roll");

                            pitch.value = roundFloat(property.x, 100);
                            yaw.value = roundFloat(property.y, 100);
                            roll.value = roundFloat(property.z, 100);

                        }
                    }
                }
            }
        }
    },
    connect: function (EventBridge) {
        this.EventBridge = EventBridge;

        var self = this;

        EventBridge.emitWebEvent(JSON.stringify({
            messageType: 'page_loaded'
        }));

        EventBridge.scriptEventReceived.connect(function (data) {
            data = JSON.parse(data);

            if (data.messageType === 'particle_settings') {
                // Update settings
                var currentProperties = data.currentProperties;
                self.fillFields(currentProperties);
                // Do expected property match with structure;
            } else if (data.messageType === 'particle_close') {
                self.disableFields();
            }
        });
    },
    build: function () {
        var self = this;
        var sections = Object.keys(this.structure);
        this.builtRows = {};
        sections.forEach(function (section, index) {
            var properties = self.structure[section];
            self.addSection(self.parent, section, properties, index);
        });
    },
    addSection: function (parent, section, properties, index) {
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

        dropDown.innerHTML = collapsed ? "L" : "M";
        sectionDivHeader.setAttribute("collapsed", collapsed);
        parent.appendChild(sectionDivHeader);

        var sectionDivBody = document.createElement("div");
        sectionDivBody.className = "property-group";

        var animationWrapper = document.createElement("div");
        animationWrapper.className = "section-wrap";

        for (var property in properties) {
            if (properties.hasOwnProperty(property)) {
                var builtRow = self.addElement(animationWrapper, properties[property]);
                var id = properties[property].id;
                if (id) {
                    self.builtRows[id] = builtRow;
                }
            }
        }
        sectionDivBody.appendChild(animationWrapper);
        parent.appendChild(sectionDivBody);
        _.defer(function () {
            var height = (animationWrapper.clientHeight) + "px";
            if (collapsed) {
                sectionDivBody.classList.remove("visible");
                sectionDivBody.style.maxHeight = "0px";
            } else {
                sectionDivBody.classList.add("visible");
                sectionDivBody.style.maxHeight = height;
            }

            sectionDivHeader.onclick = function () {
                collapsed = !collapsed;
                if (collapsed) {
                    sectionDivBody.classList.remove("visible");
                    sectionDivBody.style.maxHeight = "0px";
                } else {
                    sectionDivBody.classList.add("visible");
                    sectionDivBody.style.maxHeight = (animationWrapper.clientHeight) + "px";
                }
                // sectionDivBody.style.display = collapsed ? "none": "block";
                dropDown.innerHTML = collapsed ? "L" : "M";
                sectionDivHeader.setAttribute("collapsed", collapsed);
            };
        });
    },
    addLabel: function (parent, group) {
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
    addVector: function (parent, group, labels, domArray) {
        var self = this;
        var inputs = labels ? labels : ["x", "y", "z"];
        domArray = domArray ? domArray : [];
        parent.id = group.id;
        for (var index in inputs) {
            var element = document.createElement("input");

            element.setAttribute("type", "number");
            element.className = inputs[index];
            element.id = group.id + "-" + inputs[index];

            if (group.defaultRange) {
                if (group.defaultRange.min) {
                    element.setAttribute("min", group.defaultRange.min);
                }
                if (group.defaultRange.max) {
                    element.setAttribute("max", group.defaultRange.max);
                }
                if (group.defaultRange.step) {
                    element.setAttribute("step", group.defaultRange.step);
                }
            }
            if (group.oninput) {
                element.oninput = group.oninput;
            } else {
                element.oninput = function (event) {
                    self.webBridgeSync(group.id, {
                        x: domArray[0].value,
                        y: domArray[1].value,
                        z: domArray[2].value
                    });
                };
            }
            element.onchange = element.oninput;
            domArray.push(element);
        }

        this.addLabel(parent, group);
        var className = "";
        for (var i = 0; i < inputs.length; i++) {
            className += inputs[i].charAt(0)
                .toLowerCase();
        }
        parent.className += " property vector-section " + className;

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
    addVectorQuaternion: function (parent, group) {
        this.addVector(parent, group, ["Pitch", "Yaw", "Roll"]);
    },
    addColorPicker: function (parent, group) {
        var self = this;
        var $colPickContainer = $('<div>', {
            id: group.id,
            class: "color-picker"
        });
        var updateColors = function (red, green, blue) {
            $colPickContainer.css('background-color', "rgb(" +
                red + "," +
                green + "," +
                blue + ")");
        };

        var inputs = ["red", "green", "blue"];
        var domArray = [];
        group.oninput = function (event) {
            $colPickContainer.colpickSetColor(
                {
                    r: domArray[0].value,
                    g: domArray[1].value,
                    b: domArray[2].value
                },
                true);
        };
        group.defaultRange = {
            min: 0,
            max: 255,
            step: 1
        };

        parent.appendChild($colPickContainer[0]);
        self.addVector(parent, group, inputs, domArray);

        updateColors(domArray[0].value, domArray[1].value, domArray[2].value);

        // Could probably write a custom one for this to completely write out jquery,
        // but for now, using the same as earlier.

        /* Color Picker Logic Here */


        $colPickContainer.colpick({
            colorScheme: 'dark',
            layout: 'hex',
            color: {
                r: domArray[0].value,
                g: domArray[1].value,
                b: domArray[2].value
            },
            onChange: function (hsb, hex, rgb, el) {
                updateColors(rgb.r, rgb.g, rgb.b);

                domArray[0].value = rgb.r;
                domArray[1].value = rgb.g;
                domArray[2].value = rgb.b;
                self.webBridgeSync(group.id, {
                    red: rgb.r,
                    green: rgb.g,
                    blue: rgb.b
                });
            },
            onSubmit: function (hsb, hex, rgb, el) {
                $(el)
                    .css('background-color', '#' + hex);
                $(el)
                    .colpickHide();
                domArray[0].value = rgb.r;
                domArray[1].value = rgb.g;
                domArray[2].value = rgb.b;
                self.webBridgeSync(group.id, {
                    red: rgb.r,
                    green: rgb.g,
                    blue: rgb.b
                });
            }
        });
    },
    addTextureField: function (parent, group) {
        var self = this;
        this.addLabel(parent, group);
        parent.className += " property texture";
        var textureImage = document.createElement("div");
        var textureUrl = document.createElement("input");
        textureUrl.setAttribute("type", "text");
        textureUrl.id = group.id;
        textureImage.className = "texture-image no-texture";
        var image = document.createElement("img");
        var imageLoad = _.debounce(function (url) {
            if (url.length > 0) {
                textureImage.classList.remove("no-texture");
                textureImage.classList.add("with-texture");
                image.src = url;
                image.style.display = "block";
            } else {
                image.src = "";
                image.style.display = "none";
                textureImage.classList.add("no-texture");
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
    addSlider: function (parent, group) {
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
            inputField.setAttribute("min", group.min !== undefined ? group.min : 0);
            inputField.setAttribute("step", 1);

            slider.setAttribute("min", group.min !== undefined ? group.min : 0);
            slider.setAttribute("max", group.max !== undefined ? group.max : 10000);
            slider.setAttribute("step", 1);

            inputField.oninput = function (event) {

                if (parseInt(event.target.value) > parseInt(slider.getAttribute("max")) && group.max !== 1) {
                    slider.setAttribute("max", event.target.value);
                }
                slider.value = event.target.value;

                self.webBridgeSync(group.id, slider.value);
            };
            inputField.onchange = inputField.oninput;
            slider.oninput = function (event) {
                inputField.value = event.target.value;
                self.webBridgeSync(group.id, slider.value);
            };

            inputField.id = group.id;
        } else if (group.type === "SliderRadian") {
            slider.setAttribute("min", group.min !== undefined ? group.min : 0);
            slider.setAttribute("max", group.max !== undefined ? group.max : 180);
            slider.setAttribute("step", 1);
            parent.className += " radian";
            inputField.setAttribute("min", (group.min !== undefined ? group.min : 0));
            inputField.setAttribute("max", (group.max !== undefined ? group.max : 180));

            inputField.oninput = function (event) {
                slider.value = event.target.value;
                self.webBridgeSync(group.id, slider.value * RADIANS_PER_DEGREE);
            };
            inputField.onchange = inputField.oninput;

            inputField.id = group.id;
            slider.oninput = function (event) {
                if (event.target.value > 0) {
                    inputField.value = Math.floor(event.target.value);
                } else {
                    inputField.value = Math.ceil(event.target.value);
                }
                self.webBridgeSync(group.id, slider.value * RADIANS_PER_DEGREE);
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
            inputField.setAttribute("min", group.min !== undefined ? group.min : 0);
            slider.setAttribute("step", 0.01);

            slider.setAttribute("min", group.min !== undefined ? group.min : 0);
            slider.setAttribute("max", group.max !== undefined ? group.max : 1);
            slider.setAttribute("step", 0.01);

            inputField.oninput = function (event) {
                if (parseFloat(event.target.value) > parseFloat(slider.getAttribute("max")) && group.max !== 1) {
                    slider.setAttribute("max", event.target.value);
                }

                slider.value = event.target.value;
                self.webBridgeSync(group.id, slider.value);
                // bind web sock update here.
            };
            inputField.onchange = inputField.oninput;
            slider.oninput = function (event) {
                inputField.value = event.target.value;
                self.webBridgeSync(group.id, inputField.value);
            };

            inputField.id = group.id;
        }

        // UpdateBinding
    },
    addCheckBox: function (parent, group) {
        var checkBox = document.createElement("input");
        checkBox.setAttribute("type", "checkbox");
        var self = this;
        checkBox.onchange = function (event) {
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
        switch (group.type) {
            case "Button":
                var button = document.createElement("input");
                button.setAttribute("type", "button");
                button.id = group.id;
                if (group.disabled) {
                    button.disabled = group.disabled;
                }
                button.className = group.class;
                button.value = group.name;

                button.onclick = group.callback;
                parent.appendChild(button);
                break;
            case "Row":
                var hr = document.createElement("hr");
                hr.className = "splitter";
                if (group.id) {
                    hr.id = group.id;
                }
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
