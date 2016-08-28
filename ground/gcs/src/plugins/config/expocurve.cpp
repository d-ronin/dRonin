/**
 ******************************************************************************
 * @file       txpocurve.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Visualize the expo settings
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "expocurve.h"

ExpoCurve::ExpoCurve(QWidget *parent) :
    QwtPlot(parent)
{
    setMouseTracking(true);

    setMinimumSize(64, 64);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    //setCanvasBackground(QColor(64, 64, 64));

    //Add grid lines
    QwtPlotGrid *grid = new QwtPlotGrid;
    grid->setMajorPen(QPen(Qt::gray, 0, Qt::DashLine));
    grid->setMinorPen(QPen(Qt::lightGray, 0, Qt::DotLine));
    grid->setPen(QPen(Qt::darkGray, 1, Qt::DotLine));
    grid->attach(this);

    roll_elements.Curve.setRenderHint(QwtPlotCurve::RenderAntialiased);
    roll_elements.Curve.setPen(QPen(QColor(Qt::blue), 1.0));
    roll_elements.Curve.attach(this);

    pitch_elements.Curve.setRenderHint(QwtPlotCurve::RenderAntialiased);
    pitch_elements.Curve.setPen(QPen(QColor(Qt::red), 1.0));
    pitch_elements.Curve.attach(this);

    yaw_elements.Curve.setRenderHint(QwtPlotCurve::RenderAntialiased);
    yaw_elements.Curve.setPen(QPen(QColor(Qt::green), 1.0));
    yaw_elements.Curve.attach(this);

    // legend
    // Show a legend at the top
    QwtLegend *m_legend = new QwtLegend(this);
    m_legend->setDefaultItemMode(QwtLegendData::Checkable);
    m_legend->setFrameStyle(QFrame::Box | QFrame::Sunken);
    m_legend->setToolTip(tr("Click legend to show/hide expo curve"));

    // connect signal when clicked on legend entry to function that shows/hides the curve
    connect(m_legend, SIGNAL(checked(const QVariant &, bool, int)), this, SLOT(showCurve(QVariant, bool, int)));

    QPalette pal = m_legend->palette();
    pal.setColor(m_legend->backgroundRole(), QColor(100, 100, 100));	// background colour
    pal.setColor(QPalette::Text, QColor(0, 0, 0));			// text colour
    m_legend->setPalette(pal);

    insertLegend(m_legend, QwtPlot::TopLegend);
    // QwtPlot::insertLegend() changes the max columns attribute, so you have to set it to the desired number after the statement
    m_legend->setMaxColumns(3);


    steps = 1000;
    x_data =  new double[steps];
    y_data =  new double[steps];

    double step = 2 * 1.0 / (steps - 1);
    for (int i = 0; i < steps; i++) {
        x_data[i] = (i * step) - 1.0;
    }
}

/**
 * @brief ExpoCurve::init Init labels, titels, horizin transition,...
 * @param lbl_mode Chose the mode of this widget; RateCurve: for rate mode, AttitudeCurve for Attitude mode, HorizonCurve: for horizon mode
 * @param horizon_transitions value for the horizon transition markers in the plot; 0: disabled, >0: horizon transitions in % horizon (should be the same as defined in /flight/Modules/Stabilization/stabilization.c)
 */
void ExpoCurve::init()
{
    //  setup of the axis title
    QwtText axis_title;

    // get the default font
    QFont axis_title_font = this->axisTitle(yLeft).font();
    // and only change the font size
    axis_title_font.setPointSize(10);
    axis_title.setFont(axis_title_font);

    this->enableAxis(QwtPlot::yRight);

    axis_title.setText(tr(" normalized stick input"));
    this->setAxisTitle(QwtPlot::xBottom, axis_title);

    roll_elements.Curve.setTitle(tr("Roll rate (deg/s)"));
    pitch_elements.Curve.setTitle(tr("Pitch rate (deg/s)"));
    yaw_elements.Curve.setTitle(tr("Yaw rate (deg/s)"));

    axis_title.setText(tr("rate (deg/s)"));
    this->setAxisTitle(QwtPlot::yRight, axis_title);
    this->setAxisTitle(QwtPlot::yLeft, axis_title);
}

/**
 * @brief ExpoCurve::plotData Show expo data for one of the stick channels
 * @param value The expo coefficient; sets the exponential amount [0,100]
 * @param curve The curve that has to be plot (roll,nick,yaw)
 * @param mode  The mode chooses wich y-axis is used: Y_Right or Y_Left
 *
 * The math here is copied/ the same as in the expo3() function in /flight/Libraries/math/misc_math.c
 * Please be aware of changes that are made there.
 */
void ExpoCurve::plotData(int value, int max, ExpoPlotElements_t &plot_elements)
{
    for (int i = 0; i < steps; i++) {
        y_data[i] = max * (x_data[i]  * ((100 - value) / 100.0) + pow(x_data[i], 3) * (value / 100.0));
    }

    plot_elements.Curve.setSamples(x_data, y_data, steps);
    plot_elements.Curve.show();

    this->replot();

    this->setAxisScaleDiv(yRight, this->axisScaleDiv(yLeft));
    this->replot();
}

/**
 * @brief ExpoCurve::plotDataRoll public function to plot expo data for roll axis
 * @param value The expo coefficient; sets the exponential amount [0,100]
 * @param max   The max. scaling for the axis in physical units
 */
void ExpoCurve::plotDataRoll(double value, int max)
{
    plotData((int)value, max, this->roll_elements);
}

/**
 * @brief ExpoCurve::plotDataPitch public function to plot expo data for pitch axis
 * @param value The expo coefficient; sets the exponential amount [0,100]
 * @param max   The max. scaling for the axis in physical units
 */
void ExpoCurve::plotDataPitch(double value, int max)
{
    plotData((int)value, max, this->pitch_elements);
}

/**
 * @brief ExpoCurve::plotDataYaw public function to plot expo data for yaw axis
 * @param value The expo coefficient; sets the exponential amount [0,100]
 * @param max   The max. scaling for the axis in physical units
 */
void ExpoCurve::plotDataYaw(double value, int max)
{
    plotData((int)value, max, this->yaw_elements);
}

/**
 * @brief ExpoCurve::showCurve The Slot function to show/hide a curve and the corresponding markers. Called from a "checked" Signal
 * @param itemInfo Info for the item of the selected legend label
 * @param on       True when the legend label is checked
 * @param index    Index of the legend label in the list of widgets that are associated with the plot item; but not used here
 */
void ExpoCurve::showCurve(const QVariant & itemInfo, bool on, int index)
{
    Q_UNUSED(index);
    QwtPlotItem * item = QwtPlot::infoToItem(itemInfo);
    if (item) {
        item->setVisible(!on);
    }

    replot();
}
