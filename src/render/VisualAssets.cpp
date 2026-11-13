#include "render/VisualAssets.h"
#include <QDir>
#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QImageReader>
#include <QPainter>
#include <cmath>
namespace pms {
QHash<QString, QString> prepareVisualAssets(const Project &p, const RenderProfile &profile,
                                            const QString &dir) {
    p.validate();
    if (profile.width < 16 || profile.height < 16 || profile.width > 3840 || profile.height > 3840)
        throw DomainError(Error::InvalidProject);
    QHash<QString, QString> files;
    if (profile.format != "mp4" && profile.format != "mkv")
        return files;
    for (const auto &c : p.clips) {
        const auto &a = p.asset(c.assetId);
        if (a.kind == AssetKind::Media)
            continue;
        auto key = QString::fromStdString(a.kind == AssetKind::Title ? c.id : a.id);
        if (files.contains(key))
            continue;
        if (files.size() >= 128)
            throw DomainError(Error::LimitExceeded);
        QImage canvas(profile.width, profile.height, QImage::Format_ARGB32_Premultiplied);
        canvas.fill(Qt::transparent);
        QPainter painter(&canvas);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        if (a.kind == AssetKind::Title) {
            QFont font(QString::fromStdString(c.title.font));
            double scale = double(profile.height) / p.settings.height;
            font.setPixelSize(std::max(1, int(std::lround(c.title.size * scale))));
            font.setBold(c.title.bold);
            painter.setFont(font);
            int width = std::max(1, int(profile.width * c.title.width));
            auto text = QString::fromStdString(c.title.text);
            int flags = Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap;
            auto bounds = QFontMetrics(font).boundingRect(QRect(0, 0, width, profile.height * 4),
                                                          flags, text);
            QRectF rect(profile.width * c.title.x - width / 2.0,
                        profile.height * c.title.y - bounds.height() / 2.0, width, bounds.height());
            double shadow = std::max(1.0, 3.0 * scale);
            painter.setPen(QColor(0, 0, 0, 210));
            painter.drawText(rect.translated(shadow, shadow), flags, text);
            painter.setPen(QColor(QString::fromStdString(c.title.color)));
            painter.drawText(rect, flags, text);
        } else {
            QImageReader reader(QString::fromStdString(a.path));
            reader.setAutoTransform(true);
            auto size = reader.size();
            if (!size.isValid() || size.width() > 8192 || size.height() > 8192 ||
                qint64(size.width()) * size.height() > 32 * 1024 * 1024)
                throw DomainError(Error::LimitExceeded);
            auto image = reader.read();
            if (image.isNull())
                throw DomainError(Error::InvalidProject);
            auto fitted = image.size().scaled(canvas.size(), Qt::KeepAspectRatio);
            painter.drawImage(QRect(QPoint((canvas.width() - fitted.width()) / 2,
                                           (canvas.height() - fitted.height()) / 2),
                                    fitted),
                              image);
        }
        painter.end();
        auto rgba = canvas.convertToFormat(QImage::Format_RGBA8888);
        auto path = QDir(dir).filePath(QString("visual-%1.rgba").arg(files.size()));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly) ||
            file.write(reinterpret_cast<const char *>(rgba.constBits()), rgba.sizeInBytes()) !=
                rgba.sizeInBytes())
            throw std::runtime_error("Could not prepare image or text");
        file.close();
        files.insert(key, path);
    }
    return files;
}
} // namespace pms
