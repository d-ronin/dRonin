/**
 ******************************************************************************
 *
 * @file       spectrogramplotdata.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ScopePlugin Scope Gadget Plugin
 * @{
 * @brief The scope Gadget, graphically plots the states of UAVObjects
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

#include <QDebug>
#include <math.h>

#include "extensionsystem/pluginmanager.h"
#include "uavobjectmanager.h"
#include "scopes3d/spectrogramplotdata.h"
#include "scopes3d/spectrogramscopeconfig.h"
#include "scopegadgetwidget.h"

#include "qwt/src/qwt.h"
#include "qwt/src/qwt_color_map.h"
#include "qwt/src/qwt_matrix_raster_data.h"
#include "qwt/src/qwt_plot_spectrogram.h"
#include "qwt/src/qwt_scale_draw.h"
#include "qwt/src/qwt_scale_widget.h"

#define PI 3.1415926535897932384626433832795

/**
 * @brief SpectrogramData
 * @param uavObject
 * @param uavField
 * @param samplingFrequency
 * @param windowWidth
 * @param timeHorizon
 */
SpectrogramData::SpectrogramData(QString uavObject, QString uavField, double samplingFrequency,
                                 unsigned int windowWidth, double timeHorizon)
    : Plot3dData(uavObject, uavField)
    , spectrogram(0)
    , rasterData(0)
    , fft_object(0)
{
    this->samplingFrequency = samplingFrequency;
    this->timeHorizon = timeHorizon;
    autoscaleValueUpdated = 0;

    // Create raster data
    rasterData = new QwtMatrixRasterData();

    if (mathFunction == "FFT") {
        fft_object = new ffft::FFTReal<double>(windowWidth);
        windowWidth /= 2;
    }

    this->windowWidth = windowWidth;

    rasterData->setValueMatrix(*zDataHistory, windowWidth);

    // Set the ranges for the plot
    resetAxisRanges();
    plotData.clear();
    lastInstanceIndex =
        -1; // To keep track of missing instances. We assume communications keep packet order
}

void SpectrogramData::setXMaximum(double val)
{
    xMaximum = val;

    resetAxisRanges();
}

void SpectrogramData::setYMaximum(double val)
{
    yMaximum = val;

    resetAxisRanges();
}

void SpectrogramData::setZMaximum(double val)
{
    zMaximum = val;

    resetAxisRanges();
}

void SpectrogramData::resetAxisRanges()
{
    rasterData->setInterval(Qt::XAxis, QwtInterval(xMinimum, xMaximum));
    rasterData->setInterval(Qt::YAxis, QwtInterval(yMinimum, yMaximum));
    rasterData->setInterval(Qt::ZAxis, QwtInterval(0, zMaximum));
}

/**
 * @brief SpectrogramScopeConfig::plotNewData Update plot with new data
 * @param scopeGadgetWidget
 */
void SpectrogramData::plotNewData(PlotData *plot3dData, ScopeConfig *scopeConfig,
                                  ScopeGadgetWidget *scopeGadgetWidget)
{
    Q_UNUSED(plot3dData);

    removeStaleData();

    // Check for new data
    if (readAndResetUpdatedFlag() == true) {
        // Plot new data
        rasterData->setValueMatrix(*zDataHistory, windowWidth);

        // Check autoscale. (For some reason, QwtSpectrogram doesn't support autoscale)
        if (zMaximum == 0) {
            double newVal = readAndResetAutoscaleValue();
            if (newVal != 0) {
                rightAxis->setColorMap(
                    QwtInterval(0, newVal),
                    new ColorMap(((SpectrogramScopeConfig *)scopeConfig)->getColorMap()));
                scopeGadgetWidget->setAxisScale(QwtPlot::yRight, 0, newVal);
            }
        }
    }
}

/**
 * @brief SpectrogramData::append Appends data to spectrogram
 * @param obj UAVO with new data
 * @return
 */
