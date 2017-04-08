/**
 ******************************************************************************
 * @file       calibration.cpp
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
 * @brief      Gui-less support class for calibration
 * @see        The GNU Public License (GPL) Version 3
 *
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup ConfigPlugin Config Plugin
 * @{
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
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */

#include "calibration.h"

#include "physical_constants.h"

#include "utils/coordinateconversions.h"
#include <QMessageBox>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <QEventLoop>
#include <QApplication>

#include "accels.h"
#include "attitudesettings.h"
#include "gyros.h"
#include "homelocation.h"
#include "magnetometer.h"
#include "sensorsettings.h"
#include "subtrimsettings.h"
#include "flighttelemetrystats.h"

#include <Eigen/Core>
#include <Eigen/Cholesky>
#include <Eigen/SVD>
#include <Eigen/QR>
#include <cstdlib>

#define META_OPERATIONS_TIMEOUT 5000

enum calibrationSuccessMessages{
    CALIBRATION_SUCCESS,
    ACCELEROMETER_FAILED,
    MAGNETOMETER_FAILED
};

#define sign(x) ((x < 0) ? -1 : 1)

Calibration::Calibration() : calibrateMags(false), accelLength(GRAVITY),
    xCurve(NULL), yCurve(NULL), zCurve(NULL)
{
}

Calibration::~Calibration()
{
}

/**
 * @brief Calibration::initialize Configure whether to calibrate the magnetometer
 * and/or accelerometer during 6-point calibration
 * @param calibrateMags
 */
void Calibration::initialize(bool calibrateAccels, bool calibrateMags) {
    this->calibrateAccels = calibrateAccels;
    this->calibrateMags = calibrateMags;
}

/**
 * @brief Calibration::connectSensor
 * @param sensor The sensor to change
 * @param con Whether to connect or disconnect to this sensor
 */
void Calibration::connectSensor(sensor_type sensor, bool con)
{
    if (con) {
        switch (sensor) {
        case ACCEL:
        {
            Accels * accels = Accels::GetInstance(getObjectManager());
            Q_ASSERT(accels);

            assignUpdateRate(accels, SENSOR_UPDATE_PERIOD);
            connect(accels, &UAVObject::objectUpdated, this, &Calibration::dataUpdated);
        }
            break;

        case MAG:
        {
            Magnetometer * mag = Magnetometer::GetInstance(getObjectManager());
            Q_ASSERT(mag);

            assignUpdateRate(mag, SENSOR_UPDATE_PERIOD);
            connect(mag, &UAVObject::objectUpdated, this, &Calibration::dataUpdated);
        }
            break;

        case GYRO:
        {
            Gyros * gyros = Gyros::GetInstance(getObjectManager());
            Q_ASSERT(gyros);

            assignUpdateRate(gyros, SENSOR_UPDATE_PERIOD);
            connect(gyros, &UAVObject::objectUpdated, this, &Calibration::dataUpdated);
        }
            break;

        }
    } else {
        switch (sensor) {
        case ACCEL:
        {
            Accels * accels = Accels::GetInstance(getObjectManager());
            Q_ASSERT(accels);

            disconnect(accels, &UAVObject::objectUpdated, this, &Calibration::dataUpdated);
        }
            break;

        case MAG:
        {
            Magnetometer * mag = Magnetometer::GetInstance(getObjectManager());
            Q_ASSERT(mag);

            disconnect(mag, &UAVObject::objectUpdated, this, &Calibration::dataUpdated);
        }
            break;

        case GYRO:
        {
            Gyros * gyros = Gyros::GetInstance(getObjectManager());
            Q_ASSERT(gyros);

            disconnect(gyros, &UAVObject::objectUpdated, this, &Calibration::dataUpdated);
        }
            break;

        }
    }
}

/**
 * @brief Calibration::assignUpdateRate Assign a new update rate. The new metadata is sent
 * to the flight controller board in a separate operation.
 * @param obj
 * @param updatePeriod
 */
void Calibration::assignUpdateRate(UAVObject* obj, quint32 updatePeriod)
{
    UAVDataObject *dobj = dynamic_cast<UAVDataObject*>(obj);
    Q_ASSERT(dobj);
    UAVObject::Metadata mdata = obj->getMetadata();
    UAVObject::SetFlightTelemetryUpdateMode(mdata, UAVObject::UPDATEMODE_THROTTLED);
    mdata.flightTelemetryUpdatePeriod = static_cast<quint16>(updatePeriod);
    QEventLoop loop;
    QTimer::singleShot(META_OPERATIONS_TIMEOUT, &loop, &QEventLoop::quit);
    connect(dobj->getMetaObject(),
            QOverload<UAVObject *, bool>::of(&UAVDataObject::transactionCompleted),
            &loop, &QEventLoop::quit);
    // Show the UI is blocking
    emit calibrationBusy(true);
    obj->setMetadata(mdata);
    obj->updated();
    loop.exec();
    emit calibrationBusy(false);
}

/**
 * @brief Calibration::dataUpdated Receive updates of any connected sensors and
 * process them based on the calibration state (e.g. six point, leveling, or
 * yaw orientation.)
 * @param obj The object that was updated
 */
