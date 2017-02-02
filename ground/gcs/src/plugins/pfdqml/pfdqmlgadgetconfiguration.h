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

#ifndef PFDQMLGADGETCONFIGURATION_H
#define PFDQMLGADGETCONFIGURATION_H

#include <coreplugin/iuavgadgetconfiguration.h>

using namespace Core;

class PfdQmlGadgetConfiguration : public IUAVGadgetConfiguration
{
Q_OBJECT
public:
    explicit PfdQmlGadgetConfiguration(QString classId, QSettings* qSettings = 0, QObject *parent = 0);

    void setQmlFile(const QString &fileName) { m_qmlFile=fileName; }

    QString qmlFile() const { return m_qmlFile; }
    QVariantMap settings() const { return m_settings; }

    void saveConfig(QSettings* settings) const;
    IUAVGadgetConfiguration *clone();

private:
    QString m_qmlFile;

    QVariantMap m_settings;
};

#endif // PfdQmlGADGETCONFIGURATION_H
