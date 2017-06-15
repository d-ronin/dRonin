/**
 ******************************************************************************
 * @file       longlongspinbox.cpp
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

#include "longlongspinbox.h"

#include <QLineEdit>
#include <QEvent>
#include <QStyle>

LongLongSpinBox::LongLongSpinBox(QWidget *parent)
    : QAbstractSpinBox(parent), m_value(0), m_singleStep(1), m_min(0), m_max(100),
      m_displayBase(10)

{
    setInputMethodHints(Qt::ImhFormattedNumbersOnly);

    connect(lineEdit(), &QLineEdit::textChanged, this, &LongLongSpinBox::lineEditChanged, Qt::UniqueConnection);
}

LongLongSpinBox::~LongLongSpinBox() {}

qint64 LongLongSpinBox::value() const
{
    return m_value;
}

void LongLongSpinBox::setValue(qint64 val)
{
    if (val != m_value) {
        m_value = val;
        updateEdit();
        emit valueChanged(m_value);
        emit valueChanged(textFromValue(m_value));
    }
}

QString LongLongSpinBox::prefix() const
{
    return m_prefix;
}

void LongLongSpinBox::setPrefix(const QString &prefix)
{
    m_prefix = prefix;
    updateEdit();
    updateGeometry();
}

QString LongLongSpinBox::suffix() const
{
    return m_suffix;
}

void LongLongSpinBox::setSuffix(const QString &suffix)
{
    m_suffix = suffix;
    updateEdit();
    updateGeometry();
}

QString LongLongSpinBox::cleanText() const
{
    return stripped(text());
}

qint64 LongLongSpinBox::singleStep() const
{
    return m_singleStep;
}

void LongLongSpinBox::setSingleStep(qint64 val)
{
    if (val >= 0)
        m_singleStep = val;
}

qint64 LongLongSpinBox::minimum() const
{
    return m_min;
}

void LongLongSpinBox::setMinimum(qint64 min)
{
    setRange(min, m_max < min ? min : m_max);
}

qint64 LongLongSpinBox::maximum() const
{
    return m_min;
}

void LongLongSpinBox::setMaximum(qint64 max)
{
    setRange(max < m_min ? max : m_min, max);
}

void LongLongSpinBox::setRange(qint64 min, qint64 max)
{
    m_min = min;
    m_max = max < min ? min : max;
    setValue(qBound(m_min, m_value, m_max));
}

int LongLongSpinBox::displayIntegerBase() const
{
    return m_displayBase;
}

void LongLongSpinBox::setDisplayIntegerBase(int base)
{
    if (Q_UNLIKELY(base < 2 || base > 36)) {
        qWarning("LongLongSpinBox::setDisplayIntegerBase: Invalid base (%d)", base);
        base = 10;
    }

    if (base != m_displayBase) {
        m_displayBase = base;
        updateEdit();
    }
}

QString LongLongSpinBox::textFromValue(qint64 val) const
{
    QString str;
    if (m_displayBase == 10) {
        str = locale().toString(val);
        if (!m_showGroupSeparator && (qAbs(val) >= 1000 || val == LONG_LONG_MIN))
            str.remove(locale().groupSeparator());
    } else {
        QLatin1String prefix = val < 0 ? QLatin1String("-") : QLatin1String();
        str = prefix + QString::number(val, m_displayBase);
    }

    return str;
}

qint64 LongLongSpinBox::valueFromText(const QString &text)
{
    QString copy = text;
    int pos = lineEdit()->cursorPosition();
    QValidator::State state = QValidator::Acceptable;
    return validateAndInterpret(copy, pos, state);
}

qint64 LongLongSpinBox::validateAndInterpret(QString &input, int &pos,
                                             QValidator::State &state) const
{
    QString copy = stripped(input, &pos);
    state = QValidator::Acceptable;
    qint64 num = m_min;

    if (m_max != m_min && (copy.isEmpty()
                           || (m_min < 0 && copy == QLatin1String("-"))
                           || (m_max >= 0 && copy == QLatin1String("+")))) {
        state = QValidator::Intermediate;
    } else if (copy.startsWith(QLatin1Char('-')) && m_min >= 0) {
        state = QValidator::Invalid;
    } else {
        bool ok = false;
        if (m_displayBase == 10) {
            num = locale().toLongLong(copy, &ok);
            if (!ok && copy.contains(locale().groupSeparator())
                    && (m_max >= 1000 || m_min <= - 1000)) {
                QString copy2 = copy;
                copy2.remove(locale().groupSeparator());
                num = locale().toLongLong(copy2, &ok);
            }
        } else {
            num = copy.toLongLong(&ok, m_displayBase);
        }

        if (!ok) {
            state = QValidator::Invalid;
        } else if (num >= m_min && num <= m_max) {
            state = QValidator::Acceptable;
        } else if (m_max == m_min) {
            state = QValidator::Invalid;
        } else {
            if ((num >= 0 && num > m_max) || (num < 0 && num < m_min))
                state = QValidator::Invalid;
            else
                state = QValidator::Intermediate;
        }
    }

    if (state != QValidator::Acceptable)
        num = m_max > 0 ? m_min : m_max;

    input = m_prefix + copy + m_suffix;

    return num;
}

QValidator::State LongLongSpinBox::validate(QString &text, int &pos) const
{
    QValidator::State state;
    validateAndInterpret(text, pos, state);
    return state;
}

void LongLongSpinBox::fixup(QString &input) const
{
    if (!isGroupSeparatorShown())
        input.remove(locale().groupSeparator());
}

QString LongLongSpinBox::stripped(const QString &t, int *pos) const
{
    QStringRef text(&t);

    if (specialValueText().size() == 0 || text != specialValueText()) {
        int from = 0;
        int size = text.size();
        bool changed = false;
        if (m_prefix.size() && text.startsWith(m_prefix)) {
            from += m_prefix.size();
            size -= from;
            changed = true;
        }
        if (m_suffix.size() && text.endsWith(m_suffix)) {
            size -= suffix().size();
            changed = true;
        }
        if (changed)
            text = text.mid(from, size);
    }

    const int s = text.size();
    text = text.trimmed();
    if (pos)
        (*pos) -= (s - text.size());

    return text.toString();
}

void LongLongSpinBox::updateEdit()
{
    const bool specialValue = specialValueText().size() > 0;
    const QString newText = specialValue ? specialValueText() :
                                m_prefix + textFromValue(m_value) + m_suffix;
    if (newText == lineEdit()->displayText())
        return;

    const bool empty = lineEdit()->text().isEmpty();
    int cursor = lineEdit()->cursorPosition();
    int selsize = lineEdit()->selectedText().size();
    const QSignalBlocker blocker(lineEdit());
    lineEdit()->setText(newText);

    if (!specialValue) {
        cursor = qBound(m_prefix.size(), cursor, lineEdit()->displayText().size() - m_suffix.size());

        if (selsize > 0) {
            lineEdit()->setSelection(cursor, selsize);
        } else {
            lineEdit()->setCursorPosition(empty ? m_prefix.size() : cursor);
        }
    }
    update();
}

QAbstractSpinBox::StepEnabled LongLongSpinBox::stepEnabled() const
{
    QAbstractSpinBox::StepEnabled enabled;
    if (m_value > m_min)
        enabled |= StepDownEnabled;
    if (m_value < m_max)
        enabled |= StepUpEnabled;
    return enabled;
}

void LongLongSpinBox::stepBy(int steps)
{
    setValue(qBound(m_min, m_value + steps, m_max));
}

void LongLongSpinBox::setLineEdit(QLineEdit *edit)
{
    QAbstractSpinBox::setLineEdit(edit);
    if (edit) {
        connect(lineEdit(), &QLineEdit::textChanged, this, &LongLongSpinBox::lineEditChanged, Qt::UniqueConnection);
    }
}

void LongLongSpinBox::lineEditChanged(const QString &t)
{
    if (keyboardTracking()) {
        QString tmp = t;
        int pos = lineEdit()->cursorPosition();
        QValidator::State state = validate(tmp, pos);
        if (state == QValidator::Acceptable) {
            const qint64 v = valueFromText(tmp);
            setValue(v);
        }
    }
}

/**
 * @}
 * @}
 */
