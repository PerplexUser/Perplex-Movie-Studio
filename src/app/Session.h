#pragma once
#include "commands/Editor.h"
#include <QObject>
namespace pms {
class Session final : public QObject {
    Q_OBJECT
  public:
    explicit Session(QObject *parent = nullptr) : QObject(parent) {}
    Editor editor;
    QString path;
    std::string selected;
    Time playhead = 0;
    std::uint64_t revision = 0;
    const Project &project() const {
        return editor.project();
    }
    bool edit(const std::function<void(Editor &)> &);
    void select(const std::string &id);
    void seek(Time);
    void changedProject();
  signals:
    void changed();
    void selectionChanged();
    void positionChanged(qint64 time);
    void error(const QString &text, const QString &detail);
};
} // namespace pms
