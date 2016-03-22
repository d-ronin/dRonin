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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
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


/**
 * @brief SpectrogramData
 * @param uavObject
 * @param uavField
 * @param samplingFrequency
 * @param windowWidth
 * @param timeHorizon
 */
SpectrogramData::SpectrogramData(QString uavObject, QString uavField, double samplingFrequency, unsigned int windowWidth, double timeHorizon)
        : Plot3dData(uavObject, uavField),
          spectrogram(0),
          rasterData(0),
          fft_object(0)
{
    this->samplingFrequency = samplingFrequency;
    this->timeHorizon = timeHorizon;
    autoscaleValueUpdated = 0;

    // Create raster data
    rasterData = new QwtMatrixRasterData();
    
    if(mathFunction == "FFT") {
        fft_object = new ffft::FFTReal<double>(windowWidth);
        windowWidth /= 2;
    }

    this->windowWidth = windowWidth;

    rasterData->setValueMatrix( *zDataHistory, windowWidth );

    // Set the ranges for the plot
    resetAxisRanges();
}

void SpectrogramData::setXMaximum(double val)
{
    xMaximum=val;

    resetAxisRanges();
}

void SpectrogramData::setYMaximum(double val)
{
    yMaximum=val;

    resetAxisRanges();
}

void SpectrogramData::setZMaximum(double val)
{
    zMaximum=val;

    resetAxisRanges();
}

void SpectrogramData::resetAxisRanges()
{
    rasterData->setInterval( Qt::XAxis, QwtInterval(xMinimum, xMaximum));
    rasterData->setInterval( Qt::YAxis, QwtInterval(yMinimum, yMaximum));
    rasterData->setInterval( Qt::ZAxis, QwtInterval(0, zMaximum));
}


/**
 * @brief SpectrogramScopeConfig::plotNewData Update plot with new data
 * @param scopeGadgetWidget
 */
void SpectrogramData::plotNewData(PlotData *plot3dData, ScopeConfig *scopeConfig, ScopeGadgetWidget *scopeGadgetWidget)
{
    Q_UNUSED(plot3dData);

    removeStaleData();

    // Check for new data
    if (readAndResetUpdatedFlag() == true){
        // Plot new data
        rasterData->setValueMatrix(*zDataHistory, windowWidth);

        // Check autoscale. (For some reason, QwtSpectrogram doesn't support autoscale)
        if (zMaximum == 0){
            double newVal = readAndResetAutoscaleValue();
            if (newVal != 0){
                rightAxis->setColorMap( QwtInterval(0, newVal), new ColorMap(((SpectrogramScopeConfig*) scopeConfig)->getColorMap()));
                scopeGadgetWidget->setAxisScale( QwtPlot::yRight, 0, newVal);
            }
        }
    }
}


/**
 * @brief SpectrogramData::append Appends data to spectrogram
 * @param obj UAVO with new data
 * @return
 */
bool SpectrogramData::append(UAVObject* multiObj)
{
    QDateTime NOW = QDateTime::currentDateTime(); //TODO: Upgrade this to show UAVO time and not system time

    // Check to make sure it's the correct UAVO
    if (uavObjectName == multiObj->getName()) {

        // Only run on UAVOs that have multiple instances
        if (multiObj->isSingleInstance())
            return false;

        //Instantiate object manager
        UAVObjectManager *objManager;

        ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
        Q_ASSERT(pm != NULL);
        objManager = pm->getObject<UAVObjectManager>();
        Q_ASSERT(objManager != NULL);


        // Get list of object instances
        QVector<UAVObject*> list = objManager->getObjectInstancesVector(multiObj->getName());

        int spectrogramWidth = list.size() * list.front()->getField(uavFieldName)->getNumElements();
        if(mathFunction == "FFT") {
            if ( !((spectrogramWidth != 0) && ((spectrogramWidth & (~spectrogramWidth + 1)) == spectrogramWidth)) ) {
                qDebug() << "Spectrogram width not power of 2. Got " << spectrogramWidth << ". Ignoring";
                return false;
            }
            spectrogramWidth /= 2;
        }

        // Check that there is a full window worth of data. While GCS is starting up, the size of
        // multiple instance UAVOs is 1, so it's possible for spurious data to come in before
        // the flight controller board has had time to initialize the UAVO size.
        if ( ((unsigned int) spectrogramWidth) != windowWidth){
            qDebug() << "Different data set in" << multiObj->getName() << "." << uavFieldName <<  "spectrogram: " << spectrogramWidth << " samples provided, but expected " << windowWidth;
            windowWidth = spectrogramWidth;

            clearPlots();

            rasterData->setValueMatrix(*zDataHistory, windowWidth);

            if(mathFunction == "FFT") {
                qDebug() << "Initializing new FFT with N=" << windowWidth;
                delete fft_object;
                fft_object = new ffft::FFTReal<double>(windowWidth * 2);
            }

            qDebug() << "Spectrogram width adjusted to " << windowWidth;
        }

        //Initialize vector where we will read out an entire row of multiple instance UAVO
        QVector<double> values;

        timeDataHistory->append(NOW.toTime_t() + NOW.time().msec() / 1000.0);
        UAVObjectField* multiField =  multiObj->getField(uavFieldName);
        Q_ASSERT(multiField);
        if (multiField ) {

            // Get the field of interest
            foreach (UAVObject *obj, list) {
                UAVObjectField* field = obj->getField(uavFieldName);
                int numElements = field->getNumElements();

                for(int i=0; i<numElements; i++){
                    double currentValue = field->getValue(i).toDouble();
                    //Normally some math would go here, modifying vecVal before appending it to values
                    // .
                    // .
                    // .

                    // Last step, assign value to vector
                    values += currentValue;
                }
            }

            if (mathFunction == "FFT") {
                QVector<double> fftout(windowWidth * 2);
                fft_object->do_fft(&fftout[0], values.data());
                fftout.resize(windowWidth);
                values.clear();
                values << fftout;
            }
            
            for (unsigned int i=0; i<windowWidth; i++) {
                 // See if autoscale is turned on and if the value exceeds the maximum for the scope.
                if ( zMaximum == 0 &&  values[i] > rasterData->interval(Qt::ZAxis).maxValue()){
                    // Change scope maximum and color depth
                    rasterData->setInterval(Qt::ZAxis, QwtInterval(0, values[i]) );
                    autoscaleValueUpdated = values[i];
                }
            }

            while (timeDataHistory->back() - timeDataHistory->front() > timeHorizon){
                timeDataHistory->pop_front();
                zDataHistory->remove(0, fminl(windowWidth, zDataHistory->size()));
            }
            
            *zDataHistory << values;

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
