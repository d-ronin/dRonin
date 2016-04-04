import QtQuick 2.0
import QtQuick.XmlListModel 2.0

Item {
    id: container
    width: 100
    height: 62

    signal clicked(string url)

    Text {
        id: header
        text: "GitHub Activity"
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
        model: xmlModel
        delegate: listDelegate
        clip: true
    }

    ScrollDecorator {
        flickableItem: view
    }

    XmlListModel {
        id: xmlModel
        source: "https://github.com/d-ronin/dRonin/commits/next.atom"
        query: "/feed/entry[not(fn:contains(title,'Merge pull request'))]"
        namespaceDeclarations: "declare namespace media=\"http://search.yahoo.com/mrss/\"; declare default element namespace \"http://www.w3.org/2005/Atom\" ;"
        XmlRole { name: "title"; query: "title/string()" }
        XmlRole { name: "author"; query: "author/name/string()" }
        XmlRole { name: "authoruri"; query: "author/uri/string()" }
        XmlRole { name: "link"; query: "link/@href/string()" }
        XmlRole { name: "thumbnail"; query: 'media:thumbnail/@url/string()' }
    }

    Component {
        id: listDelegate
        Item  {
            width: view.width
            height: column.height + 8

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: Qt.openUrlExternally(link)
            }

            Row {
                id: column
                spacing: 4
                Image {
                    id: recipeImage
                    width: 30; height: 30
                    source: thumbnail
                    MouseArea {
                        anchors.fill: parent  //...to cover the whole image
                        onClicked: Qt.openUrlExternally(authoruri)
                    }
                }
                Text {
                    text: author
                    font.italic: true
                    width: view.width * 0.15
                    textFormat: text.indexOf("&") > 0 ? Text.StyledText : Text.PlainText
                    elide: Text.ElideRight
                    color: mouseArea.containsMouse ? "#224d81" : "black"
                    MouseArea {
                        anchors.fill: parent  //...to cover the whole image
                        onClicked: Qt.openUrlExternally(authoruri)
                    }
                }
                Text {
                    text: title.trim()
                    font.bold: true
                    elide: Text.ElideRight
                    color: mouseArea.containsMouse ? "#224d81" : "black"
                    MouseArea {
                        anchors.fill: parent  //...to cover the whole image
                        onClicked: Qt.openUrlExternally(link)
                    }
                }
            }
        }
    }
}
