#ifndef SEPPUKUCONFIGURATION_H
#define SEPPUKUCONFIGURATION_H

#include <QSvgRenderer>
#include <QGraphicsSvgItem>

#include "configtaskwidget.h"
#include "ui_seppuku.h"

class SeppukuConfiguration : public ConfigTaskWidget
{
    Q_OBJECT
public:
    SeppukuConfiguration(QWidget *parent = 0);
    ~SeppukuConfiguration();

private:
    Ui::Seppuku *ui;
    QSvgRenderer *m_renderer;
    QGraphicsSvgItem *m_background;
    QGraphicsSvgItem *m_uart4;
    QGraphicsScene *m_scene;

    void setupGraphicsScene();
    void resizeEvent(QResizeEvent *event);
    void showEvent(QShowEvent *event);
    void setMessage(const QString &name, const QString &msg = QString(), const QString &severity = QString("warning"));

private slots:
    void magChanged(const QString &newVal);
    void outputsChanged(const QString &newVal);
    void checkExtMag();
    void checkDsm();
};

#endif // SEPPUKUCONFIGURATION_H
