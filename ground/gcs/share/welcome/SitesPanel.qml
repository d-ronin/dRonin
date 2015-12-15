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
        ListElement { title: "dRonin Home"; link: "http://dronin.org" }
        ListElement { title: "dRonin Wiki"; link: "https://github.com/d-ronin/dRonin/wiki" }
        ListElement { title: "dRonin Code"; link: "https://github.com/d-ronin/dRonin" }
        ListElement { title: "dRonin Issues"; link: "https://github.com/d-ronin/dRonin/issues" }
        ListElement { title: "dRonin Facebook"; link: "https://www.facebook.com/flydRonin" }
        ListElement { title: "dRonin Twitter"; link: "https://twitter.com/flydRonin" }
        ListElement { title: "dRonin Instagram"; link: "https://www.instagram.com/flydronin/" }
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
                onClicked: {
                    console.log(link)
                    container.clicked(link)
                }
            }
        }
    }
}
