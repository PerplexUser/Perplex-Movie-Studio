#pragma once
#include "domain/Project.h"
#include <functional>
#include <optional>
namespace pms {
class Editor {
  public:
    explicit Editor(Project p = makeProject());
    const Project &project() const {
        return current_;
    }
    bool dirty() const {
        return current_ != saved_;
    }
    bool canUndo() const {
        return !undo_.empty();
    }
    bool canRedo() const {
        return !redo_.empty();
    }
    void markSaved() {
        saved_ = current_;
    }
    void reset(Project);
    bool apply(const std::function<void(Project &)> &);
    void undo();
    void redo();
    std::string addAsset(Asset);
    void removeAsset(const std::string &);
    std::string addTitle(Time, const std::string &);
    void visual(const std::string &, Visual);
    void title(const std::string &, const Title &);
    void settings(const Settings &);
    std::string addClip(const std::string &assetId, const std::string &trackId, Time start);
    std::string split(const std::string &id, Time at);
    void move(const std::string &id, const std::string &trackId, Time start);
    void trim(const std::string &id, Time sourceIn, Time sourceOut, Time start);
    void erase(const std::string &id);
    std::string duplicate(const std::string &id);
    std::string paste(Clip clip, const std::string &trackId, Time start);
    void audio(const std::string &id, double gain, bool muted, Time fadeIn, Time fadeOut);
    void trackState(const std::string &id, bool muted, bool locked);
    void relink(const std::string &assetId, const Asset &replacement);

  private:
    Project current_, saved_;
    std::vector<Project> undo_, redo_;
};
} // namespace pms
