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
 * with this program; if not, see <http://www.gnu.org/licenses/>
 */

#include "qwt/src/qwt_painter.h"
#include "expocurve.h"

ExpoCurve::ExpoCurve(QWidget *parent) :
    QwtPlot(parent)
{
    QwtPainter::setPolylineSplitting(false);
    QwtPainter::setRoundingAlignment(false);
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
    roll_elements.Curve.setPen(QPen(QColor(Qt::blue), 1.0, Qt::SolidLine,
                Qt::FlatCap));
    roll_elements.Curve.attach(this);

    pitch_elements.Curve.setRenderHint(QwtPlotCurve::RenderAntialiased);
    pitch_elements.Curve.setPen(QPen(QColor(Qt::red), 1.0, Qt::SolidLine,
                Qt::FlatCap));
    pitch_elements.Curve.attach(this);

    yaw_elements.Curve.setRenderHint(QwtPlotCurve::RenderAntialiased);
    yaw_elements.Curve.setPen(QPen(QColor(Qt::green), 1.0, Qt::SolidLine,
                Qt::FlatCap));
    yaw_elements.Curve.attach(this);

    // legend
    // Show a legend at the top
    QwtLegend *m_legend = new QwtLegend(this);
    m_legend->setDefaultItemMode(QwtLegendData::Checkable);
    //m_legend->setFrameStyle(QFrame::Box | QFrame::Sunken);
    m_legend->setToolTip(tr("Click legend to show/hide expo curve"));

    // connect signal when clicked on legend entry to function that shows/hides the curve
    connect(m_legend, SIGNAL(checked(const QVariant &, bool, int)), this, SLOT(showCurve(QVariant, bool, int)));

    QPalette pal = m_legend->palette();
    pal.setColor(m_legend->backgroundRole(), QColor(100, 100, 100));	// background colour
    pal.setColor(QPalette::Text, QColor(0, 0, 0));			// text colour
    m_legend->setPalette(pal);
    m_legend->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    insertLegend(m_legend, QwtPlot::RightLegend);

    steps = 150;
    x_data = new double[steps];
    y_data = new double[steps];

    //double step = 2 * 1.0 / (steps - 1);
    double step = 1.0 / (steps - 1);
    for (int i = 0; i < steps; i++) {
        //x_data[i] = (i * step) - 1.0;
        x_data[i] = (i * step);
    }

    //  setup of the axis title
    QwtText axis_title;

    // get the default font
    QFont axis_title_font = this->axisTitle(yLeft).font();
    // and only change the font size
    axis_title_font.setPointSize(10);
    axis_title.setFont(axis_title_font);

    axis_title.setText(tr("normalized stick input"));
    this->setAxisTitle(QwtPlot::xBottom, axis_title);

    roll_elements.Curve.setTitle(tr("Roll"));
    pitch_elements.Curve.setTitle(tr("Pitch"));
    yaw_elements.Curve.setTitle(tr("Yaw"));

    axis_title.setText(tr("rate (deg/s)"));
    this->setAxisTitle(QwtPlot::yLeft, axis_title);

    plotDataRoll(50, 720, 50);
    plotDataPitch(50, 720, 50);
    plotDataYaw(50, 720, 30);
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
void ExpoCurve::plotData(int value, int max, int exponent,
        ExpoPlotElements_t &plot_elements)
{
    for (int i = 0; i < steps; i++) {
        y_data[i] = max * (x_data[i]  * ((100 - value) / 100.0) + pow(x_data[i], exponent/10.0f) * (value / 100.0));
    }

    plot_elements.Curve.setSamples(x_data, y_data, steps);
    plot_elements.Curve.show();

    // Surely there is a less costly way to do this!
    setAxisAutoScale(yLeft, true);
    updateAxes();
    const QwtScaleDiv &div = axisScaleDiv(yLeft);
    setAxisScale(yLeft, 0, div.upperBound(), 180);

    this->replot();
}

/**
 * @brief ExpoCurve::plotDataRoll public function to plot expo data for roll axis
 * @param value The expo coefficient; sets the exponential amount [0,100]
 * @param max   The max. scaling for the axis in physical units
 */
void ExpoCurve::plotDataRoll(int value, int max, int exponent)
{
    plotData(value, max, exponent, this->roll_elements);
}

/**
 * @brief ExpoCurve::plotDataPitch public function to plot expo data for pitch axis
 * @param value The expo coefficient; sets the exponential amount [0,100]
 * @param max   The max. scaling for the axis in physical units
 */
void ExpoCurve::plotDataPitch(int value, int max, int exponent)
{
    plotData(value, max, exponent, this->pitch_elements);
}

/**
 * @brief ExpoCurve::plotDataYaw public function to plot expo data for yaw axis
 * @param value The expo coefficient; sets the exponential amount [0,100]
 * @param max   The max. scaling for the axis in physical units
 */
void ExpoCurve::plotDataYaw(int value, int max, int exponent)
{
    plotData(value, max, exponent, this->yaw_elements);
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
