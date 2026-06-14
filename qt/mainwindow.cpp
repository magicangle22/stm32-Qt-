#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QtMath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_hSerial(INVALID_HANDLE_VALUE)
    , m_videoMode(false)
    , m_hasFrame(false)
    , m_streaming(false)
    , m_frameCount(0)
{
    memset(m_bitmapBuffer, 0, sizeof(m_bitmapBuffer));
    m_mediaPlayer = new QMediaPlayer(this);
    m_videoSink = nullptr;
    m_streamTimer = new QTimer(this);
    setupUi();
    refreshPortList();
}

MainWindow::~MainWindow()
{
    stopStreaming();
    m_mediaPlayer->stop();
    closeSerialPort();
}

void MainWindow::setupUi()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // ======== Row 1: COM Port + Baud ========
    QHBoxLayout *portLayout = new QHBoxLayout();
    portLayout->addWidget(new QLabel("COM Port:"));
    m_portCombo = new QComboBox();
    m_portCombo->setMinimumWidth(140);
    portLayout->addWidget(m_portCombo);
    m_refreshBtn = new QPushButton("Refresh");
    portLayout->addWidget(m_refreshBtn);
    portLayout->addSpacing(20);
    portLayout->addWidget(new QLabel("Baud:"));
    m_baudCombo = new QComboBox();
    m_baudCombo->addItem("9600", 9600);
    m_baudCombo->addItem("19200", 19200);
    m_baudCombo->addItem("38400", 38400);
    m_baudCombo->addItem("57600", 57600);
    m_baudCombo->addItem("115200", 115200);
    m_baudCombo->addItem("230400", 230400);
    m_baudCombo->addItem("460800", 460800);
    m_baudCombo->addItem("921600", 921600);
    m_baudCombo->setCurrentIndex(7);
    portLayout->addWidget(m_baudCombo);
    portLayout->addStretch();
    mainLayout->addLayout(portLayout);

    // ======== Row 2: Media buttons ========
    QHBoxLayout *mediaLayout = new QHBoxLayout();
    m_openImageBtn = new QPushButton("Open Image...");
    mediaLayout->addWidget(m_openImageBtn);
    m_openVideoBtn = new QPushButton("Open Video...");
    mediaLayout->addWidget(m_openVideoBtn);
    m_captureBtn = new QPushButton("Capture Frame");
    m_captureBtn->setEnabled(false);
    mediaLayout->addWidget(m_captureBtn);
    mediaLayout->addStretch();
    mainLayout->addLayout(mediaLayout);

    // ======== Row 3: Processing + FPS ========
    QHBoxLayout *procLayout = new QHBoxLayout();
    procLayout->addWidget(new QLabel("Threshold:"));
    m_thresholdSlider = new QSlider(Qt::Horizontal);
    m_thresholdSlider->setRange(0, 255);
    m_thresholdSlider->setValue(128);
    m_thresholdSlider->setFixedWidth(150);
    procLayout->addWidget(m_thresholdSlider);
    m_thresholdLabel = new QLabel("128");
    m_thresholdLabel->setFixedWidth(35);
    procLayout->addWidget(m_thresholdLabel);
    procLayout->addSpacing(20);
    m_invertCheck = new QCheckBox("Invert Color");
    procLayout->addWidget(m_invertCheck);
    procLayout->addSpacing(20);
    procLayout->addWidget(new QLabel("FPS:"));
    m_fpsSpin = new QSpinBox();
    m_fpsSpin->setRange(1, 60);
    m_fpsSpin->setValue(30);
    m_fpsSpin->setSuffix(" fps");
    m_fpsSpin->setFixedWidth(80);
    procLayout->addWidget(m_fpsSpin);
    procLayout->addStretch();
    m_sendBtn = new QPushButton("Send to STM32");
    m_sendBtn->setEnabled(false);
    m_sendBtn->setMinimumWidth(160);
    procLayout->addWidget(m_sendBtn);
    mainLayout->addLayout(procLayout);

    // ======== Row 4: Preview ========
    QHBoxLayout *previewLayout = new QHBoxLayout();
    QGroupBox *srcGroup = new QGroupBox("Source");
    QVBoxLayout *srcLayout = new QVBoxLayout(srcGroup);
    srcLayout->setContentsMargins(4, 4, 4, 4);
    m_videoWidget = new QVideoWidget();
    m_videoWidget->setFixedSize(384, 216);
    m_videoWidget->setStyleSheet("background-color: #000;");
    m_videoWidget->hide();
    srcLayout->addWidget(m_videoWidget);
    m_sourcePreview = new QLabel();
    m_sourcePreview->setFixedSize(384, 216);
    m_sourcePreview->setAlignment(Qt::AlignCenter);
    m_sourcePreview->setStyleSheet("background-color: #ddd; border: 1px solid #999;");
    srcLayout->addWidget(m_sourcePreview);
    previewLayout->addWidget(srcGroup);

    QGroupBox *oledGroup = new QGroupBox("OLED Preview (128x64)");
    QVBoxLayout *oledLayout = new QVBoxLayout(oledGroup);
    m_oledPreview = new QLabel();
    m_oledPreview->setFixedSize(256, 128);
    m_oledPreview->setAlignment(Qt::AlignCenter);
    m_oledPreview->setStyleSheet("background-color: #000; border: 1px solid #999;");
    oledLayout->addWidget(m_oledPreview);
    previewLayout->addWidget(oledGroup);
    mainLayout->addLayout(previewLayout);

    // ======== Row 5: Status ========
    m_statusLabel = new QLabel("Ready. Connect STM32, select COM, open image or video.");
    mainLayout->addWidget(m_statusLabel);

    // ======== Connections ========
    connect(m_refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshPorts);
    connect(m_openImageBtn, &QPushButton::clicked, this, &MainWindow::onOpenImage);
    connect(m_openVideoBtn, &QPushButton::clicked, this, &MainWindow::onOpenVideo);
    connect(m_captureBtn, &QPushButton::clicked, this, &MainWindow::onCaptureFrame);
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSendOrStream);
    connect(m_thresholdSlider, &QSlider::valueChanged, this, &MainWindow::onThresholdChanged);
    connect(m_invertCheck, &QCheckBox::toggled, this, &MainWindow::onInvertToggled);
    connect(m_streamTimer, &QTimer::timeout, this, &MainWindow::onStreamTick);
    m_mediaPlayer->setVideoOutput(m_videoWidget);
    m_videoSink = m_videoWidget->videoSink();
    connect(m_videoSink, &QVideoSink::videoFrameChanged, this, &MainWindow::onVideoFrameChanged);
    connect(m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &MainWindow::onMediaStateChanged);
}

