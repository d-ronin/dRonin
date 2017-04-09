/**
 ******************************************************************************
 *
 * @file       styleanimator.h
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 *             Parts by Nokia Corporation (qt-info@nokia.com) Copyright (C) 2009.
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup CorePlugin Core Plugin
 * @{
 * @brief The Core GCS plugin
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

#ifndef ANIMATION_H
#define ANIMATION_H

#include <QtCore/QPointer>
#include <QtCore/QTime>
#include <QtCore/QBasicTimer>
#include <QStyle>
#include <QPainter>
#include <QWidget>

/*
 * This is a set of helper classes to allow for widget animations in
 * the style. Its mostly taken from Vista style so it should be fully documented
 * there.
 *
 */

class Animation
{
public:
    Animation()
        : m_running(true)
    {
    }
    virtual ~Animation() {}
    QWidget *widget() const { return m_widget; }
    bool running() const { return m_running; }
    const QTime &startTime() const { return m_startTime; }
    void setRunning(bool val) { m_running = val; }
    void setWidget(QWidget *widget) { m_widget = widget; }
    void setStartTime(const QTime &startTime) { m_startTime = startTime; }
    virtual void paint(QPainter *painter, const QStyleOption *option);

protected:
    void drawBlendedImage(QPainter *painter, QRect rect, float value);
    QTime m_startTime;
    QPointer<QWidget> m_widget;
    QImage m_primaryImage;
    QImage m_secondaryImage;
    QImage m_tempImage;
    bool m_running;
};

// Handles state transition animations
class Transition : public Animation
{
public:
    Transition()
        : Animation()
    {
    }
    virtual ~Transition() {}
    void setDuration(int duration) { m_duration = duration; }
    void setStartImage(const QImage &image) { m_primaryImage = image; }
    void setEndImage(const QImage &image) { m_secondaryImage = image; }
    virtual void paint(QPainter *painter, const QStyleOption *option);
    int duration() const { return m_duration; }
    int m_duration; // set time in ms to complete a state transition
};

class StyleAnimator : public QObject
{
    Q_OBJECT

public:
    StyleAnimator(QObject *parent = 0)
        : QObject(parent)
    {
    }

    void timerEvent(QTimerEvent *);
    void startAnimation(Animation *);
    void stopAnimation(const QWidget *);
    Animation *widgetAnimation(const QWidget *) const;

private:
    QBasicTimer animationTimer;
    QList<Animation *> animations;
};

#endif // ANIMATION_H
