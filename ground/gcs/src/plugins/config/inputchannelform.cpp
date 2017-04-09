#include "inputchannelform.h"
#include "ui_inputchannelform.h"

#include "manualcontrolsettings.h"
#include "gcsreceiver.h"

#include <coreplugin/iboardtype.h>

inputChannelForm::inputChannelForm(QWidget *parent, bool showlegend, bool showSlider,
                                   ChannelFunc chanType)
    : ConfigTaskWidget(parent)
    , ui(new Ui::InputChannelForm)
    , m_chanType(chanType)
{
    ui->setupUi(this);

    // The first time through the loop, keep the legend. All other times, delete it.
    if (!showlegend) {
        layout()->removeWidget(ui->legend0);
        layout()->removeWidget(ui->legend1);
        layout()->removeWidget(ui->legend2);
        layout()->removeWidget(ui->legend3);
        layout()->removeWidget(ui->legend4);
        layout()->removeWidget(ui->legend5);
        layout()->removeWidget(ui->legend6);
        layout()->removeWidget(ui->lblReverse);
        delete ui->legend0;
        delete ui->legend1;
        delete ui->legend2;
        delete ui->legend3;
        delete ui->legend4;
        delete ui->legend5;
        delete ui->legend6;
        delete ui->lblReverse;
    }

    if (!showSlider) {
        ui->channelNeutral->setHidden(true);
    }

    // setup a hidden widget to track channel value without too much pain
    sbChannelCurrent = new QSpinBox(this);
    sbChannelCurrent->setVisible(false);
    sbChannelCurrent->setMaximum(65535);

    // Connect slots
    connect(ui->channelMin, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &inputChannelForm::minMaxUpdated);
    connect(ui->channelMax, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &inputChannelForm::minMaxUpdated);
    connect(ui->channelGroup, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &inputChannelForm::groupUpdated);
    connect(sbChannelCurrent, QOverload<int>::of(&QSpinBox::valueChanged), ui->channelNeutral,
            &TextBubbleSlider::setIndicatorValue);
    connect(ui->btnReverse, &QAbstractButton::released, this, &inputChannelForm::reverseChannel);

    // This is awkward but since we want the UI to be a dropdown but the field is not an enum
    // so it breaks the UAUVObject widget relation of the task gadget.  Running the data through
    // a spin box fixes this
    connect(ui->channelNumberDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &inputChannelForm::channelDropdownUpdated);
    connect(ui->channelNumber, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &inputChannelForm::channelNumberUpdated);

    disableMouseWheelEvents();
}

inputChannelForm::~inputChannelForm()
{
    delete ui;
}

void inputChannelForm::setName(QString &name)
{
    ui->channelName->setText(name);
    QFontMetrics metrics(ui->channelName->font());
    int width = metrics.width(name) + 5;
    foreach (inputChannelForm *form, parent()->findChildren<inputChannelForm *>()) {
        if (form == this)
            continue;
        if (form->ui->channelName->minimumSize().width() < width)
            form->ui->channelName->setMinimumSize(width, 0);
        else
            width = form->ui->channelName->minimumSize().width();
    }
    ui->channelName->setMinimumSize(width, 0);
}

/**
  * Update the direction of the slider and boundaries
  */
void inputChannelForm::minMaxUpdated()
{
    if (ui->channelMin->value() != ui->channelMax->value()) {
        bool reverse = (ui->channelMin->value() > ui->channelMax->value());
        if (reverse) {
            ui->channelNeutral->setMinimum(ui->channelMax->value());
            ui->channelNeutral->setMaximum(ui->channelMin->value());

            // Set the QSlider groove colors so that the fill is on the side of the minimum value
            ui->channelNeutral->setProperty("state", "inverted");
        } else {
            ui->channelNeutral->setMinimum(ui->channelMin->value());
            ui->channelNeutral->setMaximum(ui->channelMax->value());

            // Set the QSlider groove colors so that the fill is on the side of the minimum value
            ui->channelNeutral->setProperty("state", "normal");
        }
        ui->channelNeutral->setInvertedAppearance(reverse);

        // make sure slider is enabled (can be removed when "else" below is removed)
        ui->channelNeutral->setEnabled(true);
    } else {
        // when the min and max is equal, disable this slider to prevent crash
        // from Qt bug: https://bugreports.qt.io/browse/QTBUG-43398
        ui->channelNeutral->setMinimum(ui->channelMin->value() - 1);
        ui->channelNeutral->setMaximum(ui->channelMax->value() + 1);
        ui->channelNeutral->setEnabled(false);
    }

    // Force refresh of style sheet.
    ui->channelNeutral->setStyle(QApplication::style());
}

/**
  * Update the channel options based on the selected receiver type
  *
  * I fully admit this is terrible practice to embed data within UI
  * like this.  Open to suggestions. JC 2011-09-07
  */
