import QtQuick 2.6

Item {
    id: root

    property int selected: -1
    signal selectionChanged(int index)
    onSelectionChanged: { selected = index }

    Connections {
        target: configWidget
        onPanelAdded: {
            selectionChanged(index);
        }
        onPanelDeleted: {
            selectionChanged(-1);
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: { root.selectionChanged(-1); root.selected = -1 }
    }

    property int osdRows: 16
    property int osdCols: 30

    Rectangle {
        anchors.fill: parent
        color: "blue"

        ListView {
            anchors.fill: parent

            model: panelsModel
            delegate: Rectangle {
                border.width: 1
                border.color: "grey"

                visible: type != "Disabled"
                color: root.selected === model.modelData.index ? "#80ff0000" : "transparent"

                anchors.left: parent.left
                anchors.top: parent.top
                anchors.leftMargin: model.modelData.xPos / root.osdCols * root.width
                anchors.topMargin: model.modelData.yPos / root.osdRows * root.height
                width: model.modelData.width / root.osdCols * root.width
                height: model.modelData.height / root.osdRows * root.height

                Text {
                    text: type
                    color: "white"
                    anchors.centerIn: parent
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (mouse.button & Qt.LeftButton)
                            root.selectionChanged(model.modelData.index)
                        else
                            root.selectionChanged(-1)
                    }
                }
            }
        }
    }
}
