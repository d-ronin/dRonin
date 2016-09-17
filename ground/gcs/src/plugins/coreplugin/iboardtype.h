/**
 ******************************************************************************
 *
 * @file       iboardtype.h
 * @author     dRonin, http://dRonin.org/, Copyright (C) 2016
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2012-2014
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
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Additional note on redistribution: The copyright and license notices above
 * must be maintained in each individual source file that is a derivative work
 * of this source file; otherwise redistribution is prohibited.
 */
#ifndef IBOARDTYPE_H
#define IBOARDTYPE_H

#include <QObject>
#include <QtCore/QStringList>
#include <QPixmap>

#include "core_global.h"

namespace Core {

/**
*   An IBoardType object defines an autopilot or more generally a hardware device,
*   that is supported by the GCS. It provides basic information to the GCS to detect
*   and use this board type.
*
*   Note: at present (2012), the GCS only supports autopilots, and assumes they will
*         talk with UAVTalk. Further down the line, GCS will be able to support additional
*         protocols, as well as other device types (beacons, other).
*/
class CORE_EXPORT IBoardType : public QObject
{
    Q_OBJECT

public:

    /**
     * @brief The USBInfo struct
     * TODO: finalize what we will put there, not everything
     *       is relevant.
     */
    struct USBInfo {
        QString serialNumber;
        QString manufacturer;
        QString product;
        int UsagePage = 0;
        int Usage = 0;
        int vendorID = 0;
        int productID = 0;
        // the convention for DFU mode is to change the
        // Lower byte of bcdDevice depending on whether
        // the board is in Bootloader mode or running mode.
        // We provide the relevant values there:
        int bootloaderMode = 0;
        int runningMode = 0;
        int bcdDevice = 0; // Note: not that useful, the two values above
                       // cater for almost the same into
    };


    /**
     * Short description of the board / friendly name
     */
    virtual QString shortName() = 0;

    /**
     * Long description of the board
     */
    virtual QString boardDescription() = 0;

    //! Types of capabilities boards can support
    enum BoardCapabilities {BOARD_CAPABILITIES_GYROS, BOARD_CAPABILITIES_ACCELS,
                            BOARD_CAPABILITIES_MAGS, BOARD_CAPABILITIES_BAROS,
                            BOARD_CAPABILITIES_RADIO, BOARD_CAPABILITIES_OSD,
                            BOARD_CAPABILITIES_UPGRADEABLE,
                            BOARD_DISABILITY_REQUIRESUPGRADER };
    /**
     * @brief Query capabilities of the board.
     * @return true if board supports the capability that is requested (from BoardCapabilities)
     *
     */
    virtual bool queryCapabilities(BoardCapabilities capability) = 0;

    /**
     * @brief Query number & names of output PWM channels banks on the board
     * @return list of channel bank names
     *
     */
    virtual QStringList queryChannelBanks();

    /**
     * @brief Get banks of output PWM channels banks on the board
     * @return matrix of channel bank names
     *
     */
    virtual QVector< QVector<int> > getChannelBanks(){return channelBanks;}

    /**
     * @brief getBoardPicture
     * @return provides a picture for the board. Uploader gadget or
     *         configuration plugin can use this, for instance.
     *
     *  TODO: this API is not stable yet.
     *
     */
    virtual QPixmap getBoardPicture() = 0;

    /**
     * Get supported protocol(s) for this board
     *
     * TODO: extend GCS to support multiple protocol types.
     */
    virtual QStringList getSupportedProtocols() = 0;

    /**
     * Get name of the HW Configuration UAVObject
     *
     */
    virtual QString getHwUAVO() = 0;

    /**
     * Get USB descriptors to detect the board
     */
    USBInfo getUSBInfo() { return boardUSBInfo; }

    /**
     * Get USB VendorID.
     */
    int getVendorID() { return boardUSBInfo.vendorID; }

    /**
     * Does this board support the bootloader and DFU protocol ?
     */
    bool isDFUSupported() { return dfuSupport; }

    //! Get the board type number
    int getBoardType() { return boardType; }

    //! Return a custom configuration widget, if one is provided
    virtual QWidget *getBoardConfiguration(QWidget * /*parent*/ = 0, bool /*connected*/ = true) { return NULL; }

    /***** methods related to configuring specific boards *****/

    //! Types of input to configure for the default port
    enum InputType {
        INPUT_TYPE_DISABLED,
        INPUT_TYPE_PWM,
        INPUT_TYPE_PPM,
        INPUT_TYPE_DSM,
        INPUT_TYPE_SBUS,
        INPUT_TYPE_SBUSNONINVERTED,
        INPUT_TYPE_HOTTSUMD,
        INPUT_TYPE_HOTTSUMH,
        INPUT_TYPE_IBUS,
        INPUT_TYPE_UNKNOWN,
        INPUT_TYPE_ANY
    };

    //! Returns the minimum bootloader version required
    virtual int minBootLoaderVersion() { return 0; }

    //! Determine if this board supports configuring the receiver
    virtual bool isInputConfigurationSupported(enum InputType type = INPUT_TYPE_ANY) { Q_UNUSED(type); return false; }

    /**
     * @brief Configure the board to use an receiver input type on a port number
     * @param type the type of receiver to use
     * @return true if successfully configured or false otherwise
     */
    virtual bool setInputType(enum InputType /*type*/) { return false; }

    /**
     * @brief getInputType get the current input type
     * @return the currently selected input type
     */
    virtual enum InputType getInputType() { return INPUT_TYPE_UNKNOWN; }

    /**
     * @brief getConnectionDiagram get the connection diagram for this board
     * @return a string with the name of the resource for this board diagram
     */
    virtual QString getConnectionDiagram() { return ""; }

    /**
     * @brief Query the board for the currently set max rate of the gyro
     * @return max rate of gyro
     *
     */
    virtual int queryMaxGyroRate() { return -1; }

    /**
     * Get the RFM22b device ID this modem
     * @return RFM22B device ID or 0 if not supported
     */
    virtual quint32 getRfmID() { return 0; }

    /**
     * Set the coordinator ID. If set to zero this device will
     * be a coordinator.
     * @param id - the ID of the coordinator to bind to, or 0 to make this
     *     board the coordinator
     * @param baud_rate - the maximum baud rate to use, or 0 to leave unchanged
     * @param rf_power - the maximum radio power to use or -1 to leave unchanged
     * @return true if successful or false if not
     */
    enum LinkMode { LINK_TELEM, LINK_TELEM_PPM, LINK_PPM };

    virtual bool bindRadio(quint32 /*id*/, quint32 /*baud_rate*/, float /*rf_power*/,
                           Core::IBoardType::LinkMode /*linkMode*/, quint8 /*min*/,
                           quint8 /*max*/) { return false; }

    /**
     * Check whether the board has USB
     * @return true if usb, false if not
     */
    virtual bool isUSBSupported() { return true; }

    static QString getBoardNameFromID(int id);

    virtual QStringList getAdcNames() { return QStringList(); }

    int getBankFromOutputChannel(int channel);

signals:

protected:

    void setUSBInfo(USBInfo info) { boardUSBInfo = info; }
    void setDFUSupport(bool support) { dfuSupport = support; }

    USBInfo boardUSBInfo;
    bool dfuSupport;

    //! The numerical board type ID
    qint32 boardType;

    //! The channel groups that are driven by timers
    QVector< QVector<qint32> > channelBanks;

};

} //namespace Core


#endif // IBOARDTYPE_H
