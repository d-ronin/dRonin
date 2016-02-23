import QtQuick 2.0

Item {
    property alias sourceSize: background.sourceSize
    width: sourceSize.width
    height: 300

    BorderImage {
        id: background
        anchors.fill: parent

        border { left: 30; top: 30; right: 30; bottom: 30 }
        source: "images/welcome-news-bg.png"
    }

    GitHubNewsPanel {
        id: gitHubNewsPanel
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: parent.width * 0.5
        anchors.margins: 32

        onClicked: welcomePlugin.openUrl(url)
    }

    AutoTunePanel {
        id: autoTunePanel
        anchors.left: gitHubNewsPanel.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: parent.width * 0.25
        anchors.margins: 32

        onClicked: welcomePlugin.openUrl(url)
    }

    //better to use image instead
    Rectangle {
        id: separator
        width: 1
        height: parent.height * 0.7
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: autoTunePanel.right
        anchors.margins: 16
        color: "#A0A0A0"
    }
    SitesPanel {
        transformOrigin: Item.Center
        anchors.left: autoTunePanel.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 32

        onClicked: welcomePlugin.openUrl(url)
    }
}
