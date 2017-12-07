import QtQuick 2.0

Item {
    id: realAttitude

    Rectangle {
        id: realAtt
        smooth: true

        property variant scaledBounds: svgRenderer.scaledElementBounds("pfd.svg", "world")
        width:  Math.round(sceneItem.width *scaledBounds.width /2)*2
        height: Math.round(sceneItem.height*scaledBounds.height/2)*2

        visible: AttitudeSimulated.isPresentOnHardware && (AttitudeSimulated.Roll !== 0 || AttitudeSimulated.Pitch !== 0)

        gradient: Gradient {
            GradientStop { position: 0.496;    color: "#00000000" }
            GradientStop { position: 0.500;    color: "#c0008000" }
            GradientStop { position: 0.504;    color: "#00000000" }
        }

        transform: [
            Translate {
                x: Math.round((realAtt.parent.width - realAtt.width)/2)
                y: (realAtt.parent.height - realAtt.height)/2+
                   realAtt.parent.height/2*Math.sin(AttitudeSimulated.Pitch*Math.PI/180)*1.405
            },
            Rotation {
                angle: -AttitudeSimulated.Roll
                origin.x : realAtt.parent.width/2
                origin.y : realAtt.parent.height/2
            }
        ]
    }
}