void Calibration::dataUpdated(UAVObject * obj) {

    if (!timer.isActive()) {
        // ignore updates that come in after the timer has expired
        return;
    }

    switch(calibration_state) {
    case IDLE:
    case SIX_POINT_WAIT1:
    case SIX_POINT_WAIT2:
    case SIX_POINT_WAIT3:
    case SIX_POINT_WAIT4:
    case SIX_POINT_WAIT5:
    case SIX_POINT_WAIT6:
        // Do nothing
        return;
    case YAW_ORIENTATION:
        // Store data while computing the yaw orientation
        // and if completed go back to the idle state
        if(storeYawOrientationMeasurement(obj)) {
            //Disconnect sensors and reset metadata
            connectSensor(ACCEL, false);
            setMetadata(originalMetaData);

            calibration_state = IDLE;

            emit showYawOrientationMessage(tr("Orientation computed"));
            emit toggleControls(true);
            emit yawOrientationProgressChanged(0);
            disconnect(&timer, &QTimer::timeout, this, &Calibration::timeout);
        }
        break;
    case LEVELING:
        // Store data while computing the level attitude
        // and if completed go back to the idle state
        if(storeLevelingMeasurement(obj)) {
            //Disconnect sensors and reset metadata
            connectSensor(GYRO, false);
            connectSensor(ACCEL, false);
            setMetadata(originalMetaData);

            calibration_state = IDLE;

            emit showLevelingMessage(tr("Level computed"));
            emit toggleControls(true);
            emit levelingProgressChanged(0);
            disconnect(&timer, &QTimer::timeout, this, &Calibration::timeout);
        }
        break;
    case SIX_POINT_COLLECT1:
        // These state collect each position for six point calibration and
        // when enough data is acquired advance to the next step
        if(storeSixPointMeasurement(obj,1)) {
            // If all the data is collected advance to the next position
            calibration_state = SIX_POINT_WAIT2;
            emit showSixPointMessage(tr("Rotate left side down and press Save Position..."));
            emit toggleSavePosition(true);
            emit updatePlane(2);
            disconnect(&timer, &QTimer::timeout, this, &Calibration::timeout);
        }
        break;
    case SIX_POINT_COLLECT2:
        if(storeSixPointMeasurement(obj,2)) {
            // If all the data is collected advance to the next position
            calibration_state = SIX_POINT_WAIT3;
            emit showSixPointMessage(tr("Rotate upside down and press Save Position..."));
            emit toggleSavePosition(true);
            emit updatePlane(3);
            disconnect(&timer, &QTimer::timeout, this, &Calibration::timeout);
        }
        break;
    case SIX_POINT_COLLECT3:
        if(storeSixPointMeasurement(obj,3)) {
            // If all the data is collected advance to the next position
            calibration_state = SIX_POINT_WAIT4;
            emit showSixPointMessage(tr("Point right side down and press Save Position..."));
            emit toggleSavePosition(true);
            emit updatePlane(4);
            disconnect(&timer, &QTimer::timeout, this, &Calibration::timeout);
        }
        break;
    case SIX_POINT_COLLECT4:
        if(storeSixPointMeasurement(obj,4)) {
            // If all the data is collected advance to the next position
            calibration_state = SIX_POINT_WAIT5;
            emit showSixPointMessage(tr("Point nose up and press Save Position..."));
            emit toggleSavePosition(true);
            emit updatePlane(5);
            disconnect(&timer, &QTimer::timeout, this, &Calibration::timeout);
        }
        break;
    case SIX_POINT_COLLECT5:
        if(storeSixPointMeasurement(obj,5)) {
            // If all the data is collected advance to the next position
            calibration_state = SIX_POINT_WAIT6;
            emit showSixPointMessage(tr("Point nose down and press Save Position..."));
            emit toggleSavePosition(true);
            emit updatePlane(6);
            disconnect(&timer, &QTimer::timeout, this, &Calibration::timeout);
        }
        break;
    case SIX_POINT_COLLECT6:
        // Store data points in the final position and if enough data
        // has been computed attempt to calculate the scale and bias
        // for the accel and optionally the mag.
        if(storeSixPointMeasurement(obj,6)) {
            // Disconnect signals and set to IDLE before resetting
            // the meta data to prevent coming here multiple times

            calibration_state = IDLE;
            disconnect(&timer, &QTimer::timeout, this, &Calibration::timeout);

            // All data collected.  Disconnect and reset all UAVOs, and compute value
            connectSensor(GYRO, false);
            if (calibrateAccels)
                connectSensor(ACCEL, false);
            if (calibrateMags)
                connectSensor(MAG, false);
            setMetadata(originalMetaData);

            emit toggleControls(true);
            emit updatePlane(0);
            emit sixPointProgressChanged(0);

            // Do calculation
            int ret=computeScaleBias();
            if (ret==CALIBRATION_SUCCESS) {
                // Load calibration results
                SensorSettings * sensorSettings = SensorSettings::GetInstance(getObjectManager());
                SensorSettings::DataFields sensorSettingsData = sensorSettings->getData();


                // Generate result messages
                QString accelCalibrationResults = "";
                QString magCalibrationResults = "";
                if (calibrateAccels == true) {
                    accelCalibrationResults = QString(tr("Accelerometer bias, in [m/s^2]: x=%1, y=%2, z=%3\n")).arg(sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_X], -9).arg(sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_Y], -9).arg(sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_Z], -9) +
                                              QString(tr("Accelerometer scale, in [-]:    x=%1, y=%2, z=%3\n")).arg(sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_X], -9).arg(sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_Y], -9).arg(sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_Z], -9);

                }
                if (calibrateMags == true) {
                    magCalibrationResults = QString(tr("Magnetometer bias, in [mG]: x=%1, y=%2, z=%3\n")).arg(sensorSettingsData.MagBias[SensorSettings::MAGBIAS_X], -9).arg(sensorSettingsData.MagBias[SensorSettings::MAGBIAS_Y], -9).arg(sensorSettingsData.MagBias[SensorSettings::MAGBIAS_Z], -9) +
                                            QString(tr("Magnetometer scale, in [-]: x=%4, y=%5, z=%6")).arg(sensorSettingsData.MagScale[SensorSettings::MAGSCALE_X], -9).arg(sensorSettingsData.MagScale[SensorSettings::MAGSCALE_Y], -9).arg(sensorSettingsData.MagScale[SensorSettings::MAGSCALE_Z], -9);
                }

                // Emit signal containing calibration success message
                emit showSixPointMessage(QString(tr("Calibration succeeded")) + QString("\n") + accelCalibrationResults + QString("\n") + magCalibrationResults);
            }
            else{
                //Return sensor calibration values to their original settings
                resetSensorCalibrationToOriginalValues();

                if(ret==ACCELEROMETER_FAILED){
                    emit showSixPointMessage(tr("Acceleromter calibration failed. Original values have been written back to device. Perhaps you moved too much during the calculation? Please repeat calibration."));
                }
                else if(ret==MAGNETOMETER_FAILED){
                    emit showSixPointMessage(tr("Magnetometer calibration failed. Original values have been written back to device. Perhaps you performed the calibration near iron? Please repeat calibration."));
                }
            }
        }
        break;
    case GYRO_TEMP_CAL:
        if (storeTempCalMeasurement(obj)) {
            // Disconnect and reset data and metadata
            connectSensor(GYRO, false);
            resetSensorCalibrationToOriginalValues();
            setMetadata(originalMetaData);

            calibration_state = IDLE;
            emit toggleControls(true);

            int ret = computeTempCal();
            if (ret == CALIBRATION_SUCCESS) {
                emit showTempCalMessage(tr("Temperature compensation calibration succeeded"));
            } else {
                emit showTempCalMessage(tr("Temperature compensation calibration succeeded"));
            }
        }
    }

}

/**
 * @brief Calibration::timeout When collecting data for leveling, orientation, or
 * six point calibration times out. Clean up the state and reset
 */
