import QtQuick 2.0

Item {
    id: container
    width: 100
    height: 62

    signal clicked(string url)

    Text {
        id: header
        text: "dRonin Links"
        width: parent.width
        color: "#44515c"
        font {
            pointSize: 14
            weight: Font.Bold
        }
    }

    ListModel {
        id: sitesModel
        ListElement { title: "Documentation"; link: "http://dronin.readme.io/" }
        ListElement { title: "Home Page"; link: "http://dronin.org" }
        ListElement { title: "Forum"; link: "https://forum.dronin.org/forum/" }
        ListElement { title: "Wiki"; link: "https://github.com/d-ronin/dRonin/wiki" }
        ListElement { title: "GitHub Repository"; link: "https://github.com/d-ronin/dRonin" }
        ListElement { title: "dRonin Facebook"; link: "https://www.facebook.com/flydRonin" }
    }

    ListView {
        id: view
        width: parent.width
        anchors { top: header.bottom; topMargin: 14; bottom: parent.bottom }
        model: sitesModel
        spacing: 8
        clip: true

        delegate: Text {
            text: title
            width: view.width
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere

            font {
                pointSize: 12
                weight: Font.Bold
            }

            color: mouseArea.containsMouse ? "#224d81" : "black"

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: Qt.openUrlExternally(link)
            }
        }
    }
}
