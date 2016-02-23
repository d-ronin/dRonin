import QtQuick 2.0
import "./JSONListModel"
import "TimeDifference.js" as TimeDifference
import "TitleCase.js" as TitleCase

Item {
    id: container
    width: 100
    height: 62

    signal clicked(string url)

    Text {
        id: header
        text: "Recent AutoTune Results"
        width: parent.width
        color: "#44515c"
        font {
            pointSize: 14
            weight: Font.Bold
        }
    }

    ListView {
        id: view
        width: parent.width
        anchors { top: header.bottom; topMargin: 14; bottom: parent.bottom }
        model: jsonModel.model
        delegate: listDelegate
        clip: true
    }

    ScrollDecorator {
        flickableItem: view
    }

    JSONListModel {
        id: jsonModel
        source: "http://dronin-autotown.appspot.com/api/recentTunes"
        query: "$[*]"
    }

    Component {
        id: listDelegate
        Item  {
            width: view.width
            height: column.height + 8

            property string link: "http://dronin-autotown.appspot.com/at/tune/" + model.Key
            property string relDate: TimeDifference.relativeTime(Date.parse(model.Timestamp))

            Row {
                id: column
                spacing: 4
                Column
                {
                    Text {
                        text: model.Board
                        width: view.width
                        font.bold: true
                        elide: Text.ElideRight
                        color: mouseArea.containsMouse ? "#224d81" : "black"
                        MouseArea {
                            anchors.fill: parent  //...to cover the whole image
                            onClicked: Qt.openUrlExternally(link)
                        }
                    }
                    Text {
                        text: "Tau: %1 ms".arg(model.Tau * 1000.0)
                        width: view.width
                        elide: Text.ElideRight
                        color: mouseArea.containsMouse ? "#224d81" : "black"
                        MouseArea {
                            anchors.fill: parent  //...to cover the whole image
                            onClicked: Qt.openUrlExternally(link)
                        }
                    }
                    Text {
                        text: "%1 in %2, %3".arg(relDate).arg(TitleCase.toTitleCase(model.City)).arg(model.Country)
                        font.italic: true
                        width: view.width
                        elide: Text.ElideRight
                        color: mouseArea.containsMouse ? "#224d81" : "black"
                        MouseArea {
                            anchors.fill: parent  //...to cover the whole image
                            onClicked: Qt.openUrlExternally(link)
                        }
                    }
                }

            }
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