void Calibration::timeout()
{
    // Reset metadata update rates
    setMetadata(originalMetaData);

    switch(calibration_state) {
    case IDLE:
        // Do nothing
        return;
    case YAW_ORIENTATION:
        // Disconnect appropriate sensors
        connectSensor(ACCEL, false);
        calibration_state = IDLE;
        emit showYawOrientationMessage(tr("Orientation timed out ..."));
        emit yawOrientationProgressChanged(0);
        break;
    case LEVELING:
        // Disconnect appropriate sensors
        connectSensor(GYRO, false);
        connectSensor(ACCEL, false);
        calibration_state = IDLE;
        emit showLevelingMessage(tr("Leveling timed out ..."));
        emit levelingProgressChanged(0);
        break;
    case SIX_POINT_WAIT1:
    case SIX_POINT_WAIT2:
    case SIX_POINT_WAIT3:
    case SIX_POINT_WAIT4:
    case SIX_POINT_WAIT5:
    case SIX_POINT_WAIT6:
        // Do nothing, shouldn't happen
        return;
    case SIX_POINT_COLLECT1:
    case SIX_POINT_COLLECT2:
    case SIX_POINT_COLLECT3:
    case SIX_POINT_COLLECT4:
    case SIX_POINT_COLLECT5:
    case SIX_POINT_COLLECT6:
        connectSensor(GYRO, false);
        if (calibrateAccels)
            connectSensor(ACCEL, false);
        if (calibrateMags)
            connectSensor(MAG, false);
        calibration_state = IDLE;
        emit showSixPointMessage(tr("Six point data collection timed out"));
        emit sixPointProgressChanged(0);
        break;
    case GYRO_TEMP_CAL:
        connectSensor(GYRO, false);
        calibration_state = IDLE;
        emit showTempCalMessage(tr("Temperature calibration timed out"));
        emit tempCalProgressChanged(0);
        break;
    }

    emit updatePlane(0);
    emit toggleControls(true);

    disconnect(&timer,&QTimer::timeout,this,&Calibration::timeout);

    QMessageBox msgBox;
    msgBox.setText(tr("Calibration timed out before receiving required updates."));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

/**
 * @brief Calibration::doStartLeveling Called by UI to start collecting data to calculate level
 */
void Calibration::doStartOrientation() {
    accel_accum_x.clear();
    accel_accum_y.clear();
    accel_accum_z.clear();

    calibration_state = YAW_ORIENTATION;

    // Update sensor rates
    originalMetaData = getObjectUtilManager()->readAllNonSettingsMetadata();
    connectSensor(GYRO, false);
    connectSensor(MAG, false);
    connectSensor(ACCEL, true);

    emit toggleControls(false);
    emit showYawOrientationMessage(tr("Pitch vehicle forward approximately 30 degrees. Ensure it absolutely does not roll"));

    // Set up timeout timer
    timer.setSingleShot(true);
    timer.start(8000 + (NUM_SENSOR_UPDATES_YAW_ORIENTATION * SENSOR_UPDATE_PERIOD));
    connect(&timer,&QTimer::timeout,this,&Calibration::timeout);
}

//! Start collecting data while vehicle is level
void Calibration::doStartBiasAndLeveling()
{
    zeroVertical = true;
    doStartLeveling();
}

//! Start collecting data while vehicle is level
void Calibration::doStartNoBiasLeveling()
{
    zeroVertical = false;
    doStartLeveling();
}

/**
 * @brief Calibration::doStartLeveling Called by UI to start collecting data to calculate level
 */
void Calibration::doStartLeveling() {
    accel_accum_x.clear();
    accel_accum_y.clear();
    accel_accum_z.clear();
    gyro_accum_x.clear();
    gyro_accum_y.clear();
    gyro_accum_z.clear();
    gyro_accum_temp.clear();

    // Disable gyro bias correction to see raw data
    AttitudeSettings *attitudeSettings = AttitudeSettings::GetInstance(getObjectManager());
    Q_ASSERT(attitudeSettings);
    AttitudeSettings::DataFields attitudeSettingsData = attitudeSettings->getData();
    attitudeSettingsData.BiasCorrectGyro = AttitudeSettings::BIASCORRECTGYRO_FALSE;
    attitudeSettings->setData(attitudeSettingsData);
    attitudeSettings->updated();

    calibration_state = LEVELING;

    // Connect to the sensor updates and set higher rates
    originalMetaData = getObjectUtilManager()->readAllNonSettingsMetadata();
    connectSensor(MAG, false);
    connectSensor(ACCEL, true);
    connectSensor(GYRO, true);

    emit toggleControls(false);
    emit showLevelingMessage(tr("Leave vehicle flat"));

    // Set up timeout timer
    timer.setSingleShot(true);
    timer.start(8000 + (NUM_SENSOR_UPDATES_LEVELING * SENSOR_UPDATE_PERIOD));
    connect(&timer,&QTimer::timeout,this,&Calibration::timeout);
}

/**
  * Called by the "Start" button.  Sets up the meta data and enables the
  * buttons to perform six point calibration of the magnetometer (optionally
  * accel) to compute the scale and bias of this sensor based on the current
  * home location magnetic strength.
  */
void Calibration::doStartSixPoint()
{
    // Save initial sensor settings
    SensorSettings * sensorSettings = SensorSettings::GetInstance(getObjectManager());
    Q_ASSERT(sensorSettings);
    SensorSettings::DataFields sensorSettingsData = sensorSettings->getData();

    // Compute the board rotation matrix, so we can undo the rotation
    AttitudeSettings * attitudeSettings = AttitudeSettings::GetInstance(getObjectManager());
    Q_ASSERT(attitudeSettings);
    AttitudeSettings::DataFields attitudeSettingsData = attitudeSettings->getData();
    double rpy[3] = { attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_ROLL] * DEG2RAD / 100.0,
                      attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_PITCH] * DEG2RAD / 100.0,
                      attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_YAW] * DEG2RAD / 100.0};
    Euler2R(rpy, boardRotationMatrix);

    // If calibrating the accelerometer, remove any scaling
    if (calibrateAccels) {
        initialAccelsScale[0]=sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_X];
        initialAccelsScale[1]=sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_Y];
        initialAccelsScale[2]=sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_Z];
        initialAccelsBias[0]=sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_X];
        initialAccelsBias[1]=sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_Y];
        initialAccelsBias[2]=sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_Z];

        // Reset the scale and bias to get a correct result
        sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_X] = 1.0;
        sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_Y] = 1.0;
        sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_Z] = 1.0;
        sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_X] = 0.0;
        sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_Y] = 0.0;
        sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_Z] = 0.0;
        sensorSettingsData.ZAccelOffset = 0.0;
    }

    // If calibrating the magnetometer, remove any scaling
    if (calibrateMags) {
        initialMagsScale[0]=sensorSettingsData.MagScale[SensorSettings::MAGSCALE_X];
        initialMagsScale[1]=sensorSettingsData.MagScale[SensorSettings::MAGSCALE_Y];
        initialMagsScale[2]=sensorSettingsData.MagScale[SensorSettings::MAGSCALE_Z];
        initialMagsBias[0]= sensorSettingsData.MagBias[SensorSettings::MAGBIAS_X];
        initialMagsBias[1]= sensorSettingsData.MagBias[SensorSettings::MAGBIAS_Y];
        initialMagsBias[2]= sensorSettingsData.MagBias[SensorSettings::MAGBIAS_Z];

        // Reset the scale to get a correct result
        sensorSettingsData.MagScale[SensorSettings::MAGSCALE_X] = 1;
        sensorSettingsData.MagScale[SensorSettings::MAGSCALE_Y] = 1;
        sensorSettingsData.MagScale[SensorSettings::MAGSCALE_Z] = 1;
        sensorSettingsData.MagBias[SensorSettings::MAGBIAS_X] = 0;
        sensorSettingsData.MagBias[SensorSettings::MAGBIAS_Y] = 0;
        sensorSettingsData.MagBias[SensorSettings::MAGBIAS_Z] = 0;
    }

    sensorSettings->setData(sensorSettingsData);

    // Clear the accumulators
    accel_accum_x.clear();
    accel_accum_y.clear();
    accel_accum_z.clear();
    mag_accum_x.clear();
    mag_accum_y.clear();
    mag_accum_z.clear();

    // TODO: Document why the thread needs to wait 100ms.
    QThread::usleep(100000);

    connectSensor(ACCEL, false);
    connectSensor(GYRO, false);
    connectSensor(MAG, false);

    // Connect sensors and set higher update rate
    if (calibrateAccels)
        connectSensor(ACCEL, true);
    if(calibrateMags)
        connectSensor(MAG, true);

    // Set new metadata

    // Show UI parts and update the calibration state
    emit showSixPointMessage(tr("Place horizontally and click save position..."));
    emit updatePlane(1);
    emit toggleControls(false);
    emit toggleSavePosition(true);
    calibration_state = SIX_POINT_WAIT1;
}


/**
 * @brief Calibration::doCancelSixPoint Cancels six point calibration and returns all values to their original settings.
 */
void Calibration::doCancelSixPoint(){
    //Return sensor calibration values to their original settings
    resetSensorCalibrationToOriginalValues();

    // Disconnect sensors and reset UAVO update rates
    if (calibrateAccels)
        connectSensor(ACCEL, false);
    if(calibrateMags)
        connectSensor(MAG, false);

    setMetadata(originalMetaData);

    calibration_state = IDLE;
    emit toggleControls(true);
    emit toggleSavePosition(false);
    emit updatePlane(0);
    emit sixPointProgressChanged(0);
    disconnect(&timer,&QTimer::timeout,this,&Calibration::timeout);

    emit showSixPointMessage(tr("Calibration canceled."));

}


/**
  * Tells the calibration utility the UAV is in position and to collect data.
  */
