#pragma once
#include "domain/Project.h"
#include <QHash>
#include <QStringList>
namespace pms {
struct RenderProfile {
    int width = 1920, height = 1080, fps = 30;
    QString format = "mp4", quality = "medium", encoder = "auto";
    bool operator==(const RenderProfile &) const = default;
};
struct Composition {
    QStringList inputArgs;
    QString filterGraph;
    Time duration = 0;
};
Composition compose(const Project &, const RenderProfile &,
                    const QHash<QString, QString> &visualAssets = {});
QStringList encodingArgs(const RenderProfile &, const QString &encoder);
} // namespace pms
