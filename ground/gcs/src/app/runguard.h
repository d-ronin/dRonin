/**
 ******************************************************************************
 * @file       runguard.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @author     Dmitry Sazonov, Copyright (C) 2015
 * @addtogroup app GCS main application group
 * @{
 * @brief Provides a mechanism to ensure only one instance runs
 * Based on https://stackoverflow.com/a/28172162
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

#ifndef RUNGUARD_H
#define RUNGUARD_H

#include <QObject>
#include <QSharedMemory>
#include <QSystemSemaphore>

class RunGuard
{

public:
    /**
     * @brief Constructor
     * @param key A key that is unique to your application
     */
    RunGuard(const QString &key);
    ~RunGuard();

    /**
     * @brief Attempt to run and fail if another instance is running
     * @return false on failure
     */
    bool tryToRun();

private:
    const QString key;
    const QString memLockKey;
    const QString sharedmemKey;

    QSharedMemory sharedMem;
    QSystemSemaphore memLock;

    bool isAnotherRunning();
    void release();

    Q_DISABLE_COPY(RunGuard)
};

#endif // RUNGUARD_H

/**
 * @}
 */
