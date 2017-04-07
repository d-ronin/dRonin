#ifndef GPSSNRWIDGET_H
#define GPSSNRWIDGET_H

#include <QGraphicsRectItem>
#include <QGraphicsView>

class GpsSnrWidget : public QGraphicsView
{
    Q_OBJECT
public:
    explicit GpsSnrWidget(QWidget *parent = 0);
    ~GpsSnrWidget();

signals:

public slots:
    void updateSat(int index, int prn, int elevation, int azimuth, int snr);
    void satellitesDone();

private:
    static const int MAX_SATELLITES = 32;
    static const int MAX_SHOWN_SATELLITES = 18;
    int satellites[MAX_SATELLITES][4];
    QGraphicsScene *scene;
    QGraphicsRectItem *boxes[MAX_SHOWN_SATELLITES];
    QGraphicsSimpleTextItem *satTexts[MAX_SHOWN_SATELLITES];
    QGraphicsSimpleTextItem *satSNRs[MAX_SHOWN_SATELLITES];

    void drawSat(int drawIndex, int index);

protected:
    void showEvent(QShowEvent *event);
    void resizeEvent(QResizeEvent *event);
};

#endif // GPSSNRWIDGET_H
