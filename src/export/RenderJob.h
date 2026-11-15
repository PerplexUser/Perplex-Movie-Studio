#pragma once
#include "render/Composition.h"
#include <QFutureWatcher>
#include <QObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <memory>
namespace pms {
class RenderJob final : public QObject {
    Q_OBJECT
  public:
    explicit RenderJob(QObject *parent = nullptr);
    ~RenderJob() override;
    void start(const Project &, const RenderProfile &, const QString &destination, bool overwrite);
    void cancel();
    bool running() const {
        return active_;
    }
    QString chosenEncoder() const {
        return encoder_;
    }
  signals:
    void progress(double fraction);
    void succeeded(const QString &path, const QString &encoder);
    void failed(const QString &message, const QString &detail);
    void cancelled();

  private:
    void beginRender();
    void finishFailure(const QString &, const QString &detail = {});
    void cleanup();
    bool commitOutput();
    struct PreparedRender {
        Composition plan;
        int domainError = -1;
        QString error;
    };
    QFutureWatcher<PreparedRender> preparation_;
    QProcess process_;
    Composition plan_;
    RenderProfile profile_;
    QString destination_, encoder_ = "mpeg4";
    std::unique_ptr<QTemporaryDir> output_;
    std::unique_ptr<QTemporaryFile> graph_;
    QString outputPath_, commitError_;
    QByteArray errors_, progressBuffer_;
    enum Phase { Prepare, Detect, Render } phase_ = Detect;
    bool active_ = false, cancelled_ = false, overwrite_ = false;
};
} // namespace pms
