#pragma once
#include "render/Composition.h"
#include <QHash>
namespace pms {
QHash<QString, QString> prepareVisualAssets(const Project &, const RenderProfile &,
                                            const QString &);
}