void MainWindow::refreshPortList()
{
    m_portCombo->clear();
    for (int i = 1; i <= 64; i++) {
        QString portName = QString("COM%1").arg(i);
        QString fullPath = QString("\\\\.\\%1").arg(portName);
        HANDLE h = CreateFileW((LPCWSTR)fullPath.utf16(),
            GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
            m_portCombo->addItem(portName, portName);
        }
    }
}

void MainWindow::onRefreshPorts()
{
    refreshPortList();
    m_statusLabel->setText("Port list refreshed.");
}

bool MainWindow::openSerialPort(const QString &portName, int baudRate)
{
    closeSerialPort();
    QString fullPath = QString("\\\\.\\%1").arg(portName);
    m_hSerial = CreateFileW((LPCWSTR)fullPath.utf16(),
        GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (m_hSerial == INVALID_HANDLE_VALUE) return false;
    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(m_hSerial, &dcb)) { closeSerialPort(); return false; }
    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    if (!SetCommState(m_hSerial, &dcb)) { closeSerialPort(); return false; }
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 5000;
    SetCommTimeouts(m_hSerial, &timeouts);
    return true;
}

void MainWindow::closeSerialPort()
{
    if (m_hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hSerial);
        m_hSerial = INVALID_HANDLE_VALUE;
    }
}

void MainWindow::sendBufferNow()
{
    if (m_hSerial == INVALID_HANDLE_VALUE) return;
    uint8_t header[] = {0xFF, 0xAA};
    DWORD written;
    WriteFile(m_hSerial, header, 2, &written, NULL);
    WriteFile(m_hSerial, m_bitmapBuffer, 1024, &written, NULL);
    uint8_t footer = 0xFE;
    WriteFile(m_hSerial, &footer, 1, &written, NULL);
}

