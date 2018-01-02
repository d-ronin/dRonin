#ifndef POPUPWIDGET_H
#define POPUPWIDGET_H

#include <uavobjectwidgetutils/uavobjectwidgetutils_global.h>

#include <QtGui>
#include <QWidget>
#include <QDialog>
#include <QHBoxLayout>
#include <QPushButton>


namespace Ui {
class PopupWidget;
}

class UAVOBJECTWIDGETUTILS_EXPORT PopupWidget : public QDialog
{
    Q_OBJECT
public:
    explicit PopupWidget(QWidget *parent = nullptr);

    void popUp(QWidget *widget = nullptr);
    void setWidget(QWidget *widget);
    QWidget *getWidget() { return m_widget; }
    QHBoxLayout *getLayout() { return m_layout; }

signals:

public slots:
    bool close();
    void done(int result);

private slots:
    void closePopup();

private:
    QHBoxLayout *m_layout;
    QWidget *m_widget;
    QWidget *m_widgetParent;
    QPushButton *m_closeButton;
    int m_widgetWidth;
    int m_widgetHeight;
};

#endif // POPUPWIDGET_H
