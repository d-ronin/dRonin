
/**
 ******************************************************************************
 * @file       expocurve.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2015
 * @author     dRonin, http://dronin.org Copyright (C) 2015
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
 * @brief Visualize the expo seettings
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

#ifndef EXPOCURVE_H
#define EXPOCURVE_H

#include <QWidget>
#ifndef QWT_DLL
#define QWT_DLL
#endif
#include "qwt/src/qwt.h"
#include "qwt/src/qwt_plot.h"
#include "qwt/src/qwt_plot_curve.h"
#include "qwt/src/qwt_scale_draw.h"
#include "qwt/src/qwt_scale_widget.h"
#include "qwt/src/qwt_plot_grid.h"
#include "qwt/src/qwt_legend.h"
#include "qwt/src/qwt_legend_label.h"
#include "qwt/src/qwt_plot_marker.h"
#include "qwt/src/qwt_symbol.h"

class ExpoCurve : public QwtPlot
{
    Q_OBJECT
public:
    explicit ExpoCurve(QWidget *parent = 0);

    typedef struct ExpoPlotElements
    {
        QwtPlotCurve Curve;
    } ExpoPlotElements_t;

    void init();

    //! Show expo data for one of the stick channels
    void plotData(int value, int max, int exponent, ExpoPlotElements_t &plot_elements);

public slots:

    //! Show expo data for roll
    void plotDataRoll(int value, int max, int exponent);

    //! Show expo data for pitch
    void plotDataPitch(int value, int max, int exponent);

    //! Show expo data for yaw
    void plotDataYaw(int value, int max, int exponent);

    //! Show/Hide a expo curve and markers
    void showCurve(const QVariant &itemInfo, bool on, int index);

signals:

private:
    int steps;
    int curve_cnt;
    double *x_data;
    double *y_data;

    ExpoPlotElements_t roll_elements;
    ExpoPlotElements_t pitch_elements;
    ExpoPlotElements_t yaw_elements;
};

#endif // EXPOCURVE_H