void MainWindow::updateStreamInterval()
{
    int baud = m_baudCombo->currentData().toInt();
    int targetFps = m_fpsSpin->value();
    // 1027 bytes * 10 bits/byte / baud = seconds per frame
    double frameTimeSec = (1027.0 * 10.0) / baud;
    int frameTimeMs = qMax(1, (int)(frameTimeSec * 1000.0));
    int targetIntervalMs = 1000 / targetFps;
    int actualInterval = qMax(frameTimeMs, targetIntervalMs);
    m_streamTimer->setInterval(actualInterval);
    int actualFps = 1000 / actualInterval;
    m_statusLabel->setText(QString("Streaming... (frame %1)  |  %2 baud, ~%3 fps")
        .arg(m_frameCount).arg(baud).arg(actualFps));
}

void MainWindow::onOpenImage()
{
    stopStreaming();
    QString fileName = QFileDialog::getOpenFileName(
        this, "Select Image", QString(),
        "Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp);;All Files (*.*)");
    if (fileName.isEmpty()) return;
    m_mediaPlayer->stop();
    m_videoWidget->hide();
    m_sourcePreview->show();
    m_videoMode = false;
    m_captureBtn->setEnabled(false);
    m_sourceImage.load(fileName);
    if (m_sourceImage.isNull()) {
        QMessageBox::warning(this, "Error", "Failed to load image.");
        return;
    }
    QPixmap pix = QPixmap::fromImage(
        m_sourceImage.scaled(m_sourcePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_sourcePreview->setPixmap(pix);
    processImage();
    m_sendBtn->setEnabled(true);
    m_sendBtn->setText("Send to STM32");
    m_statusLabel->setText(QString("Loaded: %1  |  Click Send").arg(fileName.section('/', -1)));
}

void MainWindow::onOpenVideo()
{
    stopStreaming();
    QString fileName = QFileDialog::getOpenFileName(
        this, "Select Video", QString(),
        "Videos (*.mp4 *.avi *.mkv *.mov *.wmv *.webm);;All Files (*.*)");
    if (fileName.isEmpty()) return;
    m_sourcePreview->hide();
    m_videoWidget->show();
    m_videoMode = true;
    m_hasFrame = false;
    m_captureBtn->setEnabled(true);
    m_sendBtn->setEnabled(false);
    m_sendBtn->setText("Start Video Stream");
    m_frameCount = 0;
    m_mediaPlayer->setSource(QUrl::fromLocalFile(fileName));
    m_mediaPlayer->play();
    m_statusLabel->setText(QString("Loaded: %1 | Playing... | Capture Frame then Start Stream")
        .arg(fileName.section('/', -1)));
}

void MainWindow::onMediaStateChanged(QMediaPlayer::PlaybackState state)
{
    if (m_videoMode && state == QMediaPlayer::StoppedState && m_streaming) {
        m_mediaPlayer->setPosition(0);
        m_mediaPlayer->play();
    }
}

void MainWindow::onVideoFrameChanged(const QVideoFrame &frame)
{
    if (!frame.isValid()) return;
    m_currentFrame = frame.toImage();
    if (m_currentFrame.isNull()) return;
    m_hasFrame = true;
    if (m_streaming) {
        processAndPack(m_currentFrame, m_bitmapBuffer);
        updateOledPreview();
    }
}

void MainWindow::onCaptureFrame()
{
    if (!m_videoMode || !m_hasFrame) {
        m_statusLabel->setText("No frame yet. Wait for video to start playing.");
        return;
    }
    processAndPack(m_currentFrame, m_bitmapBuffer);
    updateOledPreview();
    QImage preview = m_currentFrame.scaled(m_sourcePreview->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_sourcePreview->setPixmap(QPixmap::fromImage(preview));
    m_sendBtn->setEnabled(true);
    m_sendBtn->setText("Start Video Stream");
    m_statusLabel->setText("Frame captured! Click 'Start Video Stream' to send to OLED.");
}

void MainWindow::onSendOrStream()
{
    if (m_streaming) {
        stopStreaming();
        return;
    }
    if (m_portCombo->count() == 0) {
        QMessageBox::warning(this, "Error", "No COM port available.");
        return;
    }
    QString portName = m_portCombo->currentData().toString();
    int baud = m_baudCombo->currentData().toInt();
    if (m_videoMode) {
        if (!m_hasFrame) {
            QMessageBox::warning(this, "Error", "No frame yet. Click Capture Frame first.");
            return;
        }
        startStreaming();
    } else {
        if (!openSerialPort(portName, baud)) {
            QMessageBox::warning(this, "Error",
                QString("Failed to open %1.").arg(portName));
            return;
        }
        sendBufferNow();
        closeSerialPort();
        m_statusLabel->setText("Image sent! Check OLED.");
    }
}

void MainWindow::startStreaming()
{
    if (m_portCombo->count() == 0) return;
    QString portName = m_portCombo->currentData().toString();
    int baud = m_baudCombo->currentData().toInt();
    if (!openSerialPort(portName, baud)) {
        QMessageBox::warning(this, "Error", QString("Failed to open %1.").arg(portName));
        return;
    }
    m_streaming = true;
    m_frameCount = 0;
    if (m_mediaPlayer->playbackState() != QMediaPlayer::PlayingState)
        m_mediaPlayer->play();
    processAndPack(m_currentFrame, m_bitmapBuffer);
    sendBufferNow();
    updateOledPreview();
    m_frameCount++;
    updateStreamInterval();
    m_streamTimer->start();
    m_sendBtn->setText("Stop Streaming");
    m_sendBtn->setStyleSheet("background-color: #c44; color: white;");
    m_captureBtn->setEnabled(false);
}

void MainWindow::stopStreaming()
{
    m_streamTimer->stop();
    if (m_streaming) {
        closeSerialPort();
        m_streaming = false;
        m_sendBtn->setText("Start Video Stream");
        m_sendBtn->setStyleSheet("");
        m_captureBtn->setEnabled(true);
        m_statusLabel->setText(QString("Stopped. Sent %1 frames.").arg(m_frameCount));
    }
}

void MainWindow::onStreamTick()
{
    if (!m_streaming || !m_hasFrame) return;
    processAndPack(m_currentFrame, m_bitmapBuffer);
    sendBufferNow();
    updateOledPreview();
    m_frameCount++;
    updateStreamInterval();
}

void MainWindow::onThresholdChanged(int value)
{
    m_thresholdLabel->setText(QString::number(value));
    if (m_videoMode && m_hasFrame) {
        processAndPack(m_currentFrame, m_bitmapBuffer);
        updateOledPreview();
    } else if (!m_videoMode && !m_sourceImage.isNull()) {
        processImage();
    }
}

void MainWindow::onInvertToggled(bool)
{
    if (m_videoMode && m_hasFrame) {
        processAndPack(m_currentFrame, m_bitmapBuffer);
        updateOledPreview();
    } else if (!m_videoMode && !m_sourceImage.isNull()) {
        processImage();
    }
}

void MainWindow::processImage()
{
    processAndPack(m_sourceImage, m_bitmapBuffer);
    updateOledPreview();
}

void MainWindow::processAndPack(const QImage &src, uint8_t *buffer)
{
    int threshold = m_thresholdSlider->value();
    bool invert = m_invertCheck->isChecked();
    QImage scaled = src.scaled(128, 64, Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                        .convertToFormat(QImage::Format_Grayscale8);
    for (int y = 0; y < 64; y++) {
        uchar *line = scaled.scanLine(y);
        for (int x = 0; x < 128; x++) {
            bool white = (line[x] >= threshold);
            if (invert) white = !white;
            line[x] = white ? 255 : 0;
        }
    }
    m_convertedImage = scaled;
    memset(buffer, 0, 1024);
    for (int page = 0; page < 8; page++) {
        for (int col = 0; col < 128; col++) {
            uint8_t byteVal = 0;
            for (int bit = 0; bit < 8; bit++) {
                int y = page * 8 + bit;
                if (m_convertedImage.constScanLine(y)[col] < 128)
                    byteVal |= (1 << bit);
            }
            buffer[page * 128 + col] = byteVal;
        }
    }
}

void MainWindow::updateOledPreview()
{
    QImage preview = m_convertedImage.scaled(256, 128, Qt::KeepAspectRatio);
    m_oledPreview->setPixmap(QPixmap::fromImage(preview));
}
