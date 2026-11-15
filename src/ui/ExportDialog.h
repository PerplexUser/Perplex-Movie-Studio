#pragma once
#include "export/RenderJob.h"
#include <QDialog>
class QComboBox;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QLabel;
class QCloseEvent;
namespace pms {
class ExportDialog : public QDialog {
    Q_OBJECT
  public:
    ExportDialog(Project project, QWidget *parent = nullptr);

  protected:
    void closeEvent(QCloseEvent *) override;

  private:
    void start();
    void stop();
    Project project_;
    RenderJob *job_ = nullptr;
    QComboBox *orientation_, *format_, *resolution_, *fps_, *quality_;
    QLineEdit *path_;
    QProgressBar *progress_;
    QPushButton *export_, *cancel_, *browse_;
    QLabel *status_;
    QWidget *fields_;
};
} // namespace pms
