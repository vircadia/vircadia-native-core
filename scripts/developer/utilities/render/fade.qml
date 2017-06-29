//
//  fade.qml
//  developer/utilities/render
//
//  Olivier Prat, created on 30/04/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import "configSlider"

Column {
	property var config: Render.getConfig("RenderDeferredTask");
	spacing: 8
	Row {
		spacing: 8

		CheckBox {
			text: "Edit Fade"
			checked: config["editFade"]
			onCheckedChanged: { config["editFade"] = checked }
		}
		ComboBox {
			width: 400
			model: ["Elements enter/leave domain", "Bubble isect. - Owner POV", "Bubble isect. - Trespasser POV", "Another user leaves/arrives", "Changing an avatar"]
			onCurrentIndexChanged: { config["editedCategory"] = currentIndex }
		}
	}
	Column {
		spacing: 8

		ConfigSlider {
			label: "Duration"
			integral: false
			config: config
			property: "duration"
			max: 10.0
			min: 0.1
			width: 400
		}
		GroupBox {
			title: "Base Gradient"
			width: 500
			Column {
				spacing: 8

				ConfigSlider {
					label: "Size X"
					integral: false
					config: config
					property: "baseSizeX"
					max: 1.0
					min: 0.0
					width: 400
				}
				ConfigSlider {
					label: "Size Y"
					integral: false
					config: config
					property: "baseSizeY"
					max: 1.0
					min: 0.0
					width: 400
				}				
				ConfigSlider {
					label: "Size Z"
					integral: false
					config: config
					property: "baseSizeZ"
					max: 1.0
					min: 0.0
					width: 400
				}	
				ConfigSlider {
					label: "Level"
					integral: false
					config: config
					property: "baseLevel"
					max: 1.0
					min: 0.0
					width: 400
				}
				CheckBox {
					text: "Invert"
					checked: config["baseInverted"]
					onCheckedChanged: { config["baseInverted"] = checked }
				}
			}
		}
		GroupBox {
			title: "Noise Gradient"
			width: 500
			Column {
				spacing: 8

				ConfigSlider {
					label: "Size X"
					integral: false
					config: config
					property: "noiseSizeX"
					max: 1.0
					min: 0.0
					width: 400
				}
				ConfigSlider {
					label: "Size Y"
					integral: false
					config: config
					property: "noiseSizeY"
					max: 1.0
					min: 0.0
					width: 400
				}				
				ConfigSlider {
					label: "Size Z"
					integral: false
					config: config
					property: "noiseSizeZ"
					max: 1.0
					min: 0.0
					width: 400
				}				
				ConfigSlider {
					label: "Level"
					integral: false
					config: config
					property: "noiseLevel"
					max: 1.0
					min: 0.0
					width: 400
				}
			}
		}
		GroupBox {
			title: "Edge"
			width: 500
			Column {
				spacing: 8

				ConfigSlider {
					label: "Width"
					integral: false
					config: config
					property: "edgeWidth"
					max: 1.0
					min: 0.0
					width: 400
				}
				GroupBox {
					title: "Inner color"
					Column {
						spacing: 8
						ConfigSlider {
							label: "Color R"
							integral: false
							config: config
							property: "edgeInnerColorR"
							max: 1.0
							min: 0.0
							width: 400
						}				
						ConfigSlider {
							label: "Color G"
							integral: false
							config: config
							property: "edgeInnerColorG"
							max: 1.0
							min: 0.0
							width: 400
						}
						ConfigSlider {
							label: "Color B"
							integral: false
							config: config
							property: "edgeInnerColorB"
							max: 1.0
							min: 0.0
							width: 400
						}
						ConfigSlider {
							label: "Color intensity"
							integral: false
							config: config
							property: "edgeInnerIntensity"
							max: 5.0
							min: 0.0
							width: 400
						}
					}
				}
				GroupBox {
					title: "Outer color"
					Column {
						spacing: 8
						ConfigSlider {
							label: "Color R"
							integral: false
							config: config
							property: "edgeOuterColorR"
							max: 1.0
							min: 0.0
							width: 400
						}				
						ConfigSlider {
							label: "Color G"
							integral: false
							config: config
							property: "edgeOuterColorG"
							max: 1.0
							min: 0.0
							width: 400
						}
						ConfigSlider {
							label: "Color B"
							integral: false
							config: config
							property: "edgeOuterColorB"
							max: 1.0
							min: 0.0
							width: 400
						}
						ConfigSlider {
							label: "Color intensity"
							integral: false
							config: config
							property: "edgeOuterIntensity"
							max: 5.0
							min: 0.0
							width: 400
						}
					}
				}
			}
		}
	}
}

