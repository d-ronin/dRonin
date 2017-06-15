/**
 ******************************************************************************
 * @file       longlongspinbox.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2017
 * @author     Copyright (C) 2016 The Qt Company Ltd.
 * @addtogroup libs GCS Libraries
 * @{
 * @addtogroup utils Utilities
 * @{
 * @brief A QSpinBox backed by a 64-bit integer
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version approved by the KDE Free Qt Foundation.
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

#ifndef LONGLONGSPINBOX_H
#define LONGLONGSPINBOX_H

#include <QAbstractSpinBox>
#include "utils_global.h"


class QTCREATOR_UTILS_EXPORT LongLongSpinBox : public QAbstractSpinBox
{
    Q_OBJECT

public:
    explicit LongLongSpinBox(QWidget *parent = Q_NULLPTR);
    ~LongLongSpinBox();

    qint64 value() const;

    QString prefix() const;
    void setPrefix(const QString &prefix);

    QString suffix() const;
    void setSuffix(const QString &suffix);

    QString cleanText() const;

    qint64 singleStep() const;
    void setSingleStep(qint64 val);

    qint64 minimum() const;
    void setMinimum(qint64 min);

    qint64 maximum() const;
    void setMaximum(qint64 max);

    void setRange(qint64 min, qint64 max);

    int displayIntegerBase() const;
    void setDisplayIntegerBase(int base);

protected:
    QValidator::State validate(QString &input, int &pos) const override;
    virtual qint64 valueFromText(const QString &text);
    virtual QString textFromValue(qint64 val) const;
    void fixup(QString &str) const override;
    void updateEdit();
    QString stripped(const QString &text, int *pos = Q_NULLPTR) const;
    qint64 validateAndInterpret(QString &input, int &pos, QValidator::State &state) const;
    virtual StepEnabled stepEnabled() const override;
    virtual void stepBy(int steps) override;
    virtual void setLineEdit(QLineEdit *edit);


public Q_SLOTS:
    void setValue(qint64 val);

protected Q_SLOTS:
    void lineEditChanged(const QString &t);

Q_SIGNALS:
    void valueChanged(qint64 val);
    void valueChanged(const QString &val);

private:
    qint64 m_value, m_singleStep, m_min, m_max;
    QString m_prefix, m_suffix;
    int m_displayBase;
    bool m_showGroupSeparator;

    Q_DISABLE_COPY(LongLongSpinBox)
};

#endif // LONGLONGSPINBOX_H

/**
 * @}
 * @}
 */
