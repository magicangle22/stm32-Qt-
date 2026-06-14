#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QProgressBar>
#include <QCheckBox>
#include <QImage>
#include <QTimer>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoWidget>
#include <QVideoFrame>
#include <QVideoFrameFormat>
#include <windows.h>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRefreshPorts();
    void onOpenImage();
    void onOpenVideo();
    void onCaptureFrame();
    void onSendOrStream();
    void onThresholdChanged(int value);
    void onInvertToggled(bool checked);
    void onStreamTick();
    void onVideoFrameChanged(const QVideoFrame &frame);
    void onMediaStateChanged(QMediaPlayer::PlaybackState state);

private:
    void setupUi();
    void refreshPortList();
    void processImage();
    void processAndPack(const QImage &src, uint8_t *buffer);
    bool openSerialPort(const QString &portName, int baudRate);
    void closeSerialPort();
    void sendBufferNow();
    void startStreaming();
    void stopStreaming();
    void updateOledPreview();
    void updateStreamInterval();

    // Serial
    HANDLE m_hSerial;

    // UI controls
    QComboBox *m_portCombo;
    QComboBox *m_baudCombo;
    QPushButton *m_refreshBtn;
    QPushButton *m_openImageBtn;
    QPushButton *m_openVideoBtn;
    QPushButton *m_captureBtn;
    QPushButton *m_sendBtn;
    QSlider *m_thresholdSlider;
    QLabel *m_thresholdLabel;
    QCheckBox *m_invertCheck;
    QSpinBox *m_fpsSpin;
    QLabel *m_sourcePreview;
    QLabel *m_oledPreview;
    QLabel *m_statusLabel;

    // Video
    QMediaPlayer *m_mediaPlayer;
    QVideoWidget *m_videoWidget;
    QVideoSink *m_videoSink;
    QImage m_currentFrame;
    bool m_videoMode;

    // Image processing
    QImage m_sourceImage;
    QImage m_convertedImage;
    uint8_t m_bitmapBuffer[1024];
    bool m_hasFrame;

    // Streaming
    QTimer *m_streamTimer;
    bool m_streaming;
    int m_frameCount;
};

#endif // MAINWINDOW_H