void Calibration::doSaveSixPointPosition()
{
    switch(calibration_state) {
    case SIX_POINT_WAIT1:
        emit showSixPointMessage(tr("Hold..."));
        emit toggleControls(false);
        calibration_state = SIX_POINT_COLLECT1;
        break;
    case SIX_POINT_WAIT2:
        emit showSixPointMessage(tr("Hold..."));
        emit toggleControls(false);
        calibration_state = SIX_POINT_COLLECT2;
        break;
    case SIX_POINT_WAIT3:
        emit showSixPointMessage(tr("Hold..."));
        emit toggleControls(false);
        calibration_state = SIX_POINT_COLLECT3;
        break;
    case SIX_POINT_WAIT4:
        emit showSixPointMessage(tr("Hold..."));
        emit toggleControls(false);
        calibration_state = SIX_POINT_COLLECT4;
        break;
    case SIX_POINT_WAIT5:
        emit showSixPointMessage(tr("Hold..."));
        emit toggleControls(false);
        calibration_state = SIX_POINT_COLLECT5;
        break;
    case SIX_POINT_WAIT6:
        emit showSixPointMessage(tr("Hold..."));
        emit toggleControls(false);
        calibration_state = SIX_POINT_COLLECT6;
        break;
    default:
        return;
    }

    emit toggleSavePosition(false);

    // Set up timeout timer
    timer.setSingleShot(true);
    timer.start(8000 + (NUM_SENSOR_UPDATES_SIX_POINT * SENSOR_UPDATE_PERIOD));
    connect(&timer,&QTimer::timeout,this,&Calibration::timeout);
}

/**
 * Start collecting gyro calibration data
 */
void Calibration::doStartTempCal()
{
    gyro_accum_x.clear();
    gyro_accum_y.clear();
    gyro_accum_z.clear();
    gyro_accum_temp.clear();

    // Disable gyro sensor bias correction to see raw data
    AttitudeSettings *attitudeSettings = AttitudeSettings::GetInstance(getObjectManager());
    Q_ASSERT(attitudeSettings);
    AttitudeSettings::DataFields attitudeSettingsData = attitudeSettings->getData();
    attitudeSettingsData.BiasCorrectGyro = AttitudeSettings::BIASCORRECTGYRO_FALSE;

    attitudeSettings->setData(attitudeSettingsData);
    attitudeSettings->updated();

    // compute board rotation matrix
    double rpy[3] = { attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_ROLL] * DEG2RAD / 100.0,
                      attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_PITCH] * DEG2RAD / 100.0,
                      attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_YAW] * DEG2RAD / 100.0};
    Euler2R(rpy, boardRotationMatrix);

    calibration_state = GYRO_TEMP_CAL;

    // Set up the data rates
    originalMetaData = getObjectUtilManager()->readAllNonSettingsMetadata();
    connectSensor(GYRO, true);

    emit toggleControls(false);
    emit showTempCalMessage(tr("Leave board flat and very still while it changes temperature"));
    emit tempCalProgressChanged(0);

    // Set up timeout timer
    timer.setSingleShot(true);
    timer.start(1800000);
    connect(&timer,&QTimer::timeout,this,&Calibration::timeout);
}

/**
 * @brief Calibration::doCancelTempCalPoint Abort the temperature calibration
 */
void Calibration::doAcceptTempCal()
{
    if (calibration_state == GYRO_TEMP_CAL) {
        qDebug() << "Accepting";
        // Disconnect sensor and reset UAVO update rates
        connectSensor(GYRO, false);
        setMetadata(originalMetaData);

        calibration_state = IDLE;
        emit showTempCalMessage(tr("Temperature calibration accepted"));
        emit tempCalProgressChanged(0);
        emit toggleControls(true);

        timer.stop();
        disconnect(&timer,&QTimer::timeout,this,&Calibration::timeout);

        computeTempCal();
    }
}

/**
 * @brief Calibration::doCancelTempCalPoint Abort the temperature calibration
 */
void Calibration::doCancelTempCalPoint()
{
    if (calibration_state == GYRO_TEMP_CAL) {
        qDebug() << "Canceling";
        // Disconnect sensor and reset UAVO update rates
        connectSensor(GYRO, false);
        setMetadata(originalMetaData);

        // Reenable gyro bias correction
        AttitudeSettings *attitudeSettings = AttitudeSettings::GetInstance(getObjectManager());
        Q_ASSERT(attitudeSettings);
        AttitudeSettings::DataFields attitudeSettingsData = attitudeSettings->getData();
        attitudeSettings->setData(attitudeSettingsData);
        attitudeSettings->updated();
        QThread::usleep(100000); // Sleep 100ms to make sure the new settings values are sent

        // Reset all sensor values
        resetSensorCalibrationToOriginalValues();

        calibration_state = IDLE;
        emit showTempCalMessage(tr("Temperature calibration timed out"));
        emit tempCalProgressChanged(0);
        emit toggleControls(true);

        timer.stop();
        disconnect(&timer,&QTimer::timeout,this,&Calibration::timeout);
    }
}

/**
 * @brief Calibration::setTempCalRange Set the range for calibration
 * @param r The number of degrees that must be spanned for calibration
 * to terminate
 */
void Calibration::setTempCalRange(int r)
{
    min_temperature_range = r;
}

/**
 * @brief Calibration::storeYawOrientationMeasurement Store an accelerometer
 * measurement.
 * @return true if enough data is collected
 */
bool Calibration::storeYawOrientationMeasurement(UAVObject *obj)
{
    Accels * accels = Accels::GetInstance(getObjectManager());

    // Accumulate samples until we have _at least_ NUM_SENSOR_UPDATES_YAW_ORIENTATION samples
    if(obj->getObjID() == Accels::OBJID) {
        Accels::DataFields accelsData = accels->getData();
        accel_accum_x.append(accelsData.x);
        accel_accum_y.append(accelsData.y);
        accel_accum_z.append(accelsData.z);
    }

    // update the progress indicator
    emit yawOrientationProgressChanged((100 * accel_accum_x.size()) / NUM_SENSOR_UPDATES_YAW_ORIENTATION);

    // If we have enough samples, then stop sampling and compute the biases
    if (accel_accum_x.size() >= NUM_SENSOR_UPDATES_YAW_ORIENTATION) {
        timer.stop();
        disconnect(&timer,&QTimer::timeout,this,&Calibration::timeout);

        // Get the existing attitude settings
        AttitudeSettings *attitudeSettings = AttitudeSettings::GetInstance(getObjectManager());
        AttitudeSettings::DataFields attitudeSettingsData = attitudeSettings->getData();

        // Use sensor data without rotation, as it has already been rotated on-board.
        double a_body[3] = { listMean(accel_accum_x), listMean(accel_accum_y), listMean(accel_accum_z) };

        // Temporary variable
        double psi;

        // Solve "a_sensor = Rot(phi, theta, psi) *[0;0;-g]" for the roll (phi) and pitch (theta) values.
        // Recall that phi is in [-pi, pi] and theta is in [-pi/2, pi/2]
        psi = atan2(a_body[1], -a_body[0]);

        attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_YAW] += psi * RAD2DEG * 100.0; // Scale by 100 because units are [100*deg]

        // Wrap to [-pi, pi]
        while (attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_YAW] > 180*100)  // Scale by 100 because units are [100*deg]
            attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_YAW] -= 360*100;
        while (attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_YAW] < -180*100)
            attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_YAW] += 360*100;

        attitudeSettings->setData(attitudeSettingsData);
        attitudeSettings->updated();

        // Inform the system that the calibration process has completed
        emit calibrationCompleted();

        return true;
    }
    return false;
}


/**
 * @brief Calibration::storeLevelingMeasurement Store a measurement and if there
 * is enough data compute the level angle and gyro zero
 * @return true if enough data is collected
 */
