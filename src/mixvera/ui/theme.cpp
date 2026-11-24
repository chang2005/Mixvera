#include "mixvera/ui/theme.hpp"

namespace mixvera::ui::theme {

QString applicationStyleSheet()
{
    return QStringLiteral(R"QSS(
/* Palette derived from the OBS Yami theme values: grey scale surfaces with a
   single blue accent, red reserved for on-air / record states. */
* {
    font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif;
    font-size: 12px;
    color: #E1E1E1;
}

QMainWindow, QWidget#root {
    background: #1D1F26;
}

QDialog {
    background: #1D1F26;
}
QScrollArea {
    background: transparent;
    border: none;
}
QScrollArea > QWidget > QWidget {
    background: transparent;
}
QWidget#stripArea {
    background: transparent;
}
QPlainTextEdit, QTextEdit {
    background: #1D1F26;
    border: 1px solid #3C404D;
    border-radius: 3px;
    color: #E1E1E1;
    selection-background-color: #284CB8;
}
QToolTip {
    background: #272A33;
    color: #E1E1E1;
    border: 1px solid #3C404D;
    padding: 4px 6px;
}

QMenuBar {
    background: #1D1F26;
    border-bottom: 1px solid #323540;
    padding: 2px 6px;
}
QMenuBar::item {
    padding: 4px 10px;
    background: transparent;
}
QMenuBar::item:selected {
    background: #323540;
    border-radius: 3px;
}
QMenu {
    background: #272A33;
    border: 1px solid #3C404D;
    padding: 4px;
}
QMenu::item {
    padding: 6px 26px 6px 18px;
    border-radius: 3px;
}
QMenu::item:selected {
    background: #284CB8;
}
QMenu::separator {
    height: 1px;
    background: #3C404D;
    margin: 4px 8px;
}

QSplitter::handle {
    background: #1D1F26;
}
QSplitter::handle:horizontal {
    width: 6px;
}
QSplitter::handle:vertical {
    height: 6px;
}

QFrame#topBar {
    background: #272A33;
    border-bottom: 1px solid #3C404D;
}
QLabel#brandMark {
    background: #284CB8;
    color: #FFFFFF;
    font-family: "Bahnschrift";
    font-size: 19px;
    font-weight: 700;
    border-radius: 4px;
}
QLabel#brandTitle {
    font-family: "Bahnschrift";
    font-size: 16px;
    font-weight: 700;
    letter-spacing: 2px;
    color: #FFFFFF;
}
QLabel#brandSubtitle {
    color: #969696;
    font-size: 10px;
    letter-spacing: 1px;
}
QLabel#projectName {
    color: #B4B4B4;
    font-size: 12px;
}
QLabel#pill {
    background: #323540;
    border: 1px solid #3C404D;
    border-radius: 9px;
    padding: 2px 9px;
    color: #B4B4B4;
    font-size: 10px;
    letter-spacing: 0.5px;
}
QLabel#clock {
    font-family: "Consolas";
    color: #E1E1E1;
    font-size: 13px;
}

QFrame#dockPanel {
    background: #272A33;
    border: 1px solid #323540;
    border-radius: 4px;
}
QLabel#dockTitle {
    color: #969696;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 1px;
}
QToolButton#dockButton {
    background: #323540;
    border: 1px solid #3C404D;
    border-radius: 3px;
    color: #D2D2D2;
    padding: 1px 7px;
    font-size: 12px;
}
QToolButton#dockButton:hover {
    background: #3C404D;
    border-color: #4E5566;
}
QToolButton#dockButton:pressed {
    background: #284CB8;
}
QToolButton#dockButton:disabled {
    color: #646464;
}

QListWidget#sceneList, QListWidget#sourceList, QListWidget#inputList {
    background: #1D1F26;
    border: 1px solid #323540;
    border-radius: 3px;
    outline: 0;
}
QListWidget#sceneList::item, QListWidget#sourceList::item {
    padding: 5px 8px;
    border-bottom: 1px solid #262932;
}
QListWidget#sceneList::item:selected, QListWidget#sourceList::item:selected {
    background: #284CB8;
    color: #FFFFFF;
}
QListWidget#sceneList::item:hover, QListWidget#sourceList::item:hover {
    background: #2E323D;
}
QListWidget#inputList::item {
    border-bottom: 1px solid #262932;
}

