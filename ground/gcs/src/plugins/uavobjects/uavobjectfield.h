/**
 ******************************************************************************
 *
 * @file       uavobjectfield.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @author     dRonin, http://dronin.org Copyright (C) 2015-2016
 * @see        The GNU Public License (GPL) Version 3
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
 */
#ifndef UAVOBJECTFIELD_H
#define UAVOBJECTFIELD_H

#include "uavobjects/uavobjects_global.h"
#include "uavobjects/uavobject.h"
#include <QStringList>
#include <QVariant>
#include <QList>
#include <QMap>

class UAVObject;

class UAVOBJECTS_EXPORT UAVObjectField : public QObject
{
    Q_OBJECT

public:
    enum FieldType {
        INT8 = 0,
        INT16,
        INT32,
        UINT8,
        UINT16,
        UINT32,
        FLOAT32,
        ENUM,
        BITFIELD,
        STRING
    };
    enum LimitType { EQUAL, NOT_EQUAL, BETWEEN, BIGGER, SMALLER };
    enum DisplayType { DEC, HEX, BIN, OCT };

    struct LimitStruct
    {
        LimitType type;
        QList<QVariant> values;
        int board;
    };

    UAVObjectField(const QString &name, const QString &units, FieldType type, int numElements,
                   const QStringList &options, const QList<int> &indices,
                   const QString &limits = QString(), const QString &description = QString(),
                   const QList<QVariant> defaultValues = QList<QVariant>(),
                   const DisplayType display = DEC);
    UAVObjectField(const QString &name, const QString &units, FieldType type,
                   const QStringList &elementNames, const QStringList &options,
                   const QList<int> &indices, const QString &limits = QString(),
                   const QString &description = QString(),
                   const QList<QVariant> defaultValues = QList<QVariant>(),
                   const DisplayType display = DEC);
    void initialize(quint8 *data, quint32 dataOffset, UAVObject *obj);
    UAVObject *getObject() const;
    FieldType getType() const;
    QString getTypeAsString() const;
    QString getName() const;
    QString getUnits() const;
    int getNumElements() const;
    QStringList getElementNames() const;
    QString getElementName(int index = 0) const;
    int getElementIndex(const QString &name) const;
    QStringList getOptions() const;
    /**
     * @brief hasOption Check if the given option exists
     * @param option Option value
     * @return true if option exists, false otherwise
     */
    bool hasOption(const QString &option);
    qint32 pack(quint8 *dataOut);
    qint32 unpack(const quint8 *dataIn);
    QVariant getValue(int index = 0) const;
    bool checkValue(const QVariant &data, int index = 0) const;
    void setValue(const QVariant &data, int index = 0);
    double getDouble(int index = 0) const;
    void setDouble(double value, int index = 0);
    size_t getNumBytes() const;
    bool isNumeric() const;
    bool isText() const;
    QString toString() const;
    QString getDescription() const;
    /**
     * @brief Get the default value (defined in the UAVO def) for the element
     * @param index The element to inspect
     * @return QVariant containing the default value
     */
    QVariant getDefaultValue(int index = 0) const;
    /**
     * @brief Check if the element is set to default value
     * @param index The element to inspect
     * @return true if set to default
     */
    bool isDefaultValue(int index = 0);
    /**
     * @brief Get the preferred integer base for this field
     * @return Preferred base
     */
    int getDisplayIntegerBase() const;
    /**
     * @brief Get the prefix for the preferred display format
     * @return "0x" for hex, "0b" for binary etc.
     */
    QString getDisplayPrefix() const;

    bool isWithinLimits(QVariant var, int index, int board = 0) const;
    QVariant getMaxLimit(int index, int board = 0) const;
    QVariant getMinLimit(int index, int board = 0) const;
signals:
    void fieldUpdated(UAVObjectField *field);

protected:
    QString name;
    QString units;
    FieldType type;
    QStringList elementNames;
    QList<int> indices;
    std::map<int, int> enumToIndex;
    QStringList options;
    int numElements;
    size_t elementSize;
    size_t offset;
    quint8 *data;
    UAVObject *obj;
    QMap<int, QList<LimitStruct>> elementLimits;
    QString description;
    QList<QVariant> defaultValues;
    DisplayType display;

    void clear();
    void constructorInitialize(const QString &name, const QString &units, FieldType type,
                               const QStringList &elementNames, const QStringList &options,
                               const QList<int> &indices, const QString &limits,
                               const QString &description, const QList<QVariant> defaultValues,
                               const DisplayType display);
    void limitsInitialize(const QString &limits);
};

#endif // UAVOBJECTFIELD_H

/**
 * @}
 * @}
 */