bool Calibration::storeLevelingMeasurement(UAVObject *obj) {
    Accels * accels = Accels::GetInstance(getObjectManager());
    Gyros * gyros = Gyros::GetInstance(getObjectManager());

    // Accumulate samples until we have _at least_ NUM_SENSOR_UPDATES_LEVELING samples
    if(obj->getObjID() == Accels::OBJID) {
        Accels::DataFields accelsData = accels->getData();
        accel_accum_x.append(accelsData.x);
        accel_accum_y.append(accelsData.y);
        accel_accum_z.append(accelsData.z);
    } else if (obj->getObjID() == Gyros::OBJID) {
        Gyros::DataFields gyrosData = gyros->getData();
        gyro_accum_x.append(gyrosData.x);
        gyro_accum_y.append(gyrosData.y);
        gyro_accum_z.append(gyrosData.z);
        gyro_accum_temp.append(gyrosData.temperature);
    }

    // update the progress indicator
    emit levelingProgressChanged((100 * qMin(accel_accum_x.size(),  gyro_accum_x.size())) / NUM_SENSOR_UPDATES_LEVELING);

    // If we have enough samples, then stop sampling and compute the biases
    if (accel_accum_x.size() >= NUM_SENSOR_UPDATES_LEVELING && gyro_accum_x.size() >= NUM_SENSOR_UPDATES_LEVELING) {
        timer.stop();
        disconnect(&timer,&QTimer::timeout,this,&Calibration::timeout);

        double x_gyro_bias = listMean(gyro_accum_x);
        double y_gyro_bias = listMean(gyro_accum_y);
        double z_gyro_bias = listMean(gyro_accum_z);
        double temp = listMean(gyro_accum_temp);

        // Get the existing attitude settings
        AttitudeSettings::DataFields attitudeSettingsData = AttitudeSettings::GetInstance(getObjectManager())->getData();
        SensorSettings::DataFields sensorSettingsData = SensorSettings::GetInstance(getObjectManager())->getData();

        // Inverse rotation of sensor data, from body frame into sensor frame
        double a_body[3] = { listMean(accel_accum_x), listMean(accel_accum_y), listMean(accel_accum_z) };
        double a_sensor[3]; //! Store the sensor data without any rotation
        double Rsb[3][3];  // The initial body-frame to sensor-frame rotation
        double rpy[3] = { attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_ROLL] * DEG2RAD / 100.0,
                          attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_PITCH] * DEG2RAD / 100.0,
                          attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_YAW] * DEG2RAD / 100.0};
        Euler2R(rpy, Rsb);
        rotate_vector(Rsb, a_body, a_sensor, false);

        // Temporary variables
        double psi, theta, phi;
        Q_UNUSED(psi);
        // Keep existing yaw rotation
        psi = attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_YAW] * DEG2RAD / 100.0;

        // Solve "a_sensor = Rot(phi, theta, psi) *[0;0;-g]" for the roll (phi) and pitch (theta) values.
        // Recall that phi is in [-pi, pi] and theta is in [-pi/2, pi/2]
        phi = atan2(-a_sensor[1], -a_sensor[2]);
        theta = atan(-a_sensor[0] / (sin(phi)*a_sensor[1] + cos(phi)*a_sensor[2]));

        attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_ROLL] = static_cast<qint16>(phi * RAD2DEG * 100.0);
        attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_PITCH] = static_cast<qint16>(theta * RAD2DEG * 100.0);

        if (zeroVertical) {
            // If requested, calculate the offset in the z accelerometer that
            // would make it reflect gravity

            // Rotate the accel measurements to the new body frame
            double rpy[3] = { attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_ROLL] * DEG2RAD / 100.0,
                              attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_PITCH] * DEG2RAD / 100.0,
                              attitudeSettingsData.BoardRotation[AttitudeSettings::BOARDROTATION_YAW] * DEG2RAD / 100.0};
            double a_body_new[3];
            Euler2R(rpy, Rsb);
            rotate_vector(Rsb, a_sensor, a_body_new, false);

            // Compute the new offset to make it average accelLength(GRAVITY)
            sensorSettingsData.ZAccelOffset += -(a_body_new[2] + accelLength);
        }

        // Rotate the gyro bias from the body frame into the sensor frame
        double gyro_sensor[3];
        double gyro_body[3] = {x_gyro_bias, y_gyro_bias, z_gyro_bias};
        rotate_vector(Rsb, gyro_body, gyro_sensor, false);

        // Store these new biases, accounting for any temperature coefficients
        sensorSettingsData.XGyroTempCoeff[0] = static_cast<float>(gyro_sensor[0] -
                temp * sensorSettingsData.XGyroTempCoeff[1] -
                pow(temp,2) * sensorSettingsData.XGyroTempCoeff[2] -
                pow(temp,3) * sensorSettingsData.XGyroTempCoeff[3]);
        sensorSettingsData.YGyroTempCoeff[0] = static_cast<float>(gyro_sensor[1] -
                temp * sensorSettingsData.YGyroTempCoeff[1] -
                pow(temp,2) * sensorSettingsData.YGyroTempCoeff[2] -
                pow(temp,3) * sensorSettingsData.YGyroTempCoeff[3]);
        sensorSettingsData.ZGyroTempCoeff[0] = static_cast<float>(gyro_sensor[2] -
                temp * sensorSettingsData.ZGyroTempCoeff[1] -
                pow(temp,2) * sensorSettingsData.ZGyroTempCoeff[2] -
                pow(temp,3) * sensorSettingsData.ZGyroTempCoeff[3]);
        SensorSettings::GetInstance(getObjectManager())->setData(sensorSettingsData);

        // We offset the gyro bias by current bias to help precision
        // Disable gyro bias correction to see raw data
        AttitudeSettings *attitudeSettings = AttitudeSettings::GetInstance(getObjectManager());
        Q_ASSERT(attitudeSettings);
        attitudeSettingsData.BiasCorrectGyro = AttitudeSettings::BIASCORRECTGYRO_TRUE;
        attitudeSettings->setData(attitudeSettingsData);
        attitudeSettings->updated();

        // After recomputing the level for a frame, zero the trim settings
        SubTrimSettings *trimSettings = SubTrimSettings::GetInstance(getObjectManager());
        Q_ASSERT(trimSettings);
        SubTrimSettings::DataFields trim = trimSettings->getData();
        trim.Pitch = 0;
        trim.Roll = 0;
        trimSettings->setData(trim);

        // Inform the system that the calibration process has completed
        emit calibrationCompleted();

        return true;
    }
    return false;
}

/**
  * Grab a sample of accel or mag data while in this position and
  * store it for averaging.
  * @return true If enough data is averaged at this position
  */
bool Calibration::storeSixPointMeasurement(UAVObject * obj, int position)
{
    // Position is specified 1-6, but used as an index
    Q_ASSERT(position >= 1 && position <= 6);
    position --;

    if( calibrateAccels && obj->getObjID() == Accels::OBJID ) {
        Accels * accels = Accels::GetInstance(getObjectManager());
        Q_ASSERT(accels);
        Accels::DataFields accelsData = accels->getData();

        accel_accum_x.append(accelsData.x);
        accel_accum_y.append(accelsData.y);
        accel_accum_z.append(accelsData.z);
    }

    if( calibrateMags && obj->getObjID() == Magnetometer::OBJID) {
        Magnetometer * mag = Magnetometer::GetInstance(getObjectManager());
        Q_ASSERT(mag);
        Magnetometer::DataFields magData = mag->getData();

        mag_accum_x.append(magData.x);
        mag_accum_y.append(magData.y);
        mag_accum_z.append(magData.z);
    }

    // Update progress bar
    int progress_percentage;
    if(calibrateAccels && !calibrateMags)
        progress_percentage = (100 * accel_accum_x.size()) / NUM_SENSOR_UPDATES_SIX_POINT;
    else if(!calibrateAccels && calibrateMags)
        progress_percentage = (100 * mag_accum_x.size()) / NUM_SENSOR_UPDATES_SIX_POINT;
    else
        progress_percentage = (100 * std::min(mag_accum_x.size(), accel_accum_x.size())) / NUM_SENSOR_UPDATES_SIX_POINT;
    emit sixPointProgressChanged(progress_percentage);

    // If enough data is collected, average it for this position
    if((!calibrateAccels || accel_accum_x.size() >= NUM_SENSOR_UPDATES_SIX_POINT) &&
            (!calibrateMags || mag_accum_x.size() >= NUM_SENSOR_UPDATES_SIX_POINT)) {

        // Store the average accelerometer value in that position
        if (calibrateAccels) {
            // undo the board rotation that has been applied to the sensor values
            double accel_body[3] = {listMean(accel_accum_x), listMean(accel_accum_y), listMean(accel_accum_z)};
            double accel_sensor[3];
            rotate_vector(boardRotationMatrix, accel_body, accel_sensor, false);

            accel_data_x[position] = accel_sensor[0];
            accel_data_y[position] = accel_sensor[1];
            accel_data_z[position] = accel_sensor[2];
            accel_accum_x.clear();
            accel_accum_y.clear();
            accel_accum_z.clear();
        }

        // Store the average magnetometer value in that position
        if (calibrateMags) {
            // undo the board rotation that has been applied to the sensor values
            double mag_body[3] = {listMean(mag_accum_x), listMean(mag_accum_y), listMean(mag_accum_z)};
            double mag_sensor[3];
            rotate_vector(boardRotationMatrix, mag_body, mag_sensor, false);

            mag_data_x[position] = mag_sensor[0];
            mag_data_y[position] = mag_sensor[1];
            mag_data_z[position] = mag_sensor[2];
            mag_accum_x.clear();
            mag_accum_y.clear();
            mag_accum_z.clear();
        }

        // Indicate all data collected for this position
        return true;
    }
    return false;
}

