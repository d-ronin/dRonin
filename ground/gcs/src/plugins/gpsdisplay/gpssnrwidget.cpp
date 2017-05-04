#include "gpssnrwidget.h"

GpsSnrWidget::GpsSnrWidget(QWidget *parent)
    : QGraphicsView(parent)
{

    scene = new QGraphicsScene(this);
    setScene(scene);

    // Now create 'max Shown Satellites' satellite icons which we will move around on the map:
    for (int i = 0; i < MAX_SATELLITES; i++) {
        satellites[i][0] = 0;
        satellites[i][1] = 0;
        satellites[i][2] = 0;
        satellites[i][3] = 0;
    }

    for (int i = 0; i < MAX_SHOWN_SATELLITES; i++) {
        boxes[i] = new QGraphicsRectItem();
        boxes[i]->setBrush(QColor("Green"));
        scene->addItem(boxes[i]);
        boxes[i]->hide();

        satTexts[i] = new QGraphicsSimpleTextItem("##", boxes[i]);
        satTexts[i]->setBrush(QColor("White"));
        satTexts[i]->setFont(QFont("Courier", -1, QFont::ExtraBold));

        satSNRs[i] = new QGraphicsSimpleTextItem("##", boxes[i]);
        satSNRs[i]->setBrush(QColor("Black"));
        satSNRs[i]->setFont(QFont("Courier", -1, QFont::ExtraBold));
    }
}

GpsSnrWidget::~GpsSnrWidget()
{
    delete scene;
    scene = 0;
}

void GpsSnrWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)

    satellitesDone();
}

void GpsSnrWidget::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);

    satellitesDone();
}

void GpsSnrWidget::satellitesDone()
{
    scene->setSceneRect(0, 0, this->viewport()->width(), this->viewport()->height());

    int drawIndex = 0;

    /* Display ones with a positive snr first; UBX driver orders this way but
     * NMEA does not
     */
    for (int index = 0; index < MAX_SATELLITES; index++) {
        if (satellites[index][3] > 0) {
            drawSat(drawIndex++, index);
        }
    }

    for (int index = 0; index < MAX_SATELLITES; index++) {
        if (satellites[index][3] <= 0) {
            drawSat(drawIndex++, index);
        }
    }
}

void GpsSnrWidget::updateSat(int index, int prn, int elevation, int azimuth, int snr)
{
    if (index >= MAX_SATELLITES) {
        // A bit of error checking never hurts.
        return;
    }

    // TODO: add range checking
    satellites[index][0] = prn;
    satellites[index][1] = elevation;
    satellites[index][2] = azimuth;
    satellites[index][3] = snr;
}

void GpsSnrWidget::drawSat(int drawIndex, int index)
{
    if (index >= MAX_SATELLITES) {
        // A bit of error checking never hurts.
        return;
    }

    if (drawIndex >= MAX_SHOWN_SATELLITES) {
        return;
    }

    const int prn = satellites[index][0];
    const int snr = satellites[index][3];
    if (prn && snr) {
        boxes[drawIndex]->show();

        // When using integer values, width and height are the
        // box width and height, but the left and bottom borders are drawn on the box,
        // and the top and right borders are drawn just next to the box.
        // So the box seems one pixel wider and higher with a border.
        // I'm sure there's a good explanation for that :)

        // Casting to int rounds down, which is what I want.
        // Minus 2 to allow a pixel of white left and right.
        int availableWidth = (int)((scene->width() - 2) / MAX_SHOWN_SATELLITES);

        // 2 pixels, one on each side.
        qreal width = availableWidth - 2;
        // SNR = 1-99 (0 is special)..
        qreal height = int((scene->height() / 99) * snr + 0.5);
        // 1 for showing a pixel of white to the left.
        qreal x = availableWidth * drawIndex + 1;
        // Rember, 0 is at the top.
        qreal y = scene->height() - height;
        // Compensate for the extra pixel for the border.
        boxes[drawIndex]->setRect(0, 0, width - 1, height - 1);
        boxes[drawIndex]->setPos(x, y);

        QRectF boxRect = boxes[drawIndex]->boundingRect();
        QString prnString = QString().number(prn);
        if (prnString.length() == 1) {
            prnString = "0" + prnString;
        }
        satTexts[drawIndex]->setText(prnString);
        QRectF textRect = satTexts[drawIndex]->boundingRect();

        QTransform matrix;
        qreal scale = 0.85 * (boxRect.width() / textRect.width());
        matrix.translate(boxRect.width() / 2, boxRect.height());
        matrix.scale(scale, scale);
        matrix.translate(-textRect.width() / 2, -textRect.height());
        satTexts[drawIndex]->setTransform(matrix, false);

        QString snrString = QString().number(snr);
        if (snrString.length() == 1) { // Will probably never happen!
            snrString = "0" + snrString;
        }
        satSNRs[drawIndex]->setText(snrString);
        textRect = satSNRs[drawIndex]->boundingRect();

        matrix.reset();
        scale = 0.85 * (boxRect.width() / textRect.width());
        matrix.translate(boxRect.width() / 2, 0);
        matrix.scale(scale, scale);
        matrix.translate(-textRect.width() / 2, -textRect.height());
        satSNRs[drawIndex]->setTransform(matrix, false);

    } else {
        boxes[drawIndex]->hide();
    }
}
