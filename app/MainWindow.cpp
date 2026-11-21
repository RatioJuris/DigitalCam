#include "MainWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QSlider>
#include <QSizePolicy>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

#include <utility>

#include <digitalcam/core/playback/PlaybackState.h>

EngineBridge::EngineBridge(QObject* parent) : QObject(parent) {}

void EngineBridge::onStatus(const digitalcam::core::CoreEngine::Status& status) {
    const QString state = QString::fromUtf8(digitalcam::core::toString(status.state));
    const QString media = QString::fromStdString(status.mediaName);
    const QString camera = status.cameraAvailable ? QStringLiteral("Running") : QStringLiteral("Stopped");
    const QString audio = status.audioAvailable ? QStringLiteral("Running") : QStringLiteral("Stopped");
    const QString looping = status.looping ? QStringLiteral("On") : QStringLiteral("Off");
    const QString position = QString::number(status.position.count() / 1000000) + QStringLiteral(" ms");
    emit statusChanged(state, media, camera, audio, looping, position);
}

void EngineBridge::onError(const std::string& message) {
    emit errorOccurred(QString::fromStdString(message));
}

void EngineBridge::onFrame(const digitalcam::core::VideoFrame& frame) {
    if (!frame.size.valid() || frame.data.empty() || frame.format != digitalcam::core::PixelFormat::Bgra) {
        return;
    }
    const QImage image(
        frame.data.data(),
        frame.size.width,
        frame.size.height,
        static_cast<int>(frame.stride),
        QImage::Format_ARGB32
    );
    emit frameReady(image.copy());
}

MainWindow::MainWindow(
    std::shared_ptr<digitalcam::core::CoreEngine> engine,
    QWidget* parent
) : QMainWindow(parent), engine_(std::move(engine)), bridge_(this) {
    buildUi();
    connectUi();

    engine_->setStatusCallback([this](const digitalcam::core::CoreEngine::Status& status) {
        bridge_.onStatus(status);
    });
    engine_->setErrorCallback([this](const std::string& message) {
        bridge_.onError(message);
    });
    engine_->setFrameCallback([this](const digitalcam::core::VideoFrame& frame) {
        bridge_.onFrame(frame);
    });
}

MainWindow::~MainWindow() {
    engine_->setStatusCallback({});
    engine_->setErrorCallback({});
    engine_->setFrameCallback({});
}

