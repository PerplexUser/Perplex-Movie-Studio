#pragma once
#include "app/Session.h"
#include "export/RenderJob.h"
#include "media/ProbeJob.h"
#include <QMainWindow>
#include <QQueue>
#include <QTemporaryDir>
#include <QTimer>
#include <QTranslator>
class QMediaPlayer;
class QAudioOutput;
class QFontComboBox;
class QPlainTextEdit;
class QVBoxLayout;
class QGroupBox;
class QTabWidget;
class QLabel;
class QPushButton;
class QSlider;
class QDoubleSpinBox;
class QCheckBox;
class QComboBox;
class QStackedWidget;
class QProgressBar;
namespace pms {
class PreviewWidget;
class Timeline;
class MediaBin;
class MainWindow final : public QMainWindow {
    Q_OBJECT
  public:
    explicit MainWindow(bool testMode = false);
    ~MainWindow() override;
    Session &session() {
        return session_;
    }
    bool openProject(const QString &path);
    void importFiles(const QStringList &);
    void setLanguage(const QString &locale, bool persist = true);
    void recover();
    bool smokeTest();
    QString aboutText() const;

  protected:
    void closeEvent(QCloseEvent *) override;
    void dragEnterEvent(QDragEnterEvent *) override;
    void dropEvent(QDropEvent *) override;
    bool eventFilter(QObject *, QEvent *) override;

  private:
    void buildUi();
    void buildFeatureControls(QVBoxLayout *);
    void updateFeatureInspector();
    void applyVisual();
    void applyTitleProperties();
    void flushTitleText();
    void addTitle();
    void refresh();
    void updateInspector();
    void invalidatePreview();
    void startPreview(bool play);
    void togglePlay();
    void nextImport();
    void relinkMissing();
    void append(const QString &assetId);
    void step(int direction);
    void split();
    void erase();
    void copy();
    void paste();
    bool save(bool saveAs = false);
    bool confirmDiscard();
    void newProject();
    void message(const QString &text, const QString &detail = {});
    void about();
    void userGuide();
    void setupCache();
    void updateTime();
    QString recoveryPath() const;
    Session session_;
    QTranslator translator_, qtTranslator_;
    std::vector<std::function<void()>> translations_;
    MediaBin *bin_;
    Timeline *timeline_;
    QLabel *projectTitle_, *status_, *time_, *previewNote_, *inspectorTitle_, *assetInfo_,
        *libraryHint_;
    QStackedWidget *previewStack_;
    PreviewWidget *video_;
    QPushButton *play_, *export_, *cancelPreview_, *removeMedia_;
    QSlider *seek_, *zoom_;
    QDoubleSpinBox *start_, *in_, *out_, *gain_, *fadeIn_, *fadeOut_;
    QCheckBox *muted_;
    QDoubleSpinBox *titleDuration_, *duration_, *brightness_, *contrast_, *saturation_, *hue_,
        *visualIn_, *visualOut_, *titleX_, *titleY_, *titleSize_, *titleWidth_;
    QComboBox *transition_, *projectFormat_;
    QFontComboBox *titleFont_;
    QPlainTextEdit *titleText_;
    QCheckBox *titleBold_;
    QPushButton *titleColor_;
    QGroupBox *visualGroup_, *titleGroup_;
    QTabWidget *inspectorTabs_;
    std::string inspectedClip_;
    QTimer titleTimer_;
    std::string pendingTitleId_;
    QString pendingTitleText_, titleColorValue_ = "#ffffff";
    bool previewLoadPending_ = false;
    QComboBox *track_;
    QWidget *inspectorFields_;
    QProgressBar *previewProgress_;
    QMediaPlayer *player_;
    QAudioOutput *audio_;
    QAction *undo_, *redo_, *split_, *delete_, *copy_, *paste_, *duplicate_, *cut_;
    std::unique_ptr<QTemporaryDir> cache_;
    QString previewPath_;
    RenderJob *previewJob_ = nullptr;
    QTimer autosave_;
    QQueue<QString> imports_;
    ProbeJob *probe_ = nullptr;
    std::optional<Clip> clipboard_;
    bool testMode_ = false, updating_ = false, fromPlayer_ = false, wantPlay_ = false;
    std::uint64_t previewRevision_ = 0;
};
} // namespace pms
