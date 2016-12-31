#include "seppukuconfiguration.h"

SeppukuConfiguration::SeppukuConfiguration(QWidget *parent) :
    ConfigTaskWidget(parent),
    ui(new Ui::Seppuku),
    m_background(Q_NULLPTR)
{
    ui->setupUi(this);

    setupGraphicsScene();

    connect(ui->cbMag, &QComboBox::currentTextChanged, this, &SeppukuConfiguration::magChanged);
    connect(ui->cbOutputs, &QComboBox::currentTextChanged, this, &SeppukuConfiguration::outputsChanged);
    connect(ui->cbRcvrPort, &QComboBox::currentTextChanged, this, &SeppukuConfiguration::checkRcvr);
    connect(ui->cbUart1, &QComboBox::currentTextChanged, this, &SeppukuConfiguration::checkDsm);
    connect(ui->cbUart3, &QComboBox::currentTextChanged, this, &SeppukuConfiguration::checkUart3);
    connect(ui->cbUart4, &QComboBox::currentTextChanged, this, &SeppukuConfiguration::checkDsm);
    connect(ui->cbUart6, &QComboBox::currentTextChanged, this, &SeppukuConfiguration::checkDsm);
    connect(ui->cbDsmMode, &QComboBox::currentTextChanged, this, &SeppukuConfiguration::checkDsm);

    autoLoadWidgets();
    populateWidgets();
    refreshWidgetsValues();
}

SeppukuConfiguration::~SeppukuConfiguration()
{
    delete ui;
}

void SeppukuConfiguration::setupGraphicsScene()
{
    const QString diagram = ":/dronin/images/seppuku-config.svg";
    m_renderer = new QSvgRenderer();
    if (!m_renderer)
        return;
    if (QFile::exists(diagram) && m_renderer->load(diagram) && m_renderer->isValid()) {
        m_scene = new QGraphicsScene(this);
        if (!m_scene)
            return;
        ui->boardDiagram->setScene(m_scene);

        m_background = new QGraphicsSvgItem();
        if (!m_background)
            return;
        m_background->setSharedRenderer(m_renderer);
        m_background->setElementId("seppuku");
        m_background->setOpacity(1.0);
        m_background->setZValue(0);
        m_scene->addItem(m_background);

        m_uart3 = addGraphicsElement("uart3");
        if (!m_uart3)
            return;

        m_uart4 = addGraphicsElement("esc78");
        if (!m_uart4)
            return;

        const QRectF bound = m_background->boundingRect();
        ui->boardDiagram->setSceneRect(bound);
        ui->boardDiagram->setRenderHints(QPainter::SmoothPixmapTransform);
        ui->boardDiagram->fitInView(m_background, Qt::KeepAspectRatio);
    } else {
        qWarning() << "failed to setup seppuku board diagram!";
    }
}

QGraphicsSvgItem *SeppukuConfiguration::addGraphicsElement(const QString &elementId)
{
    QGraphicsSvgItem *item = new QGraphicsSvgItem();
    if (!item)
        return Q_NULLPTR;
    item->setSharedRenderer(m_renderer);
    item->setElementId(elementId);
    item->setOpacity(1.0);
    static int zval = 1;
    item->setZValue(zval++);
    QMatrix matrix = m_renderer->matrixForElement(elementId);
    QRectF orig = matrix.mapRect(m_renderer->boundsOnElement(elementId));
    item->setPos(orig.x(), orig.y());
    m_scene->addItem(item);
    return item;
}

void SeppukuConfiguration::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    if (m_background)
        ui->boardDiagram->fitInView(m_background, Qt::KeepAspectRatio);
}

void SeppukuConfiguration::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    if (m_background)
        ui->boardDiagram->fitInView(m_background, Qt::KeepAspectRatio);
}

void SeppukuConfiguration::magChanged(const QString &newVal)
{
    bool ext = newVal.contains("External");
    setWidgetEnabled(ui->lblExtMagOrientation, ext);
    setWidgetEnabled(ui->cbExtMagOrientation, ext);
    checkExtMag();
}

void SeppukuConfiguration::outputsChanged(const QString &newVal)
{
    bool uart = newVal.contains("UART");
    if (m_uart4)
        m_uart4->setElementId(uart ? "uart4" : "esc78");
    setWidgetEnabled(ui->lblUart4, uart);
    setWidgetEnabled(ui->cbUart4, uart);
}

void SeppukuConfiguration::checkExtMag()
{
    bool extMag = ui->cbMag->currentText().contains("External");
    bool uartExt = ui->cbUart3->currentText() == "I2C";

    if (extMag && !uartExt)
        setMessage("ExtMag", "External magnetometer selected but UART3 not configured as I2C!", "error");
    else if (uartExt && !extMag)
        setMessage("ExtMag", "UART3 configured as I2C but external magnetometer not selected.");
    else
        setMessage("ExtMag");
}

void SeppukuConfiguration::checkDsm()
{
    bool dsm = false;
    dsm |= ui->cbRcvrPort->currentText().contains("DSM");
    dsm |= ui->cbUart1->currentText().contains("DSM");
    dsm |= ui->cbUart3->currentText().contains("DSM");
    dsm |= ui->cbUart4->currentText().contains("DSM");
    dsm |= ui->cbUart6->currentText().contains("DSM");

    setWidgetEnabled(ui->lblDsmMode, dsm);
    setWidgetEnabled(ui->cbDsmMode, dsm);
}

void SeppukuConfiguration::checkUart3(const QString &newVal)
{
    checkDsm();
    checkExtMag();

    bool i2c = newVal.contains("I2C");
    if (m_uart3)
        m_uart3->setElementId(i2c ? "i2c" : "uart3");
}

void SeppukuConfiguration::checkRcvr(const QString &newVal)
{
    checkDsm();

    bool dsm = newVal.contains("DSM");
    bool enabled = newVal != "Disabled";
    if (dsm)
        setMessage("RxPower", "Please remember to solder the 3V3 receiver voltage jumper under the board.", "info");
    else if (enabled)
        setMessage("RxPower", "Please remember to solder ONE of the receiver voltage jumpers under the board.", "info");
    else
        setMessage("RxPower");
}

void SeppukuConfiguration::setMessage(const QString &name, const QString &msg, const QString &severity)
{
    QLabel *lbl = ui->gbMessages->findChild<QLabel *>(name);
    if (msg.length()) {
        if (!lbl) {
            lbl = new QLabel();
            lbl->setObjectName(name);
            ui->gbMessages->layout()->addWidget(lbl);
            ui->gbMessages->layout()->setAlignment(lbl, Qt::AlignTop);
        }
        lbl->setText(msg);
        setWidgetProperty(lbl, "severity", severity);
    } else if (lbl) {
        delete lbl;
    }
}
