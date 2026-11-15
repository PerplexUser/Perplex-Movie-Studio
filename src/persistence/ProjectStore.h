#pragma once
#include "domain/Project.h"
#include <QJsonObject>
#include <QStringList>
namespace pms {
class ProjectStore {
  public:
    static QJsonObject encode(const Project &, const QString &filePath);
    static Project decode(const QJsonObject &, const QString &filePath);
    static void save(const Project &, const QString &filePath);
    static Project load(const QString &filePath);
    static QStringList missing(const Project &);
};
} // namespace pms