QPushButton {
    background: #323540;
    border: 1px solid #3C404D;
    border-radius: 3px;
    padding: 5px 14px;
    color: #E1E1E1;
}
QPushButton:hover {
    background: #3C404D;
    border-color: #4E5566;
}
QPushButton:pressed {
    background: #284CB8;
    border-color: #284CB8;
}
QPushButton:disabled {
    color: #646464;
    background: #2A2D36;
}
QPushButton#cutButton {
    background: #A1172E;
    border: 1px solid #C01C37;
    color: #FFFFFF;
    font-weight: 700;
    min-width: 62px;
}
QPushButton#cutButton:hover {
    background: #C01C37;
}
QPushButton#autoButton {
    background: #284CB8;
    border: 1px solid #476BD7;
    color: #FFFFFF;
    font-weight: 700;
    min-width: 62px;
}
QPushButton#autoButton:hover {
    background: #476BD7;
}
QPushButton#autoButton:disabled {
    background: #213E97;
    color: #B7C4F0;
}
QPushButton#ftbButton {
    min-width: 52px;
}
QPushButton#ftbButton:checked {
    background: #A1172E;
    border-color: #C01C37;
    color: #FFFFFF;
}
QPushButton#recordButton {
    background: #323540;
    border: 1px solid #C01C37;
    color: #E85E75;
    font-weight: 700;
    padding: 4px 12px;
}
QPushButton#recordButton:checked {
    background: #C01C37;
    color: #FFFFFF;
}
QPushButton#streamButton {
    background: #323540;
    border: 1px solid #284CB8;
    color: #8FA6E8;
    font-weight: 700;
    padding: 4px 12px;
}
QPushButton#streamButton:checked {
    background: #284CB8;
    color: #FFFFFF;
}
QPushButton#transitionType {
    background: #272A33;
    border: 1px solid #3C404D;
    border-radius: 3px;
    padding: 3px 10px;
    min-width: 44px;
    color: #B4B4B4;
}
QPushButton#transitionType:hover {
    background: #3C404D;
}
QPushButton#transitionType:checked {
    background: #284CB8;
    border-color: #476BD7;
    color: #FFFFFF;
    font-weight: 700;
}

QComboBox, QSpinBox, QLineEdit, QDoubleSpinBox {
    background: #1D1F26;
    border: 1px solid #3C404D;
    border-radius: 3px;
    padding: 3px 6px;
    color: #E1E1E1;
    selection-background-color: #284CB8;
}
QComboBox:hover, QSpinBox:hover, QLineEdit:hover {
    border-color: #4E5566;
}
QComboBox::drop-down {
    border: none;
    width: 16px;
}
QComboBox QAbstractItemView {
    background: #272A33;
    border: 1px solid #3C404D;
    selection-background-color: #284CB8;
    outline: 0;
}
QLineEdit:focus, QSpinBox:focus, QComboBox:focus {
    border-color: #476BD7;
}
QCheckBox {
    spacing: 6px;
    color: #D2D2D2;
}
QCheckBox::indicator {
    width: 14px;
    height: 14px;
    border: 1px solid #3C404D;
    border-radius: 2px;
    background: #13141A;
}
QCheckBox::indicator:hover {
    border-color: #476BD7;
}
QCheckBox::indicator:checked {
    background: #284CB8;
    border-color: #476BD7;
}
QCheckBox::indicator:disabled {
    background: #1D1F26;
    border-color: #323540;
}

QSlider::groove:horizontal {
    height: 6px;
    background: #13141A;
    border: 1px solid #3C404D;
    border-radius: 3px;
}
QSlider::sub-page:horizontal {
    background: #284CB8;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: #B4B4B4;
    border: 1px solid #D2D2D2;
    width: 12px;
    margin: -6px 0;
    border-radius: 3px;
}
QSlider::handle:horizontal:hover {
    background: #FFFFFF;
}
QSlider#tbar::groove:horizontal {
    height: 12px;
    background: #13141A;
    border: 1px solid #3C404D;
    border-radius: 2px;
}
QSlider#tbar::sub-page:horizontal {
    background: #213E97;
    border-radius: 2px;
}
QSlider#tbar::handle:horizontal {
    background: #D2D2D2;
    width: 8px;
    margin: -4px 0;
    border-radius: 1px;
}

