#include "app/Session.h"
#include "media/MediaTools.h"
#include <algorithm>
namespace pms {
bool Session::edit(const std::function<void(Editor &)> &fn) {
    auto before = editor.project();
    try {
        fn(editor);
        if (before != editor.project())
            changedProject();
        return true;
    } catch (const DomainError &e) {
        emit error(errorText(int(e.code)), {});
    } catch (const std::exception &e) {
        emit error(tr("The operation could not be completed."), QString::fromUtf8(e.what()));
    }
    return false;
}
void Session::select(const std::string &id) {
    selected = id;
    emit selectionChanged();
}
void Session::seek(Time t) {
    playhead = std::clamp<Time>(t, 0, project().duration());
    emit positionChanged(playhead);
}
void Session::changedProject() {
    ++revision;
    try {
        if (!selected.empty())
            project().clip(selected);
    } catch (...) {
        selected.clear();
    }
    playhead = std::clamp<Time>(playhead, 0, project().duration());
    emit changed();
    emit selectionChanged();
    emit positionChanged(playhead);
}
} // namespace pms
