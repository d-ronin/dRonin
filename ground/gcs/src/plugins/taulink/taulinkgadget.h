/**
 ******************************************************************************
 * @file       taulinkgadget.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @addtogroup GCSPlugins GCS Plugins
 * @{
 * @addtogroup TauLinkGadgetPlugin Tau Link Gadget Plugin
 * @{
 * @brief A gadget to monitor and configure the RFM22b link
 *****************************************************************************//*
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

#ifndef TAULINKGADGET_H_
#define TAULINKGADGET_H_

#include <coreplugin/iuavgadget.h>

namespace Core {
class IUAVGadget;
}

class TauLinkGadgetWidget;

using namespace Core;

class TauLinkGadget : public Core::IUAVGadget
{
    Q_OBJECT
public:
    TauLinkGadget(QString classId, TauLinkGadgetWidget *widget, QWidget *parent = 0);
    ~TauLinkGadget();

    QList<int> context() const { return m_context; }
    QWidget *widget() { return m_widget; }
    QString contextHelpId() const { return QString(); }

    void loadConfiguration(IUAVGadgetConfiguration* config);
private:
    QWidget *m_widget;
	QList<int> m_context;
};


#endif // TAULINKGADGET_H_