QSlider::groove:vertical {
    width: 5px;
    background: #13141A;
    border: 1px solid #3C404D;
    border-radius: 2px;
}
QSlider::sub-page:vertical {
    background: #13141A;
}
QSlider::add-page:vertical {
    background: #284CB8;
}
QSlider::handle:vertical {
    background: #B4B4B4;
    border: 1px solid #D2D2D2;
    height: 10px;
    margin: 0 -5px;
    border-radius: 2px;
}
QSlider::handle:vertical:hover {
    background: #FFFFFF;
}

QScrollBar:vertical {
    background: #1D1F26;
    width: 10px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: #3C404D;
    min-height: 24px;
    border-radius: 4px;
    margin: 2px;
}
QScrollBar::handle:vertical:hover {
    background: #4E5566;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar:horizontal {
    background: #1D1F26;
    height: 10px;
}
QScrollBar::handle:horizontal {
    background: #3C404D;
    min-width: 24px;
    border-radius: 4px;
    margin: 2px;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}

QStatusBar {
    background: #1D1F26;
    border-top: 1px solid #323540;
}
QStatusBar QLabel {
    color: #969696;
    font-size: 11px;
    padding: 0 8px;
}
QLabel#statusOnAir {
    color: #E85E75;
    font-weight: 700;
}

QLabel#monitorTitle {
    font-family: "Bahnschrift";
    font-size: 12px;
    font-weight: 700;
    letter-spacing: 1px;
    color: #FFFFFF;
}
QLabel#monitorSource {
    color: #969696;
    font-size: 11px;
}
QLabel#monitorBadge {
    background: #A1172E;
    color: #FFFFFF;
    border-radius: 3px;
    padding: 1px 7px;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 1px;
}
QLabel#monitorBadge[role="preview"] {
    background: #17641E;
}
QLabel#monitorBadge[role="off"] {
    background: #323540;
    color: #969696;
}
QLabel#stageTimecode {
    font-family: "Consolas";
    color: #D2D2D2;
    font-size: 11px;
}

QFrame#monitorFrame {
    background: #13141A;
    border: 1px solid #323540;
    border-radius: 4px;
}
QFrame#monitorFrame[role="program"] {
    border-color: #C01C37;
}
QFrame#monitorFrame[role="preview"] {
    border-color: #25A231;
}

QFrame#transitionBar {
    background: #272A33;
    border: 1px solid #323540;
    border-radius: 4px;
}
QLabel#controlLabel {
    color: #969696;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 1px;
}
QLabel#mixReadout {
    font-family: "Consolas";
    color: #8FA6E8;
    font-size: 12px;
}

QFrame#mixerPanel {
    background: #272A33;
    border: 1px solid #323540;
    border-radius: 4px;
}
QFrame#channelStrip {
    background: #1D1F26;
    border: 1px solid #323540;
    border-radius: 3px;
}
QLabel#channelName {
    color: #D2D2D2;
    font-size: 10px;
}
QLabel#channelDb {
    font-family: "Consolas";
    color: #969696;
    font-size: 9px;
}
QToolButton#muteButton {
    background: #272A33;
    border: 1px solid #3C404D;
    border-radius: 2px;
    color: #969696;
    font-size: 9px;
    font-weight: 700;
    padding: 1px 3px;
}
QToolButton#muteButton:checked {
    background: #A1172E;
    border-color: #C01C37;
    color: #FFFFFF;
}
QToolButton#soloButton:checked {
    background: #B88A16;
    border-color: #E5AF24;
    color: #1D1F26;
}

QLabel#pillRec {
    background: #323540;
    border: 1px solid #3C404D;
    border-radius: 9px;
    padding: 2px 8px;
    color: #646464;
    font-size: 10px;
    font-weight: 700;
}
QLabel#pillRec[live="true"] {
    background: #A1172E;
    border-color: #C01C37;
    color: #FFFFFF;
}
QLabel#pillStream {
    background: #323540;
    border: 1px solid #3C404D;
    border-radius: 9px;
    padding: 2px 8px;
    color: #646464;
    font-size: 10px;
    font-weight: 700;
}
QLabel#pillStream[live="true"] {
    background: #284CB8;
    border-color: #476BD7;
    color: #FFFFFF;
}
)QSS");
}

} // namespace mixvera::ui::theme