/**
 ******************************************************************************
 * @file       uavobjectstests.cpp
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup UAVObjectsPlugin UAVObjects Plugin
 * @{
 * @brief      The UAVUObjects GCS plugin
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

#include "uavobjectsplugin.h"

#include "uavdataobject.h"
#include "uavobjectfield.h"

#include <QTest>
#include <memory>


void UAVObjectsPlugin::testEnumFields()
{
    auto field = new UAVObjectField("TestEnum", "photons", UAVObjectField::ENUM, 2, {"Yes", "No"},
                                    {0, 1}, QString(), QStringLiteral("Test some stuff"), {"No", "Yes"});
    quint8 testData[255] = {};

    field->initialize(testData, 0, nullptr);
    QCOMPARE(field->getName(), QStringLiteral("TestEnum"));
    QCOMPARE(field->getValue(0).toString(), QStringLiteral("Yes"));
    QCOMPARE(field->getValue(1).toString(), QStringLiteral("Yes"));
    QVERIFY(!field->isDefaultValue(0));
    QVERIFY(field->isDefaultValue(1));
}

void UAVObjectsPlugin::testIntFields()
{
    std::unique_ptr<UAVObjectField> field(new UAVObjectField("TestUInt8", "photons", UAVObjectField::UINT8, 2, {},
                                    {}, QString(), QStringLiteral("Test some stuff"), {254, 0}));
    quint8 testData[255] = {};

    field->initialize(testData, 0, nullptr);
    QCOMPARE(field->getName(), QStringLiteral("TestUInt8"));
    QVERIFY(field->getValue(0).toUInt() == 0);
    QVERIFY(field->getValue(1).toUInt() == 0);
    QVERIFY(!field->isDefaultValue(0));
    QVERIFY(field->isDefaultValue(1));
}

/**
 * @}
 * @}
 */