bool SpectrogramData::append(UAVObject *multiObj)
{
    QDateTime NOW =
        QDateTime::currentDateTime(); // TODO: Upgrade this to show UAVO time and not system time

    // Check to make sure it's the correct UAVO
    if (uavObjectName == multiObj->getName()) {

        // Only run on UAVOs that have multiple instances
        if (multiObj->isSingleInstance()) {
            return false;
        }

        // Instantiate object manager
        UAVObjectManager *objManager;

        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
        Q_ASSERT(pm != NULL);
        objManager = pm->getObject<UAVObjectManager>();
        Q_ASSERT(objManager != NULL);

        // Get list of object instances
        QVector<UAVObject *> list = objManager->getObjectInstancesVector(multiObj->getName());

        uint16_t newWindowWidth =
            list.size() * list.front()->getField(uavFieldName)->getNumElements();

        /* Check if the instance has a samples field as this will override the windowWidth
        *  Field can be used in objects that have dynamic size
        *  like the case of the Vibration Analysis modeule
        */
        QList<UAVObjectField *> fieldList = multiObj->getFields();
        foreach (UAVObjectField *field, fieldList) {
            if (field->getType() == UAVObjectField::INT16 && field->getName() == "samples") {
                newWindowWidth = field->getValue().toDouble();
                break;
            }
        }

        uint16_t valuesToProcess = newWindowWidth; // Store the number of samples expected

        // Can happen when changing the FFTP Window Width
        if (mathFunction == "FFT") {
            if (!((valuesToProcess != 0) && ((valuesToProcess & (valuesToProcess - 1)) == 0))) {
                return false;
            }
            newWindowWidth /= 2; // FFT Output is half
        }

        // Check that there is a full window worth of data. While GCS is starting up, the size of
        // multiple instance UAVOs is 1, so it's possible for spurious data to come in before
        // the flight controller board has had time to initialize the UAVO size.

        if (newWindowWidth != windowWidth) {
            windowWidth = newWindowWidth;
            clearPlots();

            plotData.clear();
            rasterData->setValueMatrix(*zDataHistory, windowWidth);

            qDebug() << "Spectrogram width adjusted to " << windowWidth;
        }

        UAVObjectField *multiField = multiObj->getField(uavFieldName);
        Q_ASSERT(multiField);
        if (multiField) {

            // Get the field of interest
            foreach (UAVObject *obj, list) {
                UAVObjectField *field = obj->getField(uavFieldName);
                int numElements = field->getNumElements();

                double scale = 1;
                QList<UAVObjectField *> fieldList = obj->getFields();
                foreach (UAVObjectField *field, fieldList) {
                    // Check if the instance has a scale field
                    if (field->getType() == UAVObjectField::FLOAT32
                        && field->getName() == "scale") {
                        scale = field->getValue().toDouble();
                        break;
                    }

                    // Check if data is ordered. If not, just discard everything
                    if (field->getType() == UAVObjectField::INT16 && field->getName() == "index") {
                        int currentIndex = field->getValue().toDouble();
                        if (currentIndex != (lastInstanceIndex + 1)) {
                            fprintf(stderr, "Out of order index. Got %d expected %d\n",
                                    currentIndex, lastInstanceIndex + 1);
                            plotData.clear();
                            lastInstanceIndex = -1; // Next index will be 0
                            return false;
                        }

                        lastInstanceIndex++;
                    }
                }

                for (int i = 0; i < numElements; i++) {
                    double currentValue =
                        field->getValue(i).toDouble() / scale; // Get the value and scale it

                    // Normally some math would go here, modifying currentValue before appending it
                    // to values
                    // .
                    // .
                    // .

                    // Last step, assign value to vector
                    plotData += currentValue;
                }

                // Check if we got enough values
                // The object instance can temporarily have more values than required
                if (plotData.size() == valuesToProcess) {
                    break;
                }
            }

            // If some instances are still missing
            if (plotData.size() != valuesToProcess) {
                return false;
            }

            // Check if the FFT needs to be calculated
            // Because this function is optional we will calculate the FFT and then
            // update the original vector. This will allow using the same code
            // to display the information.
            if (mathFunction == "FFT") {

                // Check if the fft_object was already created or needs to be updated
                // May happen if settings change after the spectrogram was created
                if (fft_object == NULL || fft_object->get_length() != valuesToProcess) {
                    if (fft_object != NULL)
                        delete fft_object;

                    fft_object = new ffft::FFTReal<double>(valuesToProcess);
                }

                // Hanning Window
                for (int i = 0; i < valuesToProcess; i++) {
                    plotData[i] *= pow(sin(PI * i / (valuesToProcess - 1)), 2);
                }

                QVector<double> fftout(valuesToProcess);

                fft_object->do_fft(&fftout[0], plotData.data()); // Do FFT
                plotData.clear(); // Clear vector

                // Lets get the magnitude and scale it.
                // mag = X * sqrt(re^2 + im^2)/n
                // X (4.2) is chosen so that the magnitude presented is similar to the acceleration
                // registered
                // although this is not 100% correct, it helps users understanding the spectrogram.
                for (unsigned int i = 0; i < valuesToProcess / 2; i++) {
                    plotData << 4.2
                            * sqrt(pow(fftout[i], 2) + pow(fftout[valuesToProcess / 2 + i], 2))
                            / valuesToProcess;
                }
            }

            // Apply autoscale if enabled
            if (zMaximum == 0) {
                for (unsigned int i = 0; i < windowWidth; i++) {
                    // See if autoscale is turned on and if the value exceeds the maximum for the
                    // scope.
                    if (plotData[i] > rasterData->interval(Qt::ZAxis).maxValue()) {
                        // Change scope maximum and color depth
                        rasterData->setInterval(Qt::ZAxis, QwtInterval(0, plotData[i]));
                        autoscaleValueUpdated = plotData[i];
                    }
                }
            }

            timeDataHistory->append(NOW.toTime_t() + NOW.time().msec() / 1000.0);
            while (timeDataHistory->back() - timeDataHistory->front() > timeHorizon) {
                timeDataHistory->pop_front();
                zDataHistory->remove(0, fminl(windowWidth, zDataHistory->size()));
            }

            *zDataHistory << plotData;
            plotData.clear();
            lastInstanceIndex = -1; // Next index will be 0

            return true;
        }
    }

    return false;
}

/**
 * @brief SpectrogramScopeConfig::deletePlots Delete all plot data
 */
void SpectrogramData::deletePlots(PlotData *spectrogramData)
{
    spectrogram->detach();

    // Don't delete raster data, this is done by the spectrogram's destructor
    /* delete rasterData; */

    // Delete spectrogram (also deletes raster data)
    delete spectrogram;
    delete spectrogramData;
}

/**
 * @brief SpectrogramScopeConfig::clearPlots Clear all plot data
 */
void SpectrogramData::clearPlots()
{
    timeDataHistory->clear();
    zDataHistory->clear();

    resetAxisRanges();
}