/**
 * @brief Calibration::configureTempCurves
 * @param x
 * @param y
 * @param z
 */
void Calibration::configureTempCurves(TempCompCurve *x,
                                      TempCompCurve *y,
                                      TempCompCurve *z)
{
    xCurve = x;
    yCurve = y;
    zCurve = z;
}

/**
  * Grab a sample of gyro data with the temperautre
  * @return true If enough data is averaged at this position
  */
bool Calibration::storeTempCalMeasurement(UAVObject * obj)
{
    if (obj->getObjID() == Gyros::OBJID) {
        Gyros *gyros = Gyros::GetInstance(getObjectManager());
        Q_ASSERT(gyros);
        Gyros::DataFields gyrosData = gyros->getData();
        double gyros_body[3] = {gyrosData.x, gyrosData.y, gyrosData.z};
        double gyros_sensor[3];
        rotate_vector(boardRotationMatrix, gyros_body, gyros_sensor, false);
        gyro_accum_x.append(gyros_sensor[0]);
        gyro_accum_y.append(gyros_sensor[1]);
        gyro_accum_z.append(gyros_sensor[2]);
        gyro_accum_temp.append(gyrosData.temperature);
    }

    auto m = std::minmax_element(gyro_accum_temp.begin(), gyro_accum_temp.end());
    double range = *m.second - *m.first; // max - min
    emit tempCalProgressChanged(static_cast<int>((100 * range) / min_temperature_range));

    if ((gyro_accum_temp.size() % 10) == 0) {
        updateTempCompCalibrationDisplay();
    }

    // If enough data is collected, average it for this position
    if(range >= min_temperature_range) {
        return true;
    }

    return false;
}

/**
 * @brief Calibration::updateTempCompCalibrationDisplay
 */
void Calibration::updateTempCompCalibrationDisplay()
{
    int n_samples = gyro_accum_temp.size();

    // Construct the matrix of temperature.
    Eigen::Matrix<double, Eigen::Dynamic, 4> X(n_samples, 4);

    // And the matrix of gyro samples.
    Eigen::Matrix<double, Eigen::Dynamic, 3> Y(n_samples, 3);

    for (int i = 0; i < n_samples; ++i) {
        X(i,0) = 1;
        X(i,1) = gyro_accum_temp[i];
        X(i,2) = pow(gyro_accum_temp[i],2);
        X(i,3) = pow(gyro_accum_temp[i],3);
        Y(i,0) = gyro_accum_x[i];
        Y(i,1) = gyro_accum_y[i];
        Y(i,2) = gyro_accum_z[i];
    }

    // Solve Y = X * B
    // Use the cholesky-based Penrose pseudoinverse method.
    Eigen::Matrix<double, 4, 3> result = (X.transpose() * X).ldlt().solve(X.transpose()*Y);

    QList<double> xCoeffs, yCoeffs, zCoeffs;
    xCoeffs.clear();
    xCoeffs.append(result(0,0));
    xCoeffs.append(result(1,0));
    xCoeffs.append(result(2,0));
    xCoeffs.append(result(3,0));
    yCoeffs.clear();
    yCoeffs.append(result(0,1));
    yCoeffs.append(result(1,1));
    yCoeffs.append(result(2,1));
    yCoeffs.append(result(3,1));
    zCoeffs.clear();
    zCoeffs.append(result(0,2));
    zCoeffs.append(result(1,2));
    zCoeffs.append(result(2,2));
    zCoeffs.append(result(3,2));

    if (xCurve != NULL)
        xCurve->plotData(gyro_accum_temp, gyro_accum_x, xCoeffs);
    if (yCurve != NULL)
        yCurve->plotData(gyro_accum_temp, gyro_accum_y, yCoeffs);
    if (zCurve != NULL)
        zCurve->plotData(gyro_accum_temp, gyro_accum_z, zCoeffs);

}

/**
 * @brief Calibration::tempCalProgressChanged Compute a polynominal fit to all
 * of the temperature data and each gyro channel
 * @return
 */
int Calibration::computeTempCal()
{
    timer.stop();
    disconnect(&timer,&QTimer::timeout,this,&Calibration::timeout);

    // Reenable gyro bias correction
    AttitudeSettings *attitudeSettings = AttitudeSettings::GetInstance(getObjectManager());
    Q_ASSERT(attitudeSettings);
    AttitudeSettings::DataFields attitudeSettingsData = attitudeSettings->getData();
    attitudeSettingsData.BiasCorrectGyro = AttitudeSettings::BIASCORRECTGYRO_TRUE;
    attitudeSettings->setData(attitudeSettingsData);
    attitudeSettings->updated();

    int n_samples = gyro_accum_temp.size();

    // Construct the matrix of temperature.
    Eigen::Matrix<double, Eigen::Dynamic, 4> X(n_samples, 4);

    // And the matrix of gyro samples.
    Eigen::Matrix<double, Eigen::Dynamic, 3> Y(n_samples, 3);

    for (int i = 0; i < n_samples; ++i) {
        X(i,0) = 1;
        X(i,1) = gyro_accum_temp[i];
        X(i,2) = pow(gyro_accum_temp[i],2);
        X(i,3) = pow(gyro_accum_temp[i],3);
        Y(i,0) = gyro_accum_x[i];
        Y(i,1) = gyro_accum_y[i];
        Y(i,2) = gyro_accum_z[i];
    }

    // Solve Y = X * B
    // Use the cholesky-based Penrose pseudoinverse method.
    Eigen::Matrix<double, 4, 3> result = (X.transpose() * X).ldlt().solve(X.transpose()*Y);

    std::stringstream str;
    str << result.format(Eigen::IOFormat(4, 0, ", ", "\n", "[", "]"));
    qDebug().noquote() << "Solution:\n" << QString::fromStdString(str.str());

    // Store the results
    SensorSettings * sensorSettings = SensorSettings::GetInstance(getObjectManager());
    Q_ASSERT(sensorSettings);
    SensorSettings::DataFields sensorSettingsData = sensorSettings->getData();
    sensorSettingsData.XGyroTempCoeff[0] = static_cast<float>(result(0, 0));
    sensorSettingsData.XGyroTempCoeff[1] = static_cast<float>(result(1, 0));
    sensorSettingsData.XGyroTempCoeff[2] = static_cast<float>(result(2, 0));
    sensorSettingsData.XGyroTempCoeff[3] = static_cast<float>(result(3, 0));
    sensorSettingsData.YGyroTempCoeff[0] = static_cast<float>(result(0, 1));
    sensorSettingsData.YGyroTempCoeff[1] = static_cast<float>(result(1, 1));
    sensorSettingsData.YGyroTempCoeff[2] = static_cast<float>(result(2, 1));
    sensorSettingsData.YGyroTempCoeff[3] = static_cast<float>(result(3, 1));
    sensorSettingsData.ZGyroTempCoeff[0] = static_cast<float>(result(0, 2));
    sensorSettingsData.ZGyroTempCoeff[1] = static_cast<float>(result(1, 2));
    sensorSettingsData.ZGyroTempCoeff[2] = static_cast<float>(result(2, 2));
    sensorSettingsData.ZGyroTempCoeff[3] = static_cast<float>(result(3, 2));
    sensorSettings->setData(sensorSettingsData);

    QList<double> xCoeffs, yCoeffs, zCoeffs;
    xCoeffs.clear();
    xCoeffs.append(result(0,0));
    xCoeffs.append(result(1,0));
    xCoeffs.append(result(2,0));
    xCoeffs.append(result(3,0));
    yCoeffs.clear();
    yCoeffs.append(result(0,1));
    yCoeffs.append(result(1,1));
    yCoeffs.append(result(2,1));
    yCoeffs.append(result(3,1));
    zCoeffs.clear();
    zCoeffs.append(result(0,2));
    zCoeffs.append(result(1,2));
    zCoeffs.append(result(2,2));
    zCoeffs.append(result(3,2));

    if (xCurve != NULL)
        xCurve->plotData(gyro_accum_temp, gyro_accum_x, xCoeffs);
    if (yCurve != NULL)
        yCurve->plotData(gyro_accum_temp, gyro_accum_y, yCoeffs);
    if (zCurve != NULL)
        zCurve->plotData(gyro_accum_temp, gyro_accum_z, zCoeffs);

    emit tempCalProgressChanged(0);

    // Inform the system that the calibration process has completed
    emit calibrationCompleted();

    return CALIBRATION_SUCCESS;
}

