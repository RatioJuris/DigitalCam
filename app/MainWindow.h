#pragma once

#include <memory>
#include <string>

#include <QImage>
#include <QMainWindow>
#include <QObject>
#include <QString>

#include <digitalcam/core/CoreEngine.h>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSlider;

class EngineBridge final : public QObject {
    Q_OBJECT

public:
    explicit EngineBridge(QObject* parent = nullptr);

public slots:
    void onStatus(const digitalcam::core::CoreEngine::Status& status);
    void onError(const std::string& message);
    void onFrame(const digitalcam::core::VideoFrame& frame);

signals:
    void statusChanged(
        const QString& state,
        const QString& media,
        const QString& camera,
        const QString& audio,
        const QString& looping,
        const QString& position
    );
    void errorOccurred(const QString& message);
    void frameReady(const QImage& image);
};

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(std::shared_ptr<digitalcam::core::CoreEngine> engine, QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void chooseMedia();
    void startCamera();
    void stopCamera();
    void playMedia();
    void pauseMedia();
    void restartMedia();
    void updateLooping(bool enabled);
    void updateMuted(bool muted);
    void updateVolume(int value);
    void applyStatus(
        const QString& state,
        const QString& media,
        const QString& camera,
        const QString& audio,
        const QString& looping,
        const QString& position
    );
    void showError(const QString& message);
    void showFrame(const QImage& image);

private:
    void buildUi();
    void connectUi();
    void applyAspectMode();

    std::shared_ptr<digitalcam::core::CoreEngine> engine_;
    EngineBridge bridge_;

    QLabel* previewLabel_ = nullptr;
    QLabel* stateLabel_ = nullptr;
    QLabel* mediaLabel_ = nullptr;
    QLabel* cameraLabel_ = nullptr;
    QLabel* audioLabel_ = nullptr;
    QLabel* loopLabel_ = nullptr;
    QLabel* positionLabel_ = nullptr;
    QLabel* errorLabel_ = nullptr;

    QPushButton* selectButton_ = nullptr;
    QPushButton* startButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;
    QPushButton* playButton_ = nullptr;
    QPushButton* pauseButton_ = nullptr;
    QPushButton* restartButton_ = nullptr;

    QCheckBox* loopBox_ = nullptr;
    QCheckBox* muteBox_ = nullptr;
    QSlider* volumeSlider_ = nullptr;

    QComboBox* resolutionBox_ = nullptr;
    QComboBox* frameRateBox_ = nullptr;
    QComboBox* aspectBox_ = nullptr;
};
