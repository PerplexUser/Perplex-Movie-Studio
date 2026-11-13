#include "ui/MainWindow.h"
#include "media/MediaTools.h"
#include "persistence/ProjectStore.h"
#include "ui/ExportDialog.h"
#include "ui/MediaBin.h"
#include "ui/PreviewWidget.h"
#include "ui/Timeline.h"
#include <QApplication>
#include <QAudioOutput>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLibraryInfo>
#include <QLineEdit>
#include <QMediaPlayer>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
#include <QSlider>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>
#include <array>
#include <cmath>

namespace pms {
static QString qs(const std::string &s) {
    return QString::fromStdString(s);
}
MainWindow::MainWindow(bool testMode) : testMode_(testMode) {
    setObjectName("PerplexMovieStudio");
    setMinimumSize(1024, 768);
    resize(1440, 900);
    setAcceptDrops(true);
    QPixmap icon(64, 64);
    icon.fill(Qt::transparent);
    {
        QPainter p(&icon);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(QColor("#83e3bd"));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRect(2, 2, 60, 60), 14, 14);
        p.setBrush(QColor("#172820"));
        p.drawPolygon(QPolygon{{24, 16}, {47, 32}, {24, 48}});
    }
    setWindowIcon(QIcon(icon));
    qputenv("QT_MEDIA_BACKEND", "ffmpeg");
    qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", ",");
    qputenv("QT_DISABLE_HW_TEXTURES_CONVERSION", "1");
    player_ = new QMediaPlayer(this);
    audio_ = new QAudioOutput(this);
    player_->setAudioOutput(audio_);
    audio_->setVolume(0.8);
    buildUi();
    setupCache();
    qApp->installEventFilter(this);
    connect(&session_, &Session::changed, this, [this] {
        invalidatePreview();
        refresh();
    });
    connect(&session_, &Session::selectionChanged, this, [this] {
        flushTitleText();
        updateInspector();
    });
    connect(&session_, &Session::error, this, &MainWindow::message);
    connect(&session_, &Session::positionChanged, this, [this](qint64 t) {
        if (!fromPlayer_ && !previewPath_.isEmpty())
            player_->setPosition(t / 1000);
        updateTime();
    });
    connect(player_, &QMediaPlayer::positionChanged, this, [this](qint64 ms) {
        if (player_->playbackState() != QMediaPlayer::PlayingState)
            return;
        fromPlayer_ = true;
        session_.seek(ms * 1000);
        fromPlayer_ = false;
    });
    connect(player_, &QMediaPlayer::playbackStateChanged, this, [this] {
        play_->setText(player_->playbackState() == QMediaPlayer::PlayingState ? tr("Pause")
                       : previewPath_.isEmpty() ? tr("Prepare and play")
                                                : tr("Play"));
    });
    connect(player_, &QMediaPlayer::mediaStatusChanged, this, [this](auto state) {
        if (previewLoadPending_ &&
            (state == QMediaPlayer::LoadedMedia || state == QMediaPlayer::BufferedMedia)) {
            previewLoadPending_ = false;
            player_->setPosition(session_.playhead / 1000);
            if (wantPlay_) {
                wantPlay_ = false;
                player_->play();
            } else
                player_->pause();
        }
    });
    connect(player_, &QMediaPlayer::errorOccurred, this, [this](auto, const auto &text) {
        if (!testMode_)
            message(tr("The preview could not be played."), text);
    });
    autosave_.setInterval(30000);
    connect(&autosave_, &QTimer::timeout, this, [this] {
        if (session_.editor.dirty() && !testMode_)
            try {
                ProjectStore::save(session_.project(), recoveryPath());
            } catch (...) {
                status_->setText(
                    tr("Automatic backup could not be written. Save your project manually."));
            }
    });
    autosave_.start();
    setLanguage(testMode ? "en-US" : QSettings().value("language", "en-US").toString(), false);
    refresh();
}
MainWindow::~MainWindow() {
    qApp->removeEventFilter(this);
    if (probe_) {
        delete probe_;
        probe_ = nullptr;
    }
    if (previewJob_) {
        delete previewJob_;
        previewJob_ = nullptr;
    }
    player_->stop();
    player_->setSource({});
    delete player_;
    player_ = nullptr;
}
void MainWindow::buildUi() {
    auto label = [&](const char *source, const QString &object = QString()) {
        auto *l = new QLabel;
        l->setObjectName(object);
        translations_.push_back([this, l, source] { l->setText(tr(source)); });
        return l;
    };
    auto button = [&](const char *source, const QString &object = QString()) {
        auto *b = new QPushButton;
        b->setObjectName(object);
        translations_.push_back([this, b, source] { b->setText(tr(source)); });
        return b;
    };
    auto menu = [&](const char *title) {
        auto *m = menuBar()->addMenu(QString());
        translations_.push_back([this, m, title] { m->setTitle(tr(title)); });
        return m;
    };
    auto action = [&](QMenu *m, const char *title, const QKeySequence &shortcut,
                      const std::function<void()> &callback) {
        auto *a = m->addAction(QString());
        translations_.push_back([this, a, title] { a->setText(tr(title)); });
        if (!shortcut.isEmpty())
            a->setShortcut(shortcut);
        connect(a, &QAction::triggered, this, callback);
        return a;
    };
    auto *file = menu(QT_TR_NOOP("File"));
    action(file, QT_TR_NOOP("New project"), QKeySequence::New, [this] { newProject(); });
    action(file, QT_TR_NOOP("Open project…"), QKeySequence::Open, [this] {
        if (!confirmDiscard())
            return;
        auto path = QFileDialog::getOpenFileName(this, tr("Open project"), {},
                                                 tr("Perplex projects (*.pmsproj)"));
        if (!path.isEmpty())
            openProject(path);
    });
    action(file, QT_TR_NOOP("Save"), QKeySequence::Save, [this] { save(); });
    action(file, QT_TR_NOOP("Save as…"), QKeySequence::SaveAs, [this] { save(true); });
    file->addSeparator();
    auto import = [this] {
        importFiles(QFileDialog::getOpenFileNames(this, tr("Import media"), {},
                                                  tr("Media (*.avi *.mpg *.mpeg *.mp4 *.mkv *.mp3 "
                                                     "*.wav *.jpg *.jpeg *.png);;All files (*)")));
    };
    action(file, QT_TR_NOOP("Import media…"), QKeySequence("Ctrl+I"), import);
    action(file, QT_TR_NOOP("Relink missing media…"), {}, [this] { relinkMissing(); });
    file->addSeparator();
    action(file, QT_TR_NOOP("Exit"), QKeySequence::Quit, [this] { close(); });
    auto *edit = menu(QT_TR_NOOP("Edit"));
    undo_ = action(edit, QT_TR_NOOP("Undo"), QKeySequence::Undo,
                   [this] { session_.edit([](Editor &e) { e.undo(); }); });
    redo_ = action(edit, QT_TR_NOOP("Redo"), QKeySequence("Ctrl+Y"),
                   [this] { session_.edit([](Editor &e) { e.redo(); }); });
    edit->addSeparator();
    cut_ = action(edit, QT_TR_NOOP("Cut"), QKeySequence::Cut, [this] {
        copy();
        erase();
    });
    copy_ = action(edit, QT_TR_NOOP("Copy"), QKeySequence::Copy, [this] { copy(); });
    paste_ = action(edit, QT_TR_NOOP("Paste"), QKeySequence::Paste, [this] { paste(); });
    split_ =
        action(edit, QT_TR_NOOP("Split at playhead"), QKeySequence("Ctrl+B"), [this] { split(); });
    duplicate_ = action(edit, QT_TR_NOOP("Duplicate"), QKeySequence("Ctrl+D"), [this] {
        if (!session_.selected.empty())
            session_.edit([&](Editor &e) { session_.selected = e.duplicate(session_.selected); });
    });
    delete_ = action(edit, QT_TR_NOOP("Delete"), QKeySequence::Delete, [this] { erase(); });
    auto *view = menu(QT_TR_NOOP("View"));
    action(view, QT_TR_NOOP("Zoom in"), QKeySequence("Ctrl+="), [this] { timeline_->zoom(1.25); });
    action(view, QT_TR_NOOP("Zoom out"), QKeySequence("Ctrl+-"), [this] { timeline_->zoom(0.8); });
    action(view, QT_TR_NOOP("Fit timeline"), QKeySequence("Shift+Z"), [this] { timeline_->fit(); });
    view->addSeparator();
    auto *settings = menu(QT_TR_NOOP("Settings"));
    auto *language = settings->addMenu(QString());
    translations_.push_back([this, language] { language->setTitle(tr("Language")); });
    auto *en = language->addAction("English");
    auto *de = language->addAction("Deutsch");
    connect(en, &QAction::triggered, this, [this] { setLanguage("en-US"); });
    connect(de, &QAction::triggered, this, [this] { setLanguage("de-DE"); });
    action(settings, QT_TR_NOOP("Cache location…"), {}, [this] {
        auto dir = QFileDialog::getExistingDirectory(this, tr("Choose cache location"));
        if (dir.isEmpty())
            return;
        QSettings().setValue("cacheLocation", dir);
        invalidatePreview();
        setupCache();
    });
    action(settings, QT_TR_NOOP("Clear preview cache"), {}, [this] {
        invalidatePreview();
        setupCache();
        status_->setText(tr("Preview cache cleared."));
    });
    auto *help = menu(QT_TR_NOOP("Help"));
    action(help, QT_TR_NOOP("User guide"), QKeySequence::HelpContents, [this] { userGuide(); });
    action(help, QT_TR_NOOP("About Perplex Movie Studio"), {}, [this] { about(); });

    auto *top = addToolBar("Main");
    top->setMovable(false);
    top->setObjectName("mainBar");
    auto *brand =
        new QLabel("  PERPLEX <span style='color:#aab6c9;font-weight:400'>MOVIE STUDIO</span>  ");
    brand->setObjectName("brand");
    top->addWidget(brand);
    top->addSeparator();
    projectTitle_ = new QLabel;
    projectTitle_->setObjectName("projectTitle");
    top->addWidget(projectTitle_);
    auto *spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    top->addWidget(spacer);
    auto *importTop = button(QT_TR_NOOP("+ Import media"));
    top->addWidget(importTop);
    connect(importTop, &QPushButton::clicked, this, import);
    export_ = button(QT_TR_NOOP("Export movie"), "primaryButton");
    auto *textButton = button(QT_TR_NOOP("+ Text"), "addTitleButton");
    connect(textButton, &QPushButton::clicked, this, &MainWindow::addTitle);
    top->addWidget(textButton);
    projectFormat_ = new QComboBox;
    projectFormat_->setObjectName("projectFormatBox");
    projectFormat_->addItems({QString(), QString()});
    translations_.push_back([this] {
        projectFormat_->setItemText(0, tr("Landscape 16:9"));
        projectFormat_->setItemText(1, tr("Portrait 9:16"));
    });
    top->addWidget(projectFormat_);
    connect(projectFormat_, &QComboBox::activated, this, [this](int i) {
        auto x = session_.project().settings;
        x.width = i ? 1080 : 1920;
        x.height = i ? 1920 : 1080;
        session_.edit([&](Editor &e) { e.settings(x); });
    });
    top->addWidget(export_);
    connect(export_, &QPushButton::clicked, this, [this] {
        flushTitleText();
        if (!session_.project().clips.empty())
            (new ExportDialog(session_.project(), this))->show();
    });
    auto dock = [&](const char *title, const QString &name, QWidget *content,
                    Qt::DockWidgetArea area) {
        auto *d = new QDockWidget(this);
        d->setObjectName(name);
        d->setWidget(content);
        d->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable);
        translations_.push_back([this, d, title] { d->setWindowTitle(tr(title)); });
        addDockWidget(area, d);
        view->addAction(d->toggleViewAction());
        return d;
    };
    auto *library = new QWidget;
    auto *ll = new QVBoxLayout(library);
    ll->setContentsMargins(12, 14, 12, 12);
    auto *libraryTitle = label(QT_TR_NOOP("Project media"), "sectionTitle");
    ll->addWidget(libraryTitle);
    auto *importButton = button(QT_TR_NOOP("+ Import files"));
    connect(importButton, &QPushButton::clicked, this, import);
    ll->addWidget(importButton);
    bin_ = new MediaBin(&session_);
    ll->addWidget(bin_, 1);
    libraryHint_ =
        label(QT_TR_NOOP("Drop media here.\nDrag a file onto the timeline to begin."), "mutedText");
    libraryHint_->setWordWrap(true);
    ll->addWidget(libraryHint_);
    removeMedia_ = button(QT_TR_NOOP("Remove from project"), "removeMediaButton");
    ll->addWidget(removeMedia_);
    connect(removeMedia_, &QPushButton::clicked, bin_, &MediaBin::removeSelected);
    connect(bin_, &MediaBin::currentItemChanged, this,
            [this] { removeMedia_->setEnabled(bin_->currentItem() != nullptr); });
    connect(bin_, &MediaBin::removeRequested, this, [this](const QString &id) {
        session_.edit([&](Editor &e) { e.removeAsset(id.toStdString()); });
    });
    removeMedia_->setEnabled(false);
    library->setMinimumWidth(235);
    auto *ld = dock(QT_TR_NOOP("MEDIA"), "mediaDock", library, Qt::LeftDockWidgetArea);
    connect(bin_, &MediaBin::filesDropped, this, &MainWindow::importFiles);
    connect(bin_, &MediaBin::addRequested, this, &MainWindow::append);

    auto *center = new QWidget;
    auto *centerLayout = new QVBoxLayout(center);
    centerLayout->setContentsMargins(18, 14, 18, 12);
    auto *previewTitle = label(QT_TR_NOOP("PROGRAM MONITOR"), "mutedText");
    centerLayout->addWidget(previewTitle);
    previewStack_ = new QStackedWidget;
    previewStack_->setObjectName("previewSurface");
    auto *welcome = new QWidget;
    auto *wl = new QVBoxLayout(welcome);
    wl->addStretch();
    auto *playmark = new QLabel("▷");
    playmark->setAlignment(Qt::AlignCenter);
    playmark->setObjectName("playMark");
    wl->addWidget(playmark);
    auto *welcomeTitle = label(QT_TR_NOOP("Every story starts with a cut."), "welcomeTitle");
    welcomeTitle->setAlignment(Qt::AlignCenter);
    welcomeTitle->setWordWrap(true);
    wl->addWidget(welcomeTitle);
    auto *subtitle = label(QT_TR_NOOP("Import your footage. Make it your own."), "mutedText");
    subtitle->setAlignment(Qt::AlignCenter);
    wl->addWidget(subtitle);
    wl->addStretch();
    previewStack_->addWidget(welcome);
    video_ = new PreviewWidget;
    previewStack_->addWidget(video_);
    player_->setVideoSink(video_->videoSink());
    centerLayout->addWidget(previewStack_, 1);
    previewNote_ = new QLabel;
    previewNote_->setWordWrap(true);
    previewNote_->setObjectName("mutedText");
    centerLayout->addWidget(previewNote_);
    previewProgress_ = new QProgressBar;
    previewProgress_->setRange(0, 1000);
    previewProgress_->setFormat("%p%");
    previewProgress_->hide();
    centerLayout->addWidget(previewProgress_);
    cancelPreview_ = button(QT_TR_NOOP("Cancel preview preparation"));
    cancelPreview_->hide();
    centerLayout->addWidget(cancelPreview_);
    connect(cancelPreview_, &QPushButton::clicked, this, [this] {
        if (previewJob_)
            previewJob_->cancel();
    });
    seek_ = new QSlider(Qt::Horizontal);
    seek_->setAccessibleName(tr("Playback position"));
    centerLayout->addWidget(seek_);
    connect(seek_, &QSlider::sliderMoved, this,
            [this](int ms) { session_.seek(qint64(ms) * 1000); });
    auto *controls = new QHBoxLayout;
    auto *previous = button(QT_TR_NOOP("Previous frame"));
    auto *next = button(QT_TR_NOOP("Next frame"), "nextFrameButton");
    play_ = button(QT_TR_NOOP("Prepare and play"));
    play_->setObjectName("playButton");
    connect(previous, &QPushButton::clicked, this, [this] { step(-1); });
    connect(next, &QPushButton::clicked, this, [this] { step(1); });
    connect(play_, &QPushButton::clicked, this, &MainWindow::togglePlay);
    controls->addWidget(previous);
    controls->addWidget(play_);
    controls->addWidget(next);
    controls->addStretch();
    auto *volume = new QSlider(Qt::Horizontal);
    volume->setRange(0, 100);
    volume->setValue(80);
    volume->setMaximumWidth(100);
    volume->setAccessibleName(tr("Preview volume"));
    controls->addWidget(volume);
    connect(volume, &QSlider::valueChanged, this,
            [this](int v) { audio_->setVolume(float(v) / 100); });
    centerLayout->addLayout(controls);
    time_ = new QLabel;
    time_->setObjectName("timecode");
    time_->setAlignment(Qt::AlignCenter);
    centerLayout->addWidget(time_);
    setCentralWidget(center);

    auto *inspector = new QWidget;
    auto *il = new QVBoxLayout(inspector);
    il->setContentsMargins(16, 14, 16, 14);
    inspectorTitle_ = new QLabel;
    inspectorTitle_->setObjectName("sectionTitle");
    inspectorTitle_->setWordWrap(true);
    il->addWidget(inspectorTitle_);
    assetInfo_ = new QLabel;
    assetInfo_->setWordWrap(true);
    assetInfo_->setObjectName("mutedText");
    il->addWidget(assetInfo_);
    inspectorFields_ = new QWidget;
    auto *form = new QFormLayout(inspectorFields_);
    form->setContentsMargins(0, 14, 0, 0);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    auto number = [&](const char *title, double max, int decimals = 3) {
        auto *spin = new QDoubleSpinBox;
        spin->setDecimals(decimals);
        spin->setRange(0, max);
        spin->setSingleStep(decimals ? 0.1 : 5);
        spin->setKeyboardTracking(false);
        form->addRow(label(title), spin);
        return spin;
    };
    track_ = new QComboBox;
    form->addRow(label(QT_TR_NOOP("Track")), track_);
    start_ = number(QT_TR_NOOP("Start (s)"), 86400, 6);
    in_ = number(QT_TR_NOOP("Source in (s)"), 86400, 6);
    out_ = number(QT_TR_NOOP("Source out (s)"), 86400, 6);
    duration_ = number(QT_TR_NOOP("Duration (s)"), 86400, 3);
    duration_->setObjectName("clipDuration");
    connect(duration_, &QDoubleSpinBox::editingFinished, this, [this] {
        if (updating_ || session_.selected.empty())
            return;
        auto c = session_.project().clip(session_.selected);
        session_.edit([&](Editor &e) {
            e.trim(c.id, c.sourceIn, c.sourceIn + Time(std::llround(duration_->value() * Second)),
                   c.start);
        });
        updateInspector();
    });
    gain_ = number(QT_TR_NOOP("Volume (%)"), 400, 0);
    fadeIn_ = number(QT_TR_NOOP("Audio fade in (s)"), 86400);
    fadeOut_ = number(QT_TR_NOOP("Audio fade out (s)"), 86400);
    muted_ = new QCheckBox;
    form->addRow(label(QT_TR_NOOP("Mute audio")), muted_);
    il->addWidget(inspectorFields_);
    buildFeatureControls(il);
    il->addStretch();
    inspector->setMinimumWidth(340);
    auto *scroll = new QScrollArea;
    scroll->setWidget(inspector);
    scroll->setWidgetResizable(true);
    auto *rd = dock(QT_TR_NOOP("INSPECTOR"), "inspectorDock", scroll, Qt::RightDockWidgetArea);
    auto timing = [this] {
        if (updating_ || session_.selected.empty())
            return;
        auto c = session_.project().clip(session_.selected);
        session_.edit([&](Editor &e) {
            e.trim(c.id, Time(std::llround(in_->value() * Second)),
                   Time(std::llround(out_->value() * Second)),
                   Time(std::llround(start_->value() * Second)));
        });
        updateInspector();
    };
    for (auto *box : {start_, in_, out_})
        connect(box, &QDoubleSpinBox::editingFinished, this, timing);
    connect(track_, &QComboBox::activated, this, [this] {
        if (updating_ || session_.selected.empty())
            return;
        session_.edit([&](Editor &e) {
            e.move(session_.selected, track_->currentData().toString().toStdString(),
                   session_.project().clip(session_.selected).start);
        });
        updateInspector();
    });
    auto audioChange = [this] {
        if (updating_ || session_.selected.empty())
            return;
        session_.edit([&](Editor &e) {
            e.audio(session_.selected, gain_->value() / 100, muted_->isChecked(),
                    Time(std::llround(fadeIn_->value() * Second)),
                    Time(std::llround(fadeOut_->value() * Second)));
        });
        updateInspector();
    };
    for (auto *box : {gain_, fadeIn_, fadeOut_})
        connect(box, &QDoubleSpinBox::editingFinished, this, audioChange);
    connect(muted_, &QCheckBox::clicked, this, audioChange);

    auto *timelinePanel = new QWidget;
    auto *tl = new QVBoxLayout(timelinePanel);
    tl->setContentsMargins(0, 0, 0, 0);
    tl->setSpacing(0);
    auto *tools = new QToolBar;
    tools->setObjectName("timelineBar");
    tools->addAction(undo_);
    tools->addAction(redo_);
    tools->addSeparator();
    tools->addAction(split_);
    tools->addAction(delete_);
    tools->addSeparator();
    auto *snap = new QCheckBox;
    translations_.push_back([this, snap] { snap->setText(tr("Snap")); });
    snap->setChecked(true);
    tools->addWidget(snap);
    auto *gap = new QWidget;
    gap->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tools->addWidget(gap);
    auto *minus = button(QT_TR_NOOP("−"));
    auto *plus = button(QT_TR_NOOP("+"));
    auto *fit = button(QT_TR_NOOP("Fit"));
    tools->addWidget(minus);
    tools->addWidget(plus);
    tools->addWidget(fit);
    tl->addWidget(tools);
    timeline_ = new Timeline(&session_);
    tl->addWidget(timeline_);
    connect(snap, &QCheckBox::toggled, timeline_, &Timeline::setSnapping);
    connect(minus, &QPushButton::clicked, this, [this] { timeline_->zoom(0.8); });
    connect(plus, &QPushButton::clicked, this, [this] { timeline_->zoom(1.25); });
    connect(fit, &QPushButton::clicked, timeline_, &Timeline::fit);
    connect(timeline_, &Timeline::filesDropped, this, &MainWindow::importFiles);
    auto *td =
        dock(QT_TR_NOOP("TIMELINE"), "timelineDock", timelinePanel, Qt::BottomDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
    resizeDocks({ld, rd}, {260, 355}, Qt::Horizontal);
    resizeDocks({td}, {390}, Qt::Vertical);
    auto *copyright = new QLabel("© by www.perplex.click");
    statusBar()->addWidget(copyright);
    status_ = new QLabel;
    statusBar()->addPermanentWidget(status_);
}
void MainWindow::refresh() {
    {
        QSignalBlocker b(projectFormat_);
        projectFormat_->setCurrentIndex(
            session_.project().settings.height > session_.project().settings.width ? 1 : 0);
    }
    auto title = session_.project().name == "Untitled" ? tr("Untitled project")
                                                       : qs(session_.project().name);
    projectTitle_->setText("  " + title + (session_.editor.dirty() ? "  •" : ""));
    setWindowTitle(title + (session_.editor.dirty() ? " *" : "") + " — Perplex Movie Studio");
    undo_->setEnabled(session_.editor.canUndo());
    redo_->setEnabled(session_.editor.canRedo());
    paste_->setEnabled(clipboard_.has_value());
    bool hasClips = !session_.project().clips.empty();
    export_->setEnabled(hasClips);
    play_->setEnabled(hasClips && !previewJob_);
    seek_->setRange(0, int(session_.project().duration() / 1000));
    status_->setText(tr("%1 media · %2 clips · %3 fps")
                         .arg(session_.project().assets.size())
                         .arg(session_.project().clips.size())
                         .arg(session_.project().settings.fpsNum));
    libraryHint_->setVisible(session_.project().assets.empty());
    updateInspector();
    updateTime();
}
void MainWindow::updateInspector() {
    updating_ = true;
    updateFeatureInspector();
    bool has = !session_.selected.empty();
    bool locked =
        has && session_.project().track(session_.project().clip(session_.selected).trackId).locked;
    for (auto *a : {split_, delete_, duplicate_, cut_})
        a->setEnabled(has && !locked);
    copy_->setEnabled(has);
    inspectorFields_->setEnabled(has && !locked);
    if (!has) {
        inspectorTitle_->setText(tr("Clip properties"));
        assetInfo_->setText(tr("Select a clip in the timeline to adjust timing and audio."));
        updating_ = false;
        return;
    }
    const auto &c = session_.project().clip(session_.selected);
    const auto &a = session_.project().asset(c.assetId);
    const auto &t = session_.project().track(c.trackId);
    inspectorTitle_->setText(
        a.kind == AssetKind::Title ? qs(c.title.text).section('\n', 0, 0).left(80) : qs(a.name));
    assetInfo_->setText(qs(a.container) + " · " + qs(a.videoCodec) + " / " + qs(a.audioCodec) +
                        "\n" + timecode(c.duration()) + (locked ? "\n" + tr("Track locked") : ""));
    track_->clear();
    for (const auto &track : session_.project().tracks)
        if (track.type == t.type)
            track_->addItem(qs(track.name), qs(track.id));
    track_->setCurrentIndex(track_->findData(qs(t.id)));
    start_->setValue(double(c.start) / Second);
    in_->setValue(double(c.sourceIn) / Second);
    out_->setValue(double(c.sourceOut) / Second);
    duration_->setValue(double(c.duration()) / Second);
    gain_->setValue(c.gain * 100);
    fadeIn_->setMaximum(double(c.envelopeOut - c.envelopeIn) / Second);
    fadeOut_->setMaximum(double(c.envelopeOut - c.envelopeIn) / Second);
    fadeIn_->setValue(double(c.fadeIn) / Second);
    fadeOut_->setValue(double(c.fadeOut) / Second);
    muted_->setChecked(c.muted);
    for (QWidget *w : std::array<QWidget *, 4>{gain_, fadeIn_, fadeOut_, muted_})
        w->setEnabled(!locked && a.audioStream >= 0);
    updating_ = false;
}
void MainWindow::updateTime() {
    auto s = session_.project().settings;
    time_->setText(timecode(session_.playhead, s.fpsNum, s.fpsDen) + "  /  " +
                   timecode(session_.project().duration(), s.fpsNum, s.fpsDen));
    if (!seek_->isSliderDown()) {
        QSignalBlocker block(seek_);
        seek_->setValue(int(session_.playhead / 1000));
    }
}
void MainWindow::setLanguage(const QString &locale, bool persist) {
    qApp->removeTranslator(&translator_);
    qApp->removeTranslator(&qtTranslator_);
    if (locale == "de-DE") {
        if (translator_.load(":/i18n/perplex_de_DE.qm") ||
            translator_.load("perplex_de_DE", QCoreApplication::applicationDirPath()))
            qApp->installTranslator(&translator_);
        if (qtTranslator_.load("qtbase_de", QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
            qApp->installTranslator(&qtTranslator_);
    }
    if (persist)
        QSettings().setValue("language", locale);
    for (const auto &fn : translations_)
        fn();
    bin_->refresh();
    timeline_->viewport()->update();
    refresh();
    previewNote_->setText(
        previewPath_.isEmpty()
            ? tr("Prepare the preview after editing. Preview and export use the same composition.")
            : tr("Preview ready · 720p"));
    play_->setText(player_->playbackState() == QMediaPlayer::PlayingState ? tr("Pause")
                   : previewPath_.isEmpty()                               ? tr("Prepare and play")
                                                                          : tr("Play"));
}
void MainWindow::setupCache() {
    auto root =
        QSettings()
            .value("cacheLocation", QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
            .toString();
    QDir().mkpath(root);
    cache_ = std::make_unique<QTemporaryDir>(root + "/session-XXXXXX");
    if (!cache_->isValid())
        status_->setText(
            tr("The cache folder is not writable. Choose another folder in Settings."));
}
void MainWindow::invalidatePreview() {
    previewLoadPending_ = false;
    wantPlay_ = false;
    player_->stop();
    player_->setSource({});
    if (session_.project().clips.empty())
        video_->clear();
    previewStack_->setCurrentIndex(video_->hasFrame() ? 1 : 0);
    if (previewJob_)
        previewJob_->cancel();
    if (!previewPath_.isEmpty())
        QFile::remove(previewPath_);
    previewPath_.clear();
    previewNote_->setText(
        tr("Prepare the preview after editing. Preview and export use the same composition."));
    play_->setText(tr("Prepare and play"));
}
void MainWindow::startPreview(bool play) {
    if (previewJob_ || session_.project().clips.empty() || !cache_ || !cache_->isValid())
        return;
    const auto revision = session_.revision;
    wantPlay_ = play;
    previewRevision_ = revision;
    auto path = cache_->path() + "/preview-" + QString::number(revision) + ".mp4";
    RenderProfile profile;
    profile.width = 1280;
    profile.height = 720;
    if (session_.project().settings.height > session_.project().settings.width)
        std::swap(profile.width, profile.height);
    profile.fps = session_.project().settings.fpsNum;
    profile.quality = "low";
    previewJob_ = new RenderJob(this);
    previewProgress_->setValue(0);
    previewProgress_->show();
    cancelPreview_->show();
    play_->setEnabled(false);
    previewNote_->setText(tr("Preparing preview… Editing remains available."));
    auto done = [this] {
        previewJob_->deleteLater();
        previewJob_ = nullptr;
        previewProgress_->hide();
        cancelPreview_->hide();
        play_->setEnabled(!session_.project().clips.empty());
    };
    connect(previewJob_, &RenderJob::progress, this,
            [this](double p) { previewProgress_->setValue(int(p * 1000)); });
    connect(previewJob_, &RenderJob::succeeded, this,
            [this, revision, done](const QString &path, const QString &) {
                done();
                if (revision != session_.revision) {
                    QFile::remove(path);
                    return;
                }
                previewPath_ = path;
                previewStack_->setCurrentIndex(1);
                previewNote_->setText(tr("Preview ready · 720p"));
                play_->setText(tr("Play"));
                previewLoadPending_ = true;
                player_->setSource(QUrl::fromLocalFile(path));
            });
    connect(previewJob_, &RenderJob::failed, this,
            [this, done](const QString &text, const QString &detail) {
                done();
                previewNote_->setText(text);
                message(text, detail);
            });
    connect(previewJob_, &RenderJob::cancelled, this, [this, done] {
        done();
        previewNote_->setText(tr("Preview preparation cancelled."));
    });
    previewJob_->start(session_.project(), profile, path, true);
}
void MainWindow::togglePlay() {
    if (player_->playbackState() == QMediaPlayer::PlayingState) {
        player_->pause();
        return;
    }
    if (session_.project().clips.empty())
        return;
    if (session_.playhead >=
        session_.project().duration() - frameTime(1, session_.project().settings))
        session_.seek(0);
    if (previewPath_.isEmpty())
        startPreview(true);
    else
        player_->play();
}
void MainWindow::step(int direction) {
    player_->pause();
    auto s = session_.project().settings;
    session_.seek(
        frameTime(std::max<std::int64_t>(0, frameIndex(session_.playhead, s) + direction), s));
    if (previewPath_.isEmpty())
        startPreview(false);
}
void MainWindow::split() {
    if (!session_.selected.empty())
        session_.edit(
            [&](Editor &e) { session_.selected = e.split(session_.selected, session_.playhead); });
}
void MainWindow::erase() {
    if (!session_.selected.empty())
        session_.edit([&](Editor &e) { e.erase(session_.selected); });
}
void MainWindow::copy() {
    if (session_.selected.empty())
        return;
    clipboard_ = session_.project().clip(session_.selected);
    paste_->setEnabled(true);
}
void MainWindow::paste() {
    if (!clipboard_)
        return;
    std::string target = clipboard_->trackId;
    if (!session_.selected.empty())
        target = session_.project().clip(session_.selected).trackId;
    session_.edit(
        [&](Editor &e) { session_.selected = e.paste(*clipboard_, target, session_.playhead); });
}
void MainWindow::append(const QString &id) {
    const auto &a = session_.project().asset(id.toStdString());
    auto type = a.videoStream >= 0 ? TrackType::Video : TrackType::Audio;
    const Track *target = nullptr;
    for (const auto &t : session_.project().tracks)
        if (t.type == type && !t.locked && (!target || t.order < target->order))
            target = &t;
    if (!target) {
        message(tr("Unlock a suitable track before adding media."));
        return;
    }
    Time start = 0;
    for (const auto &c : session_.project().clips)
        if (c.trackId == target->id)
            start = std::max(start, c.end());
    auto trackId = target->id;
    session_.edit(
        [&](Editor &e) { session_.selected = e.addClip(id.toStdString(), trackId, start); });
}
void MainWindow::importFiles(const QStringList &files) {
    for (const auto &path : files)
        if (!path.isEmpty())
            imports_.enqueue(path);
    nextImport();
}
void MainWindow::nextImport() {
    if (probe_ || imports_.isEmpty())
        return;
    auto path = imports_.dequeue();
    probe_ = new ProbeJob(this);
    status_->setText(tr("Importing %1…").arg(QFileInfo(path).fileName()));
    connect(probe_, &ProbeJob::succeeded, this, [this](const Asset &a) {
        session_.edit([&](Editor &e) { e.addAsset(a); });
        probe_->deleteLater();
        probe_ = nullptr;
        nextImport();
    });
    connect(probe_, &ProbeJob::failed, this, [this, path](const auto &text, const auto &detail) {
        probe_->deleteLater();
        probe_ = nullptr;
        message(QFileInfo(path).fileName() + "\n" + text, detail);
        nextImport();
    });
    probe_->start(path);
}
bool MainWindow::save(bool saveAs) {
    flushTitleText();
    QString path = session_.path;
    if (saveAs || path.isEmpty())
        path = QFileDialog::getSaveFileName(this, tr("Save project"),
                                            path.isEmpty() ? tr("Untitled.pmsproj") : path,
                                            tr("Perplex projects (*.pmsproj)"));
    if (path.isEmpty())
        return false;
    if (!path.endsWith(".pmsproj", Qt::CaseInsensitive))
        path += ".pmsproj";
    try {
        ProjectStore::save(session_.project(), path);
        session_.path = path;
        session_.editor.markSaved();
        if (!testMode_)
            QFile::remove(recoveryPath());
        refresh();
        return true;
    } catch (const DomainError &e) {
        message(errorText(int(e.code)));
    } catch (const std::exception &e) {
        message(tr("The project could not be saved. The previous file was preserved."),
                QString::fromUtf8(e.what()));
    }
    return false;
}
bool MainWindow::confirmDiscard() {
    flushTitleText();
    if (!session_.editor.dirty())
        return true;
    auto answer = QMessageBox::question(
        this, tr("Save changes?"), tr("Save your changes before continuing?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Cancel)
        return false;
    return answer == QMessageBox::Discard || save();
}
bool MainWindow::openProject(const QString &path) {
    try {
        auto project = ProjectStore::load(path);
        imports_.clear();
        if (probe_) {
            delete probe_;
            probe_ = nullptr;
        }
        clipboard_.reset();
        session_.editor.reset(std::move(project));
        session_.path = path;
        session_.selected.clear();
        session_.playhead = 0;
        session_.changedProject();
        timeline_->fit();
        if (!testMode_ && !ProjectStore::missing(session_.project()).isEmpty())
            relinkMissing();
        return true;
    } catch (const DomainError &e) {
        message(errorText(int(e.code)));
    } catch (const std::exception &e) {
        message(tr("The project could not be opened. Your current project has been preserved."),
                QString::fromUtf8(e.what()));
    }
    return false;
}
void MainWindow::newProject() {
    if (!confirmDiscard())
        return;
    bool ok;
    auto name = QInputDialog::getText(this, tr("New project"), tr("Project name"),
                                      QLineEdit::Normal, tr("Untitled project"), &ok);
    if (!ok)
        return;
    imports_.clear();
    if (probe_) {
        delete probe_;
        probe_ = nullptr;
    }
    auto p = makeProject();
    p.name = name.trimmed().isEmpty() ? "Untitled" : name.trimmed().toStdString();
    session_.editor.reset(p);
    session_.path.clear();
    session_.selected.clear();
    session_.playhead = 0;
    clipboard_.reset();
    session_.changedProject();
    if (!testMode_)
        QFile::remove(recoveryPath());
}
void MainWindow::relinkMissing() {
    if (probe_) {
        message(tr("Wait for the current media import to finish."));
        return;
    }
    auto missing = ProjectStore::missing(session_.project());
    if (missing.isEmpty()) {
        message(tr("All source files are available."));
        return;
    }
    const auto id = missing.first().toStdString();
    const auto &old = session_.project().asset(id);
    auto path = QFileDialog::getOpenFileName(
        this, tr("Locate %1").arg(qs(old.name)), {},
        tr("Media (*.avi *.mpg *.mpeg *.mp4 *.mkv *.mp3 *.wav *.jpg *.jpeg *.png);;All files (*)"));
    if (path.isEmpty())
        return;
    probe_ = new ProbeJob(this);
    connect(probe_, &ProbeJob::succeeded, this, [this, id](const Asset &a) {
        probe_->deleteLater();
        probe_ = nullptr;
        bool ok = session_.edit([&](Editor &e) { e.relink(id, a); });
        if (ok && !ProjectStore::missing(session_.project()).isEmpty())
            relinkMissing();
    });
    connect(probe_, &ProbeJob::failed, this, [this](const auto &msg, const auto &detail) {
        probe_->deleteLater();
        probe_ = nullptr;
        message(msg, detail);
    });
    probe_->start(path);
}
void MainWindow::message(const QString &text, const QString &detail) {
    if (testMode_) {
        qWarning().noquote() << text << detail;
        return;
    }
    QMessageBox box(QMessageBox::Information, tr("Perplex Movie Studio"), text, QMessageBox::Ok,
                    this);
    if (!detail.isEmpty())
        box.setDetailedText(detail);
    box.exec();
}
QString MainWindow::recoveryPath() const {
    auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/recovery.pmsproj";
}
void MainWindow::recover() {
    if (testMode_ || !QFileInfo::exists(recoveryPath()))
        return;
    if (QMessageBox::question(this, tr("Recover project?"),
                              tr("An automatic backup was found. Open it to recover unsaved work?"),
                              QMessageBox::Yes | QMessageBox::No,
                              QMessageBox::Yes) == QMessageBox::Yes) {
        if (openProject(recoveryPath())) {
            session_.path.clear();
            session_.editor.apply([](Project &p) { p.name += " (recovered)"; });
            session_.changedProject();
        }
    } else
        QFile::remove(recoveryPath());
}
QString MainWindow::aboutText() const {
    return tr("Perplex Movie Studio\nVersion %1\n\n© by www.perplex.click\nMichael "
              "Lappenbusch\n\nOriginal application code: MIT License.\nQt: © The Qt Company Ltd. "
              "and contributors, LGPL v3.\nFFmpeg: LGPL v2.1 or later in the supplied "
              "build.\nLAME: LGPL v2 or later.\n\nSee LICENSE and THIRD_PARTY_NOTICES.md beside "
              "the application.\nDevelopment build: Windows acceptance and performance tests "
              "remain open.")
        .arg(PMS_VERSION);
}
void MainWindow::about() {
    message(aboutText());
}
void MainWindow::userGuide() {
    QDialog dialog(this);
    dialog.setWindowTitle(tr("User guide"));
    dialog.resize(680, 540);
    auto *l = new QVBoxLayout(&dialog);
    auto *text = new QTextEdit;
    text->setReadOnly(true);
    text->setPlainText(tr(
        "QUICK START\n\nImport video, audio, JPG or PNG. Double-click to append, or drag to a "
        "track. Remove unused media with Remove from project; source files stay on disk.\n\nChoose "
        "Landscape 16:9 or Portrait 9:16 above the preview. Phone rotation metadata is applied "
        "automatically. Drag clips to move, their edges to trim; Ctrl+B splits at the playhead. "
        "Clip settings include start, duration, source range, volume and audio fades.\n\nIn "
        "Picture, adjust brightness, contrast, saturation and hue. Choose Fade, Wipe or Slide and "
        "its in/out duration. For a transition between clips, overlap them on different video "
        "tracks. Without a lower clip the background is black.\n\nClick + Text. Set wording, "
        "display duration, font, size, color and position. Click or drag in the prepared preview "
        "to place text. Picture transitions also apply to text and images. T1 is above V2, V2 "
        "above V1. M mutes track audio; L locks editing.\n\nPrepare and play builds a local 720p "
        "preview using the export composition. After editing, prepare again. Next frame also "
        "prepares a still preview. Use the ruler or slider to seek.\n\nSave a .pmsproj project. "
        "Older projects can be opened; saving writes schema 2 with visual settings. Older "
        "application versions cannot open schema 2. Source files are never modified.\n\nExport "
        "MP4/MKV with software MPEG-4 Part 2 and AAC, or MP3/WAV audio. Choose landscape or "
        "portrait at 720p or 1080p. The entire timeline including gaps is exported. Existing "
        "destinations are replaced only on success.\n\nSHORTCUTS\nCtrl+N/O/S: New/Open/Save · "
        "Ctrl+Shift+S: Save as\nCtrl+Z/Y: Undo/Redo · Ctrl+B: Split · Ctrl+D: "
        "Duplicate\nCtrl+X/C/V: Cut/Copy/Paste · Delete: Delete\nSpace: Play/Pause · Left/Right: "
        "Previous/Next frame\nCtrl+wheel: Zoom · Shift+wheel: Scroll\n\nDEVELOPMENT BUILD\nWindows "
        "acceptance and performance tests remain open. The software pixel path prioritizes correct "
        "colors over speed. No accounts, telemetry or uploads. Full documentation is in docs."));
    l->addWidget(text);
    auto *b = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(b, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    l->addWidget(b);
    dialog.exec();
}
void MainWindow::closeEvent(QCloseEvent *e) {
    if (!confirmDiscard()) {
        e->ignore();
        return;
    }
    for (auto *dialog : findChildren<ExportDialog *>())
        if (!dialog->close()) {
            e->ignore();
            return;
        }
    if (!testMode_)
        QFile::remove(recoveryPath());
    e->accept();
}
void MainWindow::dragEnterEvent(QDragEnterEvent *e) {
    if (e->mimeData()->hasUrls())
        e->acceptProposedAction();
}
void MainWindow::dropEvent(QDropEvent *e) {
    QStringList paths;
    for (auto url : e->mimeData()->urls())
        if (url.isLocalFile())
            paths << url.toLocalFile();
    importFiles(paths);
    e->acceptProposedAction();
}
bool MainWindow::eventFilter(QObject *, QEvent *event) {
    if (!isActiveWindow())
        return false;
    auto *focus = QApplication::focusWidget();
    bool textInput = qobject_cast<QLineEdit *>(focus) || qobject_cast<QTextEdit *>(focus) ||
                     qobject_cast<QPlainTextEdit *>(focus) ||
                     qobject_cast<QPlainTextEdit *>(focus) ||
                     qobject_cast<QAbstractSpinBox *>(focus) || qobject_cast<QComboBox *>(focus);
    if ((focus == bin_ || focus == bin_->viewport()) &&
        (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress)) {
        auto *key = static_cast<QKeyEvent *>(event);
        if (key->key() == Qt::Key_Delete && key->modifiers() == Qt::NoModifier) {
            if (event->type() == QEvent::KeyPress)
                bin_->removeSelected();
            key->accept();
            return true;
        }
    }
    if (event->type() == QEvent::ShortcutOverride && textInput) {
        auto *e = static_cast<QKeyEvent *>(event);
        if (e->key() == Qt::Key_Space || e->key() == Qt::Key_Delete || e->key() == Qt::Key_Left ||
            e->key() == Qt::Key_Right ||
            ((e->modifiers() & Qt::ControlModifier) &&
             QString("ACVXZ YBD").contains(QChar(e->key())))) {
            e->accept();
            return true;
        }
    }
    if (event->type() == QEvent::KeyPress && !textInput &&
        !qobject_cast<QAbstractButton *>(focus)) {
        auto *e = static_cast<QKeyEvent *>(event);
        if (e->modifiers() == Qt::NoModifier) {
            if (e->key() == Qt::Key_Space) {
                togglePlay();
                return true;
            }
            if (e->key() == Qt::Key_Left) {
                step(-1);
                return true;
            }
            if (e->key() == Qt::Key_Right) {
                step(1);
                return true;
            }
        }
    }
    return false;
}
bool MainWindow::smokeTest() {
    if (windowTitle().isEmpty() || !findChild<Timeline *>() || !findChild<MediaBin *>())
        return false;
    setLanguage("de-DE", false);
    bool translated = menuBar()->actions().first()->text() == "Datei";
    setLanguage("en-US", false);
    if (!translated || menuBar()->actions().first()->text() != "File" ||
        session_.project().tracks.size() != 4)
        return false;
    bool copyright = false;
    for (const auto *l : findChildren<QLabel *>())
        if (l->text() == QString::fromUtf8("© by www.perplex.click"))
            copyright = true;
    if (!copyright)
        return false;
    Asset a;
    a.path = "/synthetic/ui-smoke.mp4";
    a.name = "UI smoke";
    a.duration = 10 * Second;
    a.videoStream = 0;
    a.audioStream = 1;
    a.width = 320;
    a.height = 180;
    session_.edit([&](Editor &e) {
        auto id = e.addAsset(a);
        session_.selected = e.addClip(id, e.project().tracks[0].id, 0);
    });
    if (bin_->count() != 1 || !split_->isEnabled())
        return false;
    session_.seek(4 * Second);
    split_->trigger();
    if (session_.project().clips.size() != 2 ||
        session_.project().clips[0].duration() != 4 * Second)
        return false;
    undo_->trigger();
    if (session_.project().clips.size() != 1)
        return false;
    redo_->trigger();
    if (session_.project().clips.size() != 2)
        return false;
    session_.select(session_.project().clips[0].id);
    gain_->setValue(50);
    QMetaObject::invokeMethod(gain_, "editingFinished");
    if (session_.project().clip(session_.selected).gain != 0.5)
        return false;
    session_.edit([&](Editor &e) {
        const auto id = e.project().clips[0].trackId;
        e.trackState(id, false, true);
    });
    return !split_->isEnabled() && !inspectorFields_->isEnabled();
}
} // namespace pms
