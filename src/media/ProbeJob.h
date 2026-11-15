#pragma once
#include "domain/Project.h"
#include <QFutureWatcher>
#include <QObject>
#include <QProcess>
#include <QTimer>
namespace pms {
class ProbeJob final : public QObject {
    Q_OBJECT
  public:
    explicit ProbeJob(QObject *parent = nullptr);
    ~ProbeJob() override;
    void start(const QString &path);
    void cancel();
    static Asset parse(const QByteArray &json, const QString &path);
  signals:
    void succeeded(const pms::Asset &asset);
    void failed(const QString &message, const QString &detail);

  private:
    void fail(const QString &, const QString &detail = {});
    QFutureWatcher<Asset> imageProbe_;
    QProcess process_;
    QTimer timeout_;
    QByteArray data_, errors_;
    QString path_;
    bool finished_ = false, decoding_ = false;
    Asset asset_;
};
} // namespace pms
Q_DECLARE_METATYPE(pms::Asset)