void MainWindow::buildUi() {
    setWindowTitle(QStringLiteral("DigitalCam Studio"));
    resize(1180, 760);

    previewLabel_ = new QLabel(this);
    previewLabel_->setAlignment(Qt::AlignCenter);
    previewLabel_->setMinimumSize(640, 360);
    previewLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    previewLabel_->setStyleSheet(QStringLiteral("background: #101014; border: 1px solid #303038;"));
    previewLabel_->setText(QStringLiteral("No camera stream"));

    stateLabel_ = new QLabel(QStringLiteral("idle"), this);
    mediaLabel_ = new QLabel(QStringLiteral("No media"), this);
    cameraLabel_ = new QLabel(QStringLiteral("Stopped"), this);
    audioLabel_ = new QLabel(QStringLiteral("Stopped"), this);
    loopLabel_ = new QLabel(QStringLiteral("On"), this);
    positionLabel_ = new QLabel(QStringLiteral("0 ms"), this);
    errorLabel_ = new QLabel(this);
    errorLabel_->setWordWrap(true);
    errorLabel_->setStyleSheet(QStringLiteral("color: #ff8080;"));

    selectButton_ = new QPushButton(QStringLiteral("Select Media"), this);
    startButton_ = new QPushButton(QStringLiteral("Start Camera"), this);
    stopButton_ = new QPushButton(QStringLiteral("Stop Camera"), this);
    playButton_ = new QPushButton(QStringLiteral("Play"), this);
    pauseButton_ = new QPushButton(QStringLiteral("Pause"), this);
    restartButton_ = new QPushButton(QStringLiteral("Restart"), this);

    loopBox_ = new QCheckBox(QStringLiteral("Loop video"), this);
    loopBox_->setChecked(true);
    muteBox_ = new QCheckBox(QStringLiteral("Mute"), this);

    volumeSlider_ = new QSlider(Qt::Horizontal, this);
    volumeSlider_->setRange(0, 400);
    volumeSlider_->setValue(100);

    resolutionBox_ = new QComboBox(this);
    resolutionBox_->addItems({
        QStringLiteral("640 x 480"),
        QStringLiteral("1280 x 720"),
        QStringLiteral("1920 x 1080")
    });
    resolutionBox_->setCurrentText(QStringLiteral("1920 x 1080"));

    frameRateBox_ = new QComboBox(this);
    frameRateBox_->addItems({QStringLiteral("30"), QStringLiteral("60")});

    aspectBox_ = new QComboBox(this);
    aspectBox_->addItems({
        QStringLiteral("Fit"),
        QStringLiteral("Fill"),
        QStringLiteral("Original"),
        QStringLiteral("Crop")
    });

    auto mediaLayout = new QHBoxLayout();
    mediaLayout->addWidget(selectButton_);
    mediaLayout->addWidget(playButton_);
    mediaLayout->addWidget(pauseButton_);
    mediaLayout->addWidget(restartButton_);

    auto deviceLayout = new QHBoxLayout();
    deviceLayout->addWidget(startButton_);
    deviceLayout->addWidget(stopButton_);

    auto statusForm = new QFormLayout();
    statusForm->addRow(QStringLiteral("State"), stateLabel_);
    statusForm->addRow(QStringLiteral("Media"), mediaLabel_);
    statusForm->addRow(QStringLiteral("Camera"), cameraLabel_);
    statusForm->addRow(QStringLiteral("Audio"), audioLabel_);
    statusForm->addRow(QStringLiteral("Loop"), loopLabel_);
    statusForm->addRow(QStringLiteral("Position"), positionLabel_);

    auto settingsForm = new QFormLayout();
    settingsForm->addRow(QStringLiteral("Resolution"), resolutionBox_);
    settingsForm->addRow(QStringLiteral("Frame rate"), frameRateBox_);
    settingsForm->addRow(QStringLiteral("Aspect"), aspectBox_);
    settingsForm->addRow(QStringLiteral("Volume"), volumeSlider_);
    settingsForm->addRow(QStringLiteral("Audio"), muteBox_);

    auto statusGroup = new QGroupBox(QStringLiteral("Status"), this);
    statusGroup->setLayout(statusForm);
    auto settingsGroup = new QGroupBox(QStringLiteral("Output settings"), this);
    settingsGroup->setLayout(settingsForm);

    auto sidePanel = new QVBoxLayout();
    sidePanel->addLayout(mediaLayout);
    sidePanel->addLayout(deviceLayout);
    sidePanel->addWidget(statusGroup);
    sidePanel->addWidget(settingsGroup);
    sidePanel->addWidget(loopBox_);
    sidePanel->addWidget(errorLabel_);
    sidePanel->addStretch();

    auto mainLayout = new QHBoxLayout();
    mainLayout->addWidget(previewLabel_, 3);
    mainLayout->addLayout(sidePanel, 1);

    auto central = new QWidget(this);
    central->setLayout(mainLayout);
    setCentralWidget(central);
}

