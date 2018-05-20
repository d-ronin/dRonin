#ifndef INPUTCHANNELFORM_H
#define INPUTCHANNELFORM_H

#include <QWidget>
#include "configinputwidget.h"
namespace Ui {
class InputChannelForm;
}

class inputChannelForm : public ConfigTaskWidget
{
    Q_OBJECT

public:
    typedef enum { CHANNELFUNC_RC, CHANNELFUNC_RSSI } ChannelFunc;

    explicit inputChannelForm(QWidget *parent = nullptr, bool showlegend = false, bool showSlider = true,
                              ChannelFunc chanType = CHANNELFUNC_RC);
    ~inputChannelForm();
    friend class ConfigInputWidget;
    void setName(QString &name);
    bool assigned();

Q_SIGNALS:
    void assignmentChanged();

private slots:
    void minMaxUpdated();
    void groupUpdated();
    void channelDropdownUpdated(int);
    void channelNumberUpdated(int);
    void reverseChannel();

private:
    Ui::InputChannelForm *ui;
    ChannelFunc m_chanType;
    QSpinBox *sbChannelCurrent;
};

#endif // INPUTCHANNELFORM_H
