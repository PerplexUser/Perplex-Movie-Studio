#include "ui/MainWindow.h"
#include "ui/PreviewWidget.h"
#include "ui/Timeline.h"
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <cmath>
namespace pms {
void MainWindow::buildFeatureControls(QVBoxLayout *layout) {
    inspectorTabs_ = new QTabWidget;
    inspectorTabs_->setObjectName("inspectorTabs");
    layout->removeWidget(inspectorFields_);
    inspectorTabs_->addTab(inspectorFields_, QString());
    visualGroup_ = new QGroupBox;
    titleGroup_ = new QGroupBox;
    inspectorTabs_->addTab(visualGroup_, QString());
    inspectorTabs_->addTab(titleGroup_, QString());
    layout->addWidget(inspectorTabs_);
    translations_.push_back([this] {
        inspectorTabs_->setTabText(0, tr("Clip"));
        inspectorTabs_->setTabText(1, tr("Picture"));
        inspectorTabs_->setTabText(2, tr("Text"));
    });
    auto label = [this](const char *text) {
        auto *l = new QLabel;
        l->setWordWrap(true);
        translations_.push_back([this, l, text] { l->setText(tr(text)); });
        return l;
    };
    auto number = [&](QFormLayout *f, const char *text, const char *name, double lo, double hi,
                      int decimals) {
        auto *n = new QDoubleSpinBox;
        n->setObjectName(name);
        n->setRange(lo, hi);
        n->setDecimals(decimals);
        n->setKeyboardTracking(false);
        n->setSingleStep(decimals ? .1 : 1);
        f->addRow(label(text), n);
        return n;
    };
    auto *form = new QFormLayout(visualGroup_);
    form->setContentsMargins(0, 8, 0, 0);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    brightness_ = number(form, QT_TR_NOOP("Brightness (%)"), "brightness", -100, 100, 0);
    contrast_ = number(form, QT_TR_NOOP("Contrast (%)"), "contrast", 0, 300, 0);
    saturation_ = number(form, QT_TR_NOOP("Saturation (%)"), "saturation", 0, 300, 0);
    hue_ = number(form, QT_TR_NOOP("Hue (degrees)"), "hue", -180, 180, 0);
    auto *reset = new QPushButton;
    translations_.push_back([this, reset] { reset->setText(tr("Reset colors")); });
    form->addRow(reset);
    transition_ = new QComboBox;
    transition_->setObjectName("visualTransition");
    transition_->addItems({QString(), QString(), QString()});
    translations_.push_back([this] {
        transition_->setItemText(0, tr("Fade"));
        transition_->setItemText(1, tr("Wipe"));
        transition_->setItemText(2, tr("Slide"));
    });
    form->addRow(label(QT_TR_NOOP("Transition")), transition_);
    visualIn_ = number(form, QT_TR_NOOP("Picture fade in (s)"), "visualFadeIn", 0, 86400, 3);
    visualOut_ = number(form, QT_TR_NOOP("Picture fade out (s)"), "visualFadeOut", 0, 86400, 3);
    form->addRow(
        label(QT_TR_NOOP("Overlap clips on different video tracks for a transition between them. "
                         "On an empty background, the transition reveals black.")));
    for (auto *n : {brightness_, contrast_, saturation_, hue_, visualIn_, visualOut_})
        connect(n, &QDoubleSpinBox::editingFinished, this, &MainWindow::applyVisual);
    connect(transition_, &QComboBox::activated, this, &MainWindow::applyVisual);
    connect(reset, &QPushButton::clicked, this, [this] {
        if (session_.selected.empty())
            return;
        auto v = session_.project().clip(session_.selected).visual;
        v.brightness = 0;
        v.contrast = 1;
        v.saturation = 1;
        v.hue = 0;
        session_.edit([&](Editor &e) { e.visual(session_.selected, v); });
    });
    auto *tf = new QFormLayout(titleGroup_);
    tf->setContentsMargins(0, 8, 0, 0);
    tf->setRowWrapPolicy(QFormLayout::WrapLongRows);
    tf->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    titleText_ = new QPlainTextEdit;
    titleText_->setObjectName("titleText");
    titleText_->setMinimumHeight(85);
    titleText_->setMaximumHeight(110);
    tf->addRow(label(QT_TR_NOOP("Text")));
    tf->addRow(titleText_);
    titleDuration_ =
        number(tf, QT_TR_NOOP("Display duration (s)"), "titleDuration", .001, 86400, 3);
    connect(titleDuration_, &QDoubleSpinBox::editingFinished, this, [this] {
        if (updating_ || session_.selected.empty())
            return;
        auto c = session_.project().clip(session_.selected);
        session_.edit([&](Editor &e) {
            e.trim(c.id, c.sourceIn,
                   c.sourceIn + Time(std::llround(titleDuration_->value() * Second)), c.start);
        });
    });
    titleFont_ = new QFontComboBox;
    titleFont_->setObjectName("titleFont");
    tf->addRow(label(QT_TR_NOOP("Font")), titleFont_);
    titleSize_ = number(tf, QT_TR_NOOP("Font size (px)"), "titleSize", 8, 500, 0);
    titleBold_ = new QCheckBox;
    tf->addRow(label(QT_TR_NOOP("Bold")), titleBold_);
    titleColor_ = new QPushButton;
    titleColor_->setObjectName("titleColor");
    tf->addRow(label(QT_TR_NOOP("Text color")), titleColor_);
    titleX_ = number(tf, QT_TR_NOOP("Horizontal position (%)"), "titleX", 0, 100, 1);
    titleY_ = number(tf, QT_TR_NOOP("Vertical position (%)"), "titleY", 0, 100, 1);
    titleWidth_ = number(tf, QT_TR_NOOP("Text width (%)"), "titleWidth", 5, 100, 0);
    tf->addRow(label(QT_TR_NOOP("Click or drag in the prepared preview to place the text. Picture "
                                "transitions also apply to text.")));
    titleTimer_.setSingleShot(true);
    titleTimer_.setInterval(450);
    connect(&titleTimer_, &QTimer::timeout, this, &MainWindow::flushTitleText);
    connect(titleText_, &QPlainTextEdit::textChanged, this, [this] {
        if (updating_ || session_.selected.empty())
            return;
        pendingTitleId_ = session_.selected;
        pendingTitleText_ = titleText_->toPlainText();
        titleTimer_.start();
    });
    for (auto *n : {titleX_, titleY_, titleWidth_, titleSize_})
        connect(n, &QDoubleSpinBox::editingFinished, this, &MainWindow::applyTitleProperties);
    connect(titleFont_, &QFontComboBox::currentFontChanged, this, [this] {
        if (!updating_)
            applyTitleProperties();
    });
    connect(titleBold_, &QCheckBox::clicked, this, &MainWindow::applyTitleProperties);
    connect(titleColor_, &QPushButton::clicked, this, [this] {
        auto c = QColorDialog::getColor(QColor(titleColorValue_), this, tr("Text color"));
        if (c.isValid()) {
            titleColorValue_ = c.name();
            applyTitleProperties();
        }
    });
    connect(video_, &PreviewWidget::textPositionRequested, this, [this](double x, double y) {
        if (session_.selected.empty() || previewJob_)
            return;
        auto c = session_.project().clip(session_.selected);
        if (session_.project().asset(c.assetId).kind != AssetKind::Title)
            return;
        c.title.x = x;
        c.title.y = y;
        if (session_.edit([&](Editor &e) { e.title(c.id, c.title); }))
            startPreview(false);
    });
}
void MainWindow::updateFeatureInspector() {
    const bool has = !session_.selected.empty();
    bool picture = false, text = false, locked = true;
    if (has) {
        const auto &c = session_.project().clip(session_.selected);
        const auto &t = session_.project().track(c.trackId);
        picture = t.type == TrackType::Video;
        text = session_.project().asset(c.assetId).kind == AssetKind::Title;
        locked = t.locked;
    }
    visualGroup_->setEnabled(picture && !locked);
    titleGroup_->setEnabled(text && !locked);
    inspectorTabs_->setTabEnabled(1, picture);
    inspectorTabs_->setTabEnabled(2, text);
    video_->setPlacementEnabled(text && !locked);
    if (!has)
        return;
    if (inspectedClip_ != session_.selected) {
        inspectedClip_ = session_.selected;
        inspectorTabs_->setCurrentIndex(text ? 2 : 0);
    }
    const auto &c = session_.project().clip(session_.selected);
    const auto &v = c.visual;
    brightness_->setValue(v.brightness * 100);
    contrast_->setValue(v.contrast * 100);
    saturation_->setValue(v.saturation * 100);
    hue_->setValue(v.hue);
    visualIn_->setMaximum(double(v.envelopeOut - v.envelopeIn) / Second);
    visualOut_->setMaximum(double(v.envelopeOut - v.envelopeIn) / Second);
    visualIn_->setValue(double(v.fadeIn) / Second);
    visualOut_->setValue(double(v.fadeOut) / Second);
    transition_->setCurrentIndex(int(v.transition));
    if (text) {
        if (pendingTitleId_ != c.id)
            titleText_->setPlainText(QString::fromStdString(c.title.text));
        titleDuration_->setValue(double(c.duration()) / Second);
        titleFont_->setCurrentFont(QFont(QString::fromStdString(c.title.font)));
        titleSize_->setValue(c.title.size);
        titleBold_->setChecked(c.title.bold);
        titleColorValue_ = QString::fromStdString(c.title.color);
        titleColor_->setText(titleColorValue_);
        titleX_->setValue(c.title.x * 100);
        titleY_->setValue(c.title.y * 100);
        titleWidth_->setValue(c.title.width * 100);
    }
}
void MainWindow::applyVisual() {
    if (updating_ || session_.selected.empty())
        return;
    auto c = session_.project().clip(session_.selected);
    auto v = c.visual;
    v.brightness = brightness_->value() / 100;
    v.contrast = contrast_->value() / 100;
    v.saturation = saturation_->value() / 100;
    v.hue = hue_->value();
    v.transition = Transition(transition_->currentIndex());
    v.fadeIn = Time(std::llround(visualIn_->value() * Second));
    v.fadeOut = Time(std::llround(visualOut_->value() * Second));
    session_.edit([&](Editor &e) { e.visual(c.id, v); });
    updateInspector();
}
void MainWindow::applyTitleProperties() {
    if (updating_ || session_.selected.empty())
        return;
    auto c = session_.project().clip(session_.selected);
    if (session_.project().asset(c.assetId).kind != AssetKind::Title)
        return;
    c.title.text = titleText_->toPlainText().toStdString();
    c.title.font = titleFont_->currentFont().family().toStdString();
    c.title.size = titleSize_->value();
    c.title.bold = titleBold_->isChecked();
    c.title.color = titleColorValue_.toStdString();
    c.title.x = titleX_->value() / 100;
    c.title.y = titleY_->value() / 100;
    c.title.width = titleWidth_->value() / 100;
    if (pendingTitleId_ == c.id) {
        pendingTitleId_.clear();
        titleTimer_.stop();
    }
    session_.edit([&](Editor &e) { e.title(c.id, c.title); });
}
void MainWindow::flushTitleText() {
    titleTimer_.stop();
    if (pendingTitleId_.empty())
        return;
    auto id = pendingTitleId_;
    auto text = pendingTitleText_;
    pendingTitleId_.clear();
    try {
        auto t = session_.project().clip(id).title;
        t.text = text.toStdString();
        session_.edit([&](Editor &e) { e.title(id, t); });
    } catch (const DomainError &) {
    }
}
void MainWindow::addTitle() {
    flushTitleText();
    if (session_.edit([&](Editor &e) {
            session_.selected = e.addTitle(session_.playhead, tr("Your text").toStdString());
        })) {
        timeline_->fit();
        titleText_->setFocus();
        titleText_->selectAll();
    }
}
} // namespace pms