/**
 * Util function to get a pointer to the object manager
 * @return pointer to the UAVObjectManager
 */
UAVObjectManager* Calibration::getObjectManager() {
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectManager * objMngr = pm->getObject<UAVObjectManager>();
    Q_ASSERT(objMngr);
    return objMngr;
}

/**
 * @brief Calibration::getObjectUtilManager Utility function to get a pointer to the object manager utilities
 * @return pointer to the UAVObjectUtilManager
 */
UAVObjectUtilManager* Calibration::getObjectUtilManager() {
    ExtensionSystem::PluginManager *pm = ExtensionSystem::PluginManager::instance();
    UAVObjectUtilManager *utilMngr = pm->getObject<UAVObjectUtilManager>();
    Q_ASSERT(utilMngr);
    return utilMngr;
}

/**
  * Computes the scale and bias for the accelerometer and mag once all the data
  * has been collected in 6 positions.
  */
int Calibration::computeScaleBias()
{
    SensorSettings * sensorSettings = SensorSettings::GetInstance(getObjectManager());
    Q_ASSERT(sensorSettings);
    SensorSettings::DataFields sensorSettingsData = sensorSettings->getData();

    bool good_calibration = true;

    //Assign calibration data
    if (calibrateAccels) {
        good_calibration = true;

        qDebug() << "Accel measurements";
        for(int i = 0; i < 6; i++)
            qDebug() << accel_data_x[i] << ", " << accel_data_y[i] << ", " << accel_data_z[i] << ";";

        // Solve for accelerometer calibration
        double S[3], b[3];
        SixPointInConstFieldCal(accelLength, accel_data_x, accel_data_y, accel_data_z, S, b);

        sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_X] += (-sign(S[0]) * b[0]);
        sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_Y] += (-sign(S[1]) * b[1]);
        sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_Z] += (-sign(S[2]) * b[2]);

        sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_X] *= fabs(S[0]);
        sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_Y] *= fabs(S[1]);
        sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_Z] *= fabs(S[2]);

        // Check the accel calibration is good (checks for NaN's)
        good_calibration &= !std::isnan(sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_X]);
        good_calibration &= !std::isnan(sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_Y]);
        good_calibration &= !std::isnan(sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_Z]);
        good_calibration &= !std::isnan(sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_X]);
        good_calibration &= !std::isnan(sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_Y]);
        good_calibration &= !std::isnan(sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_Z]);

        //This can happen if, for instance, HomeLocation.g_e == 0
        if((S[0]+S[1]+S[2])<0.0001){
            good_calibration=false;
        }

        if (good_calibration) {
            qDebug()<<  "Accel bias: " << sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_X] << " " << sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_Y] << " " << sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_Z];
        } else {
            return ACCELEROMETER_FAILED;
        }
    }

    if (calibrateMags) {
        good_calibration = true;

        qDebug() << "Mag measurements";
        for(int i = 0; i < 6; i++)
            qDebug() << mag_data_x[i] << ", " << mag_data_y[i] << ", " << mag_data_z[i] << ";";

        // Work out the average vector length as nominal since mag scale normally close to 1
        // and we don't use the magnitude anyway.  Avoids requiring home location.
        double m_x = 0, m_y = 0, m_z = 0, len = 0;
        for (int i = 0; i < 6; i++) {
            m_x += mag_data_x[i];
            m_y += mag_data_x[i];
            m_z += mag_data_x[i];
        }
        m_x /= 6;
        m_y /= 6;
        m_z /= 6;
        for (int i = 0; i < 6; i++) {
            len += sqrt(pow(mag_data_x[i] - m_x,2) + pow(mag_data_y[i] - m_y,2) + pow(mag_data_z[i] - m_z,2));
        }
        len /= 6;

        // Solve for magnetometer calibration
        double S[3], b[3];
        SixPointInConstFieldCal(len, mag_data_x, mag_data_y, mag_data_z, S, b);

        //Assign calibration data
        sensorSettingsData.MagBias[SensorSettings::MAGBIAS_X] += (-sign(S[0]) * b[0]);
        sensorSettingsData.MagBias[SensorSettings::MAGBIAS_Y] += (-sign(S[1]) * b[1]);
        sensorSettingsData.MagBias[SensorSettings::MAGBIAS_Z] += (-sign(S[2]) * b[2]);
        sensorSettingsData.MagScale[SensorSettings::MAGSCALE_X] *= fabs(S[0]);
        sensorSettingsData.MagScale[SensorSettings::MAGSCALE_Y] *= fabs(S[1]);
        sensorSettingsData.MagScale[SensorSettings::MAGSCALE_Z] *= fabs(S[2]);

        // Check the mag calibration is good (checks for NaN's)
        good_calibration &= !std::isnan(sensorSettingsData.MagBias[SensorSettings::MAGBIAS_X]);
        good_calibration &= !std::isnan(sensorSettingsData.MagBias[SensorSettings::MAGBIAS_Y]);
        good_calibration &= !std::isnan(sensorSettingsData.MagBias[SensorSettings::MAGBIAS_Z]);
        good_calibration &= !std::isnan(sensorSettingsData.MagScale[SensorSettings::MAGSCALE_X]);
        good_calibration &= !std::isnan(sensorSettingsData.MagScale[SensorSettings::MAGSCALE_Y]);
        good_calibration &= !std::isnan(sensorSettingsData.MagScale[SensorSettings::MAGSCALE_Z]);

        //This can happen if, for instance, HomeLocation.g_e == 0
        if((S[0]+S[1]+S[2])<0.0001){
            good_calibration=false;
        }

        if (good_calibration) {
            qDebug()<<  "Mag bias: " << sensorSettingsData.MagBias[SensorSettings::MAGBIAS_X] << " " << sensorSettingsData.MagBias[SensorSettings::MAGBIAS_Y]  << " " << sensorSettingsData.MagBias[SensorSettings::MAGBIAS_Z];
        } else {
            return MAGNETOMETER_FAILED;
        }
    }

    // If we've made it this far, it's because good_calibration == true
    sensorSettings->setData(sensorSettingsData);

    // Inform the system that the calibration process has completed
    emit calibrationCompleted();

    return CALIBRATION_SUCCESS;
}

/**
 * @brief Calibration::resetToOriginalValues Resets the accelerometer and magnetometer setting to their pre-calibration values
 */
