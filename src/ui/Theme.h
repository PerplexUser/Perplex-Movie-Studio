#pragma once
namespace pms {
inline constexpr const char *Theme = R"(
QWidget { background:#1a2029; color:#dce4ef; font-family:'Segoe UI','Noto Sans',sans-serif; font-size:12px; }
QMainWindow { background:#11161e; }
QMenuBar { background:#151a22; padding:3px 9px; color:#acb8c9; }
QMenuBar::item { padding:6px 11px; background:transparent; } QMenuBar::item:selected,QMenu::item:selected { background:#34443e; color:#b4f8d8; }
QMenu { border:1px solid #3b4655; padding:7px; } QMenu::item { padding:7px 25px; }
QToolBar { background:#1a2029; border:0; spacing:8px; padding:8px; }
QToolBar#mainBar { border-bottom:1px solid #333c49; padding:14px 10px; }
QToolBar#timelineBar { background:#1d242e; padding:8px; border-bottom:1px solid #333c49; }
QToolButton { border:1px solid transparent; padding:6px 10px; border-radius:4px; }
QToolButton:hover { background:#303a48; } QToolButton:disabled { color:#596574; }
QPushButton { background:#293342; border:1px solid #3b485b; padding:8px 13px; border-radius:5px; font-weight:500; }
QPushButton:hover { background:#354356; border-color:#63728a; } QPushButton:pressed { background:#40536a; }
QPushButton:disabled { background:#232a34; border-color:#303845; color:#667386; }
QPushButton#primaryButton { background:#84e1bc; border:1px solid #84e1bc; color:#142b23; font-weight:600; }
QPushButton#primaryButton:hover { background:#a2f3d4; } QPushButton#primaryButton:disabled { background:#2e5045; border-color:#2e5045; color:#6e9385; }
QPushButton#playButton { background:#2f4b42; color:#b5f4d9; }
QLabel { background:transparent; } QLabel#brand { color:#b7f2d7; font-size:15px; font-weight:700; }
QLabel#projectTitle { color:#a2afc1; font-size:12px; } QLabel#sectionTitle { font-size:16px; font-weight:600; }
QLabel#dialogTitle { font-size:21px; font-weight:600; padding:10px 0; } QLabel#mutedText { color:#97a6ba; line-height:1.5; }
QLabel#welcomeTitle { color:#d4e0ed; font-size:21px; font-weight:500; } QLabel#playMark { font-size:64px; color:#81c6aa; }
QLabel#timecode { font-family:'Consolas','DejaVu Sans Mono',monospace; color:#d6e7e2; font-size:12px; padding:5px; }
QStackedWidget#previewSurface,QStackedWidget#previewSurface QWidget { background:#10151d; }
QDockWidget { titlebar-close-icon:url(none); } QDockWidget::title { background:#202833; color:#95a8bf; padding:9px 12px; font-size:10px; }
QMainWindow::separator { width:5px; height:5px; background:#11161e; } QMainWindow::separator:hover { background:#5da78d; }
QListWidget { background:#1a2029; border:none; outline:none; } QListWidget::item { border:1px solid #333f4d; border-radius:5px; background:#202936; padding:5px; } QListWidget::item:selected { border-color:#84e1bc; background:#293d39; }
QLineEdit,QDoubleSpinBox,QComboBox,QTextEdit,QPlainTextEdit { background:#141b24; border:1px solid #3a4658; border-radius:4px; padding:6px; selection-background-color:#326951; }
QLineEdit:focus,QDoubleSpinBox:focus,QComboBox:focus,QPushButton:focus,QListWidget:focus { border:1px solid #93ebc6; }
QDoubleSpinBox:disabled,QComboBox:disabled { color:#667386; border-color:#2b3441; } QComboBox QAbstractItemView { background:#202a36; selection-background-color:#345c4d; }
QSlider::groove:horizontal { height:4px; background:#3c495a; border-radius:2px; } QSlider::sub-page:horizontal { background:#77cba9; border-radius:2px; } QSlider::handle:horizontal { width:11px; margin:-4px 0; background:#c6f3e0; border-radius:5px; }
QScrollBar:horizontal { height:11px; background:#151b24; } QScrollBar:vertical { width:11px; background:#151b24; } QScrollBar::handle { background:#425269; border-radius:4px; min-height:24px; min-width:24px; } QScrollBar::add-line,QScrollBar::sub-line { width:0;height:0; }
QStatusBar { background:#151b23; border-top:1px solid #2c3643; color:#8da18f; } QStatusBar QLabel { font-size:11px; color:#91a498; padding:3px 9px; } QStatusBar::item { border:0; }
QProgressBar { border:1px solid #364354; border-radius:4px; text-align:center; min-height:16px; background:#141b24; } QProgressBar::chunk { background:#3f9a77; border-radius:3px; }
QCheckBox { spacing:7px; } QCheckBox::indicator { width:15px; height:15px; border:1px solid #5f7087; border-radius:3px; background:#17202b; } QCheckBox::indicator:checked { background:#6dc69f; border:1px solid #a6f2d1; }
QTabWidget::pane { border:0; } QTabBar::tab { background:#222d39; padding:9px 12px; color:#aebdcd; border-bottom:2px solid transparent; } QTabBar::tab:selected { color:#b7f2d7; border-bottom:2px solid #84e1bc; } QGroupBox { border:0; padding-top:6px; }
QToolTip { background:#293649; color:#e5edf6; border:1px solid #586b84; padding:6px; }
)";
}