void MainWindow::connectUi() {
    QObject::connect(selectButton_, &QPushButton::clicked, this, &MainWindow::chooseMedia);
    QObject::connect(startButton_, &QPushButton::clicked, this, &MainWindow::startCamera);
    QObject::connect(stopButton_, &QPushButton::clicked, this, &MainWindow::stopCamera);
    QObject::connect(playButton_, &QPushButton::clicked, this, &MainWindow::playMedia);
    QObject::connect(pauseButton_, &QPushButton::clicked, this, &MainWindow::pauseMedia);
    QObject::connect(restartButton_, &QPushButton::clicked, this, &MainWindow::restartMedia);
    QObject::connect(loopBox_, &QCheckBox::toggled, this, &MainWindow::updateLooping);
    QObject::connect(muteBox_, &QCheckBox::toggled, this, &MainWindow::updateMuted);
    QObject::connect(volumeSlider_, &QSlider::valueChanged, this, &MainWindow::updateVolume);
    QObject::connect(aspectBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        applyAspectMode();
    });

    QObject::connect(
        &bridge_,
        &EngineBridge::statusChanged,
        this,
        &MainWindow::applyStatus,
        Qt::QueuedConnection
    );
    QObject::connect(
        &bridge_,
        &EngineBridge::errorOccurred,
        this,
        &MainWindow::showError,
        Qt::QueuedConnection
    );
    QObject::connect(
        &bridge_,
        &EngineBridge::frameReady,
        this,
        &MainWindow::showFrame,
        Qt::QueuedConnection
    );
}

void MainWindow::chooseMedia() {
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Select image or video"),
        {},
        QStringLiteral("Media files (*.jpg *.jpeg *.png *.bmp *.webp *.tif *.tiff *.mp4 *.mov *.mkv *.avi *.webm *.mpeg *.mpg)")
    );
    if (path.isEmpty()) {
        return;
    }
    errorLabel_->clear();
    engine_->loadMedia(path.toStdString(), true);
}

void MainWindow::startCamera() {
    const QStringList dimensions = resolutionBox_->currentText().split(
        QRegularExpression(QStringLiteral("\\s+x\\s+"))
    );
    if (dimensions.size() != 2) {
        showError(QStringLiteral("Invalid resolution"));
        return;
    }

    digitalcam::core::VideoOutputFormat format;
    format.size.width = dimensions.at(0).toInt();
    format.size.height = dimensions.at(1).toInt();
    format.frameRate = frameRateBox_->currentText().toDouble();
    format.format = digitalcam::core::PixelFormat::Bgra;

    applyAspectMode();
    const digitalcam::core::Result result = engine_->startCamera(format);
    if (!result) {
        showError(QString::fromStdString(result.message));
    }
}

void MainWindow::stopCamera() {
    engine_->stopCamera();
}

void MainWindow::playMedia() {
    engine_->play();
}

void MainWindow::pauseMedia() {
    engine_->pause();
}

void MainWindow::restartMedia() {
    engine_->seek(digitalcam::core::Timestamp{0});
    engine_->play();
}

void MainWindow::updateLooping(bool enabled) {
    engine_->setLooping(enabled);
}

void MainWindow::updateMuted(bool muted) {
    engine_->setMuted(muted);
}

void MainWindow::updateVolume(int value) {
    engine_->setVolume(static_cast<float>(value) / 100.0F);
}

void MainWindow::applyStatus(
    const QString& state,
    const QString& media,
    const QString& camera,
    const QString& audio,
    const QString& looping,
    const QString& position
) {
    stateLabel_->setText(state);
    mediaLabel_->setText(media.isEmpty() ? QStringLiteral("No media") : media);
    cameraLabel_->setText(camera);
    audioLabel_->setText(audio);
    loopLabel_->setText(looping);
    positionLabel_->setText(position);
}

void MainWindow::showError(const QString& message) {
    errorLabel_->setText(message);
}

void MainWindow::showFrame(const QImage& image) {
    if (image.isNull()) {
        return;
    }
    const QSize previewSize = previewLabel_->size();
    previewLabel_->setPixmap(
        QPixmap::fromImage(image).scaled(previewSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)
    );
}

void MainWindow::applyAspectMode() {
    using digitalcam::core::AspectMode;
    switch (aspectBox_->currentIndex()) {
        case 0: engine_->setAspectMode(AspectMode::Fit); break;
        case 1: engine_->setAspectMode(AspectMode::Fill); break;
        case 2: engine_->setAspectMode(AspectMode::Original); break;
        case 3: engine_->setAspectMode(AspectMode::Crop); break;
        default: engine_->setAspectMode(AspectMode::Fit); break;
    }
}
