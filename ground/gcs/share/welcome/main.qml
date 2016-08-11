import QtQuick 2.0

Rectangle {
    id: container
    width: 1024
    height: 768
    gradient: Gradient {
        GradientStop {
            position: 0
            color: "#ffffff"
        }

        GradientStop {
            position: 0.03
            color: "#ffffff"
        }

        GradientStop {
            position: 1
            color: "#b9b9b9"
        }

        GradientStop {
            position: 0.927
            color: "#d2d2d2"
        }


    }

    Column {
        id: buttonsGrid
        anchors.horizontalCenter: parent.horizontalCenter
        // distribute a vertical space between the icons blocks an community widget as:
        // top - 48% - Icons - 27% - CommunityWidget - 25% - bottom
        y: (parent.height - buttons.height - communityPanel.height) * 0.48
        width: parent.width
        spacing: (parent.height - buttons.height - communityPanel.height) * 0.27

        Row {
            //if the buttons grid overlaps vertically with the wizard buttons,
            //move it left to use only the space left to wizard buttons
            property real availableWidth: container.width
            x: (availableWidth-width)/2
            spacing: 16

            Image {
                x: -55
                width: 223
                smooth: false
                antialiasing: true
                sourceSize.height: 223
                sourceSize.width: 223
                source: "images/welcome-logo.png"
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -1 //it looks better aligned to icons grid

                //hide the logo on the very small screen to fit the buttons
                visible: parent.availableWidth > width + parent.spacing + buttons.width + wizard.width
            }

            Grid {
                id: buttons
                objectName: "modeButtons"
                columns: 3
                spacing: 4
                anchors.verticalCenter: parent.verticalCenter
                property variant modeNames: []

                WelcomePageButton {
                    baseIconName: "flightdata"
                    label: "Flight Data"
                    onClicked: welcomePlugin.openPage("Flight data")
                    visible: parent.modeNames.indexOf("Flight data") >= 0
                }

                WelcomePageButton {
                    baseIconName: "config"
                    label: "Configuration"
                    onClicked: welcomePlugin.openPage("Configuration")
                    visible: parent.modeNames.indexOf("Configuration") >= 0
                }

                WelcomePageButton {
                    baseIconName: "system"
                    label: "System"
                    onClicked: welcomePlugin.openPage("System")
                    visible: parent.modeNames.indexOf("System") >= 0
                }

               WelcomePageButton {
                    baseIconName: "scopes"
                    label: "Scopes"
                    onClicked: welcomePlugin.openPage("Scopes")
                    visible: parent.modeNames.indexOf("Scopes") >= 0
                }

                WelcomePageButton {
                    baseIconName: "advanced"
                    label: "Advanced"
                    onClicked: welcomePlugin.openPage("Advanced")
                    visible: parent.modeNames.indexOf("Advanced") >= 0
                }

                WelcomePageButton {
                    baseIconName: "firmware"
                    label: "Firmware"
                    onClicked: welcomePlugin.openPage("Firmware")
                    visible: parent.modeNames.indexOf("Firmware") >= 0
                }
            } //icons grid

            WelcomePageButton {
                id: wizard
                width: 233
                height: 233
                scale: 0.97
                anchors.verticalCenterOffset: -3
                anchors.verticalCenter: parent.verticalCenter
                baseIconName: "wizard"
                onClicked: welcomePlugin.triggerAction("SetupWizardPlugin.ShowSetupWizard")
            }

        } // images row

        CommunityPanel {
            id: communityPanel
            anchors.horizontalCenter: parent.horizontalCenter
            width: container.width * 0.9
            height: Math.min(300, container.height*0.5)
        }
    }
}
