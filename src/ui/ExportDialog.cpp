#include "ui/ExportDialog.h"
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
namespace pms {
ExportDialog::ExportDialog(Project p, QWidget *parent) : QDialog(parent), project_(std::move(p)) {
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Export movie"));
    setMinimumWidth(560);
    auto *layout = new QVBoxLayout(this);
    auto *heading = new QLabel(tr("Your movie, ready to share"));
    heading->setObjectName("dialogTitle");
    layout->addWidget(heading);
    fields_ = new QWidget;
    auto *form = new QFormLayout(fields_);
    format_ = new QComboBox;
    format_->addItems({"MP4", "MKV", "MP3", "WAV"});
    form->addRow(tr("Format"), format_);
    orientation_ = new QComboBox;
    orientation_->setObjectName("exportOrientation");
    orientation_->addItems({tr("Landscape 16:9"), tr("Portrait 9:16")});
    orientation_->setCurrentIndex(project_.settings.height > project_.settings.width ? 1 : 0);
    form->addRow(tr("Orientation"), orientation_);
    resolution_ = new QComboBox;
    resolution_->addItem("1920 × 1080", 1080);
    resolution_->addItem("1280 × 720", 720);
    form->addRow(tr("Resolution"), resolution_);
    auto sizes = [this] {
        bool p = orientation_->currentIndex() == 1;
        resolution_->setItemText(0, p ? "1080 × 1920" : "1920 × 1080");
        resolution_->setItemText(1, p ? "720 × 1280" : "1280 × 720");
    };
    sizes();
    connect(orientation_, &QComboBox::currentIndexChanged, this, sizes);
    fps_ = new QComboBox;
    fps_->addItems({"24", "25", "30", "60"});
    fps_->setCurrentText(QString::number(project_.settings.fpsNum));
    form->addRow(tr("Frames per second"), fps_);
    quality_ = new QComboBox;
    quality_->addItem(tr("Low"), "low");
    quality_->addItem(tr("Medium"), "medium");
    quality_->addItem(tr("High"), "high");
    quality_->setCurrentIndex(1);
    form->addRow(tr("Quality"), quality_);
    path_ = new QLineEdit(QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) +
                          "/Perplex Movie.mp4");
    browse_ = new QPushButton(tr("Browse…"));
    auto *pathBox = new QWidget;
    auto *row = new QHBoxLayout(pathBox);
    row->setContentsMargins(0, 0, 0, 0);
    row->addWidget(path_);
    row->addWidget(browse_);
    form->addRow(tr("Destination"), pathBox);
    layout->addWidget(fields_);
    auto *info = new QLabel(tr("Video exports use software MPEG-4 Part 2 and AAC stereo audio. A "
                               "silent project receives a silent audio track."));
    info->setWordWrap(true);
    info->setObjectName("mutedText");
    layout->addWidget(info);
    progress_ = new QProgressBar;
    progress_->setRange(0, 1000);
    progress_->setValue(0);
    progress_->setFormat("%p%");
    layout->addWidget(progress_);
    status_ = new QLabel(tr("Ready to export locally. No watermark."));
    status_->setWordWrap(true);
    layout->addWidget(status_);
    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    cancel_ = new QPushButton(tr("Close"));
    export_ = new QPushButton(tr("Export"));
    export_->setObjectName("primaryButton");
    buttons->addWidget(cancel_);
    buttons->addWidget(export_);
    layout->addLayout(buttons);
    connect(browse_, &QPushButton::clicked, this, [this] {
        auto path = QFileDialog::getSaveFileName(this, tr("Export destination"), path_->text(),
                                                 format_->currentText() + " (*." +
                                                     format_->currentText().toLower() + ")",
                                                 nullptr, QFileDialog::DontConfirmOverwrite);
        if (!path.isEmpty())
            path_->setText(path);
    });
    connect(format_, &QComboBox::currentIndexChanged, this, [this](int i) {
        bool video = i < 2;
        resolution_->setEnabled(video);
        orientation_->setEnabled(video);
        fps_->setEnabled(video);
        quality_->setEnabled(video);
        QFileInfo f(path_->text());
        path_->setText(f.absolutePath() + "/" + f.completeBaseName() + "." +
                       format_->currentText().toLower());
    });
    connect(export_, &QPushButton::clicked, this, &ExportDialog::start);
    connect(cancel_, &QPushButton::clicked, this, &ExportDialog::stop);
}
void ExportDialog::start() {
    if (job_ || path_->text().trimmed().isEmpty())
        return;
    auto path = path_->text().trimmed(), format = format_->currentText().toLower();
    if (!path.endsWith("." + format, Qt::CaseInsensitive))
        path += "." + format;
    path_->setText(path);
    bool overwrite = QFileInfo::exists(path);
    if (overwrite &&
        QMessageBox::question(
            this, tr("Replace file?"),
            tr("Replace the existing destination after the export completes successfully?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    RenderProfile profile;
    profile.format = format;
    profile.height = resolution_->currentData().toInt();
    profile.width = profile.height == 1080 ? 1920 : 1280;
    if (orientation_->currentIndex() == 1)
        std::swap(profile.width, profile.height);
    profile.fps = fps_->currentText().toInt();
    profile.quality = quality_->currentData().toString();
    job_ = new RenderJob(this);
    fields_->setEnabled(false);
    export_->setEnabled(false);
    cancel_->setText(tr("Cancel export"));
    status_->setText(tr("Rendering… You can continue editing the project."));
    auto reset = [this] {
        job_->deleteLater();
        job_ = nullptr;
        fields_->setEnabled(true);
        export_->setEnabled(true);
        cancel_->setText(tr("Close"));
    };
    connect(job_, &RenderJob::progress, this,
            [this](double p) { progress_->setValue(int(p * 1000)); });
    connect(job_, &RenderJob::succeeded, this,
            [this, reset](const auto &path, const auto &encoder) {
                status_->setText(tr("Export complete: %1\nEncoder: %2")
                                     .arg(QDir::toNativeSeparators(path), encoder));
                reset();
            });
    connect(job_, &RenderJob::failed, this, [this, reset](const auto &message, const auto &detail) {
        status_->setText(message);
        QMessageBox box(QMessageBox::Warning, tr("Export failed"), message, QMessageBox::Ok, this);
        box.setDetailedText(detail);
        box.exec();
        reset();
    });
    connect(job_, &RenderJob::cancelled, this, [this, reset] {
        status_->setText(tr("Export cancelled. Existing files were preserved."));
        reset();
    });
    job_->start(project_, profile, path, overwrite);
}
void ExportDialog::stop() {
    if (job_) {
        status_->setText(tr("Cancelling…"));
        job_->cancel();
    } else
        close();
}
void ExportDialog::closeEvent(QCloseEvent *e) {
    if (job_) {
        if (QMessageBox::question(this, tr("Cancel export?"),
                                  tr("An export is running. Cancel it and close this window?"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No) != QMessageBox::Yes) {
            e->ignore();
            return;
        }
        job_->cancel();
    }
    e->accept();
}
} // namespace pms