void Calibration::resetSensorCalibrationToOriginalValues()
{
    //Write the original accelerometer values back to the device
    SensorSettings * sensorSettings = SensorSettings::GetInstance(getObjectManager());
    Q_ASSERT(sensorSettings);
    SensorSettings::DataFields sensorSettingsData = sensorSettings->getData();

    if (calibrateAccels) {
        sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_X] = static_cast<float>(initialAccelsScale[0]);
        sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_Y] = static_cast<float>(initialAccelsScale[1]);
        sensorSettingsData.AccelScale[SensorSettings::ACCELSCALE_Z] = static_cast<float>(initialAccelsScale[2]);
        sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_X] = static_cast<float>(initialAccelsBias[0]);
        sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_Y] = static_cast<float>(initialAccelsBias[1]);
        sensorSettingsData.AccelBias[SensorSettings::ACCELBIAS_Z] = static_cast<float>(initialAccelsBias[2]);
    }

    if (calibrateMags) {
        //Write the original magnetometer values back to the device
        sensorSettingsData.MagScale[SensorSettings::MAGSCALE_X] = static_cast<float>(initialMagsScale[0]);
        sensorSettingsData.MagScale[SensorSettings::MAGSCALE_Y] = static_cast<float>(initialMagsScale[1]);
        sensorSettingsData.MagScale[SensorSettings::MAGSCALE_Z] = static_cast<float>(initialMagsScale[2]);
        sensorSettingsData.MagBias[SensorSettings::MAGBIAS_X] = static_cast<float>(initialMagsBias[0]);
        sensorSettingsData.MagBias[SensorSettings::MAGBIAS_Y] = static_cast<float>(initialMagsBias[1]);
        sensorSettingsData.MagBias[SensorSettings::MAGBIAS_Z] = static_cast<float>(initialMagsBias[2]);
    }

    sensorSettings->setData(sensorSettingsData);
    sensorSettings->updated();
}

/**
 * @brief Compute a rotation matrix from a set of euler angles
 * @param[in] rpy The euler angles in roll, pitch, yaw
 * @param[out] Rbe The rotation matrix to take a matrix from
 *       0,0,0 to that rotation
 */
void Calibration::Euler2R(double rpy[3], double Rbe[3][3])
{
    double sF = sin(rpy[0]), cF = cos(rpy[0]);
    double sT = sin(rpy[1]), cT = cos(rpy[1]);
    double sP = sin(rpy[2]), cP = cos(rpy[2]);

    Rbe[0][0] = cT*cP;
    Rbe[0][1] = cT*sP;
    Rbe[0][2] = -sT;
    Rbe[1][0] = sF*sT*cP - cF*sP;
    Rbe[1][1] = sF*sT*sP + cF*cP;
    Rbe[1][2] = cT*sF;
    Rbe[2][0] = cF*sT*cP + sF*sP;
    Rbe[2][1] = cF*sT*sP - sF*cP;
    Rbe[2][2] = cT*cF;
}

/**
 * @brief Rotate a vector by the rotation matrix, optionally trasposing
 * @param[in] R the rotation matrix
 * @param[in] vec The vector to rotate by this matrix
 * @param[out] vec_out The rotated vector
 * @param[in] transpose Optionally transpose the rotation matrix first (reverse the rotation)
 */
void Calibration::rotate_vector(double R[3][3], const double vec[3], double vec_out[3], bool transpose = true)
{
    if (!transpose){
        vec_out[0] = R[0][0] * vec[0] + R[0][1] * vec[1] + R[0][2] * vec[2];
        vec_out[1] = R[1][0] * vec[0] + R[1][1] * vec[1] + R[1][2] * vec[2];
        vec_out[2] = R[2][0] * vec[0] + R[2][1] * vec[1] + R[2][2] * vec[2];
    }
    else {
        vec_out[0] = R[0][0] * vec[0] + R[1][0] * vec[1] + R[2][0] * vec[2];
        vec_out[1] = R[0][1] * vec[0] + R[1][1] * vec[1] + R[2][1] * vec[2];
        vec_out[2] = R[0][2] * vec[0] + R[1][2] * vec[1] + R[2][2] * vec[2];
    }
}

/**
 * @name SixPointInConstFieldCal
 * @brief Compute the scale and bias assuming the data comes from six orientations
 *        in a constant field
 *
 *   x, y, z are vectors of six measurements
 *
 * Computes sensitivity and offset such that:
 *
 * c = S * A + b
 *
 * where c is the measurement, S is the sensitivity, b is the bias offset, and
 * A is the field being measured  expressed as a ratio of the measured value
 * to the field strength. aka a direction cosine.
 *
 * A is what we really want and it is computed using the equation:
 *
 * A = (c - b)/S
 *
 */
int Calibration::SixPointInConstFieldCal( double ConstMag, double x[6], double y[6], double z[6], double S[3], double b[3] )
{
    Eigen::Matrix<double, 5, 5> A;
    Eigen::Matrix<double, 5, 1> f;
    double xp, yp, zp, Sx;

    // Fill in matrix A -
    // write six difference-in-magnitude equations of the form
    // Sx^2(x2^2-x1^2) + 2*Sx*bx*(x2-x1) + Sy^2(y2^2-y1^2) + 2*Sy*by*(y2-y1) + Sz^2(z2^2-z1^2) + 2*Sz*bz*(z2-z1) = 0
    // or in other words
    // 2*Sx*bx*(x2-x1)/Sx^2  + Sy^2(y2^2-y1^2)/Sx^2  + 2*Sy*by*(y2-y1)/Sx^2  + Sz^2(z2^2-z1^2)/Sx^2  + 2*Sz*bz*(z2-z1)/Sx^2  = (x1^2-x2^2)
    for (int i = 0; i < 5; i++) {
        A(i, 0) = 2.0 * (x[i+1] - x[i]);
        A(i, 1) = y[i+1]*y[i+1] - y[i]*y[i];
        A(i, 2) = 2.0 * (y[i+1] - y[i]);
        A(i, 3) = z[i+1]*z[i+1] - z[i]*z[i];
        A(i, 4) = 2.0 * (z[i+1] - z[i]);
        f[i]    = x[i]*x[i] - x[i+1]*x[i+1];
    }

    // solve for c0=bx/Sx, c1=Sy^2/Sx^2; c2=Sy*by/Sx^2, c3=Sz^2/Sx^2, c4=Sz*bz/Sx^2
    Eigen::Matrix<double, 5, 1> c = A.fullPivHouseholderQr().solve(f);

    // use one magnitude equation and c's to find Sx - doesn't matter which - all give the same answer
    xp = x[0]; yp = y[0]; zp = z[0];
    Sx = sqrt(ConstMag*ConstMag / (xp*xp + 2*c[0]*xp + c[0]*c[0] + c[1]*yp*yp + 2*c[2]*yp + c[2]*c[2]/c[1] + c[3]*zp*zp + 2*c[4]*zp + c[4]*c[4]/c[3]));

    S[0] = Sx;
    b[0] = Sx*c[0];
    S[1] = sqrt(c[1]*Sx*Sx);
    b[1] = c[2]*Sx*Sx/S[1];
    S[2] = sqrt(c[3]*Sx*Sx);
    b[2] = c[4]*Sx*Sx/S[2];

    return 1;
}

void Calibration::setMetadata(QMap<QString, UAVObject::Metadata> metaList)
{
    QEventLoop loop;
    QTimer::singleShot(META_OPERATIONS_TIMEOUT, &loop, &QEventLoop::quit);
    connect(getObjectUtilManager(), &UAVObjectUtilManager::completedMetadataWrite, &loop, &QEventLoop::quit);
    // Show the UI is blocking
    emit calibrationBusy(true);
    // Set new metadata
    getObjectUtilManager()->setAllNonSettingsMetadata(metaList);
    loop.exec();
    emit calibrationBusy(false);
}