void inputChannelForm::groupUpdated()
{
    int selected = ui->channelNumberDropdown->currentIndex();
    ui->channelNumberDropdown->clear();
    ui->channelNumberDropdown->addItem("Disabled");

    quint8 count = 0;

    switch (m_chanType) {

    case inputChannelForm::CHANNELFUNC_RC: {
        switch (ui->channelGroup->currentIndex()) {
        case -1: // Nothing selected
            count = 0;
            break;
        case ManualControlSettings::CHANNELGROUPS_PWM:
            count = 8; // Need to make this 6 for CC
            break;
        case ManualControlSettings::CHANNELGROUPS_PPM:
        case ManualControlSettings::CHANNELGROUPS_DSM:
            count = 12;
            break;
        case ManualControlSettings::CHANNELGROUPS_SBUS:
            count = 18;
            break;
        case ManualControlSettings::CHANNELGROUPS_RFM22B:
            count = 9;
            break;
        case ManualControlSettings::CHANNELGROUPS_OPENLRS:
            count = 8;
            break;
        case ManualControlSettings::CHANNELGROUPS_GCS:
            count = GCSReceiver::CHANNEL_NUMELEM;
            break;
        case ManualControlSettings::CHANNELGROUPS_HOTTSUM:
            count = 32;
            break;
        case ManualControlSettings::CHANNELGROUPS_SRXL:
            count = 16;
            break;
        case ManualControlSettings::CHANNELGROUPS_IBUS:
            count = 10;
            break;
        case ManualControlSettings::CHANNELGROUPS_TBSCROSSFIRE:
            count = 12;
            break;
        case ManualControlSettings::CHANNELGROUPS_NONE:
            count = 0;
            break;
        default:
            Q_ASSERT(0);
        }
    } break;

    case inputChannelForm::CHANNELFUNC_RSSI: {
        switch (ui->channelGroup->currentIndex()) {
        case -1: // Nothing selected
            count = 0;
            break;
        case ManualControlSettings::RSSITYPE_PWM:
            count = 8;
            break;
        case ManualControlSettings::RSSITYPE_PPM:
            count = 12;
            break;
        case ManualControlSettings::RSSITYPE_SBUS:
            count = 18;
            break;
        case ManualControlSettings::RSSITYPE_HOTTSUM:
            count = 32;
            break;
        case ManualControlSettings::RSSITYPE_ADC:
            count = 9;
            break;
        case ManualControlSettings::RSSITYPE_OPENLRS:
            count = 8;
            break;
        case ManualControlSettings::RSSITYPE_RFM22B:
            count = 1;
            break;
        case ManualControlSettings::RSSITYPE_FRSKYPWM:
            count = 1;
            break;
        case ManualControlSettings::RSSITYPE_TBSCROSSFIRE:
            count = 12;
            break;
        case ManualControlSettings::RSSITYPE_NONE:
            count = 0;
            break;
        default:
            Q_ASSERT(0);
        }
    } break;
    }

    ui->channelNumber->setMaximum(count);
    ui->channelNumber->setMinimum(0);

    if (m_chanType == inputChannelForm::CHANNELFUNC_RSSI
        && ui->channelGroup->currentIndex() == ManualControlSettings::RSSITYPE_ADC) {
        QStringList names;
        Core::IBoardType *board = utilMngr->getBoardType();
        if (board)
            names = board->getAdcNames();

        for (int i = 0; i < count; i++) {
            QString name;
            if (i < names.length())
                name = names[i] + QString(" (ADC%1)").arg(i);
            else if (names.isEmpty())
                name = QString("ADC%1").arg(i);
            else
                name = QString("N/A (ADC%1)").arg(i);

            ui->channelNumberDropdown->addItem(name);
        }
    } else {
        for (int i = 0; i < count; i++)
            ui->channelNumberDropdown->addItem(QString(tr("Chan %1").arg(i + 1)));
        if (selected > 0 && selected <= count)
            ui->channelNumberDropdown->setCurrentIndex(selected);
    }
}

/**
  * Update the dropdown from the hidden control
  */
void inputChannelForm::channelDropdownUpdated(int newval)
{
    ui->channelNumber->setValue(newval);
}

/**
  * Update the hidden control from the dropdown
  */
void inputChannelForm::channelNumberUpdated(int newval)
{
    ui->channelNumberDropdown->setCurrentIndex(newval);
}

/**
 * @brief Switch the channel min and max
 */
void inputChannelForm::reverseChannel()
{
    int min = ui->channelMin->value();
    int neutral = ui->channelNeutral->value();
    int max = ui->channelMax->value();
    ui->channelMax->setValue(min);
    ui->channelMin->setValue(max);
    if (ui->channelName->text() == "Throttle") // this is bad... but...
        neutral = max - (neutral - min);
    ui->channelNeutral->setValue(neutral);
}
