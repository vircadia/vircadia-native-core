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
	id: root
	property var config: Render.getConfig("RenderMainView.FadeConfigure");
	property var switchConfig: Render.getConfig("RenderMainView.FadeSwitch");
	spacing: 8
	Row {
		spacing: 8

		CheckBox {
			text: "Edit Fade"
			checked: root.switchConfig["editFade"]
			onCheckedChanged: { 
				root.switchConfig["editFade"] = checked; 
				Render.getConfig("RenderMainView.DrawFadedOpaqueBounds").enabled = checked;
			}
		}
		ComboBox {
			id: categoryBox
			width: 400
			model: ["Elements enter/leave domain", "Bubble isect. - Owner POV", "Bubble isect. - Trespasser POV", "Another user leaves/arrives", "Changing an avatar"]
			Timer {
				id: postpone
				interval: 100; running: false; repeat: false
				onTriggered: { paramWidgetLoader.sourceComponent = paramWidgets }
			}
			onCurrentIndexChanged: {
				root.config["editedCategory"] = currentIndex;
				// This is a hack to be sure the widgets below properly reflect the change of category: delete the Component
				// by setting the loader source to Null and then recreate it 100ms later
				paramWidgetLoader.sourceComponent = undefined;
				postpone.start()
			}
		}
	}
	Row {
		CheckBox {
			text: "Manual"
			checked: root.config["manualFade"]
			onCheckedChanged: { 
				root.config["manualFade"] = checked; 
			}
		}
		ConfigSlider {
			label: "Threshold"
			integral: false
			config: root.config
			property: "manualThreshold"
			max: 1.0
			min: 0.0
			width: 400
		}
	}


	Component {
		id: paramWidgets

		Column {
			spacing: 8

			ConfigSlider {
				label: "Duration"
				integral: false
				config: root.config
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
						config: root.config
						property: "baseSizeX"
						max: 1.0
						min: 0.0
						width: 400
					}
					ConfigSlider {
						label: "Size Y"
						integral: false
						config: root.config
						property: "baseSizeY"
						max: 1.0
						min: 0.0
						width: 400
					}				
					ConfigSlider {
						label: "Size Z"
						integral: false
						config: root.config
						property: "baseSizeZ"
						max: 1.0
						min: 0.0
						width: 400
					}	
					ConfigSlider {
						label: "Level"
						integral: false
						config: root.config
						property: "baseLevel"
						max: 1.0
						min: 0.0
						width: 400
					}
					CheckBox {
						text: "Invert"
						checked: root.config["baseInverted"]
						onCheckedChanged: { root.config["baseInverted"] = checked }
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
						config: root.config
						property: "noiseSizeX"
						max: 1.0
						min: 0.0
						width: 400
					}
					ConfigSlider {
						label: "Size Y"
						integral: false
						config: root.config
						property: "noiseSizeY"
						max: 1.0
						min: 0.0
						width: 400
					}				
					ConfigSlider {
						label: "Size Z"
						integral: false
						config: root.config
						property: "noiseSizeZ"
						max: 1.0
						min: 0.0
						width: 400
					}				
					ConfigSlider {
						label: "Level"
						integral: false
						config: root.config
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
						config: root.config
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
								config: root.config
								property: "edgeInnerColorR"
								max: 1.0
								min: 0.0
								width: 400
							}				
							ConfigSlider {
								label: "Color G"
								integral: false
								config: root.config
								property: "edgeInnerColorG"
								max: 1.0
								min: 0.0
								width: 400
							}
							ConfigSlider {
								label: "Color B"
								integral: false
								config: root.config
								property: "edgeInnerColorB"
								max: 1.0
								min: 0.0
								width: 400
							}
							ConfigSlider {
								label: "Color intensity"
								integral: false
								config: root.config
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
								config: root.config
								property: "edgeOuterColorR"
								max: 1.0
								min: 0.0
								width: 400
							}				
							ConfigSlider {
								label: "Color G"
								integral: false
								config: root.config
								property: "edgeOuterColorG"
								max: 1.0
								min: 0.0
								width: 400
							}
							ConfigSlider {
								label: "Color B"
								integral: false
								config: root.config
								property: "edgeOuterColorB"
								max: 1.0
								min: 0.0
								width: 400
							}
							ConfigSlider {
								label: "Color intensity"
								integral: false
								config: root.config
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

	Loader {
		id: paramWidgetLoader
		sourceComponent: paramWidgets
	}
}
