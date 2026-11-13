#pragma once
#include <QString>
#include <QStringList>
namespace pms {
QString mediaTool(const QString &name);
QString seconds(qint64 microseconds);
QString timecode(qint64 microseconds, int fpsNum = 30, int fpsDen = 1);
QString errorText(int code);
QStringList localInput(const QString &path);
} // namespace pms
