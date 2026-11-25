#include "mixvera/ui/theme.hpp"

namespace mixvera::ui::theme {

QString applicationStyleSheet()
{
    // Palette sampled from the vMix main window (dark blue-grey surfaces,
    // orange preview, dark green program) with OBS Yami's control contrast.
    return QStringLiteral(R"QSS(
* {
    font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif;
    font-size: 12px;
    color: #E7EAED;
}

QMainWindow, QWidget#root {
    background: #1E2328;
}
QDialog {
    background: #1E2328;
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
    background: #1A1F24;
    border: 1px solid #3E4853;
    border-radius: 2px;
    color: #E7EAED;
    selection-background-color: #FF8C00;
    selection-color: #14181C;
}
QToolTip {
    background: #2C333A;
    color: #E7EAED;
    border: 1px solid #4A5561;
    padding: 4px 6px;
}

QMenuBar {
    background: #1E2328;
    border-bottom: 1px solid #2C333A;
    padding: 2px 4px;
}
QMenuBar::item {
    padding: 4px 10px;
    background: transparent;
}
QMenuBar::item:selected {
    background: #333B44;
    border-radius: 2px;
}
QMenu {
    background: #262C33;
    border: 1px solid #4A5561;
    padding: 4px;
}
QMenu::item {
    padding: 6px 26px 6px 18px;
    border-radius: 2px;
}
QMenu::item:selected {
    background: #FF8C00;
    color: #14181C;
}
QMenu::separator {
    height: 1px;
    background: #3E4853;
    margin: 4px 8px;
}

QSplitter::handle {
    background: #1E2328;
}
QSplitter::handle:horizontal {
    width: 5px;
}
QSplitter::handle:vertical {
    height: 5px;
}

QFrame#topBar {
    background: #262C33;
    border-bottom: 1px solid #3E4853;
}
QLabel#brandMark {
    background: #FF8C00;
    color: #14181C;
    font-family: "Bahnschrift";
    font-size: 19px;
    font-weight: 700;
    border-radius: 2px;
}
QLabel#brandTitle {
    font-family: "Bahnschrift";
    font-size: 16px;
    font-weight: 700;
    letter-spacing: 2px;
    color: #FFFFFF;
}
QLabel#brandSubtitle {
    color: #8E98A3;
    font-size: 9px;
    letter-spacing: 1px;
}
QLabel#projectName {
    color: #C6CDD4;
    font-size: 12px;
}
QToolButton#topToggle {
    background: #2C333A;
    border: 1px solid #3E4853;
    border-radius: 2px;
    padding: 4px 10px;
    color: #D7DDE3;
}
QToolButton#topToggle:hover {
    background: #3A434D;
    border-color: #4A5561;
}
QToolButton#topToggle:checked {
    background: #3B2A17;
    border-color: #FF8C00;
    color: #FFB45C;
    font-weight: 700;
}
QPushButton#topButton {
    background: #2C333A;
    border: 1px solid #3E4853;
    border-radius: 2px;
    padding: 4px 10px;
    color: #D7DDE3;
}
QPushButton#topButton:hover {
    background: #3A434D;
    border-color: #4A5561;
}
QLabel#pill {
    background: #2C333A;
    border: 1px solid #3E4853;
    border-radius: 8px;
    padding: 2px 8px;
    color: #8E98A3;
    font-size: 9px;
    font-weight: 700;
}
QLabel#clock {
    font-family: "Consolas";
    color: #E7EAED;
    font-size: 12px;
}

QFrame#dockPanel {
    background: #262C33;
    border: 1px solid #3E4853;
    border-radius: 2px;
}
QLabel#dockTitle {
    color: #8E98A3;
    font-size: 9px;
    font-weight: 700;
    letter-spacing: 1px;
}
QToolButton#dockButton {
    background: #333B44;
    border: 1px solid #4A5561;
    border-radius: 2px;
    color: #D7DDE3;
    padding: 1px 7px;
    font-size: 11px;
}
QToolButton#dockButton:hover {
    background: #3E4853;
    border-color: #5A6572;
}
QToolButton#dockButton:pressed {
    background: #FF8C00;
    color: #14181C;
}
QToolButton#dockButton:disabled {
    color: #6E7781;
}

QListWidget#sceneList, QListWidget#sourceList {
    background: #1A1F24;
    border: 1px solid #333B44;
    border-radius: 2px;
    outline: 0;
}
QListWidget#sceneList::item, QListWidget#sourceList::item {
    padding: 5px 8px;
    border-bottom: 1px solid #262C33;
    color: #D7DDE3;
}
QListWidget#sceneList::item:selected, QListWidget#sourceList::item:selected {
    background: #343C45;
    color: #FFFFFF;
    border-left: 2px solid #FF8C00;
}
QListWidget#sceneList::item:hover, QListWidget#sourceList::item:hover {
    background: #2C333A;
}

QPushButton {
    background: #333B44;
    border: 1px solid #4A5561;
    border-radius: 2px;
    padding: 5px 14px;
    color: #E7EAED;
}
QPushButton:hover {
    background: #3E4853;
    border-color: #5A6572;
}
QPushButton:pressed {
    background: #FF8C00;
    border-color: #FF8C00;
    color: #14181C;
}
QPushButton:disabled {
    color: #6E7781;
    background: #2A3037;
}
QPushButton#cutButton {
    background: #A1172E;
    border: 1px solid #C01C37;
    color: #FFFFFF;
    font-weight: 700;
    font-size: 13px;
}
QPushButton#cutButton:hover {
    background: #C01C37;
}
QPushButton#autoButton {
    background: #2D6CDF;
    border: 1px solid #4A85F0;
    color: #FFFFFF;
    font-weight: 700;
    font-size: 13px;
}
QPushButton#autoButton:hover {
    background: #4A85F0;
}
QPushButton#autoButton:disabled {
    background: #244F9E;
    color: #B7C4F0;
}
QPushButton#ftbButton {
    min-width: 48px;
}
QPushButton#ftbButton:checked {
    background: #A1172E;
    border-color: #C01C37;
    color: #FFFFFF;
    font-weight: 700;
}
QPushButton#transitionType {
    background: #2C333A;
    border: 1px solid #3E4853;
    border-radius: 2px;
    padding: 3px 6px;
    color: #AEB7C0;
    font-size: 10px;
}
QPushButton#transitionType:hover {
    background: #3A434D;
}
QPushButton#transitionType:checked {
    background: #FF8C00;
    border-color: #FFB45C;
    color: #14181C;
    font-weight: 700;
}
QPushButton#recordButton {
    background: #2C333A;
    border: 1px solid #C01C37;
    color: #E85E75;
    font-weight: 700;
    padding: 6px 18px;
}
QPushButton#recordButton:checked {
    background: #C01C37;
    color: #FFFFFF;
}
QPushButton#streamButton {
    background: #2C333A;
    border: 1px solid #2D6CDF;
    color: #8FB3FF;
    font-weight: 700;
    padding: 6px 18px;
}
QPushButton#streamButton:checked {
    background: #2D6CDF;
    color: #FFFFFF;
}
QPushButton#footerButton {
    background: #333B44;
    border: 1px solid #4A5561;
    border-radius: 2px;
    padding: 6px 18px;
    font-weight: 600;
}
QPushButton#footerButton:hover {
    background: #3E4853;
}
QLabel#footerHint {
    color: #6E7781;
    font-size: 10px;
}

QComboBox, QSpinBox, QLineEdit, QDoubleSpinBox {
    background: #1A1F24;
    border: 1px solid #3E4853;
    border-radius: 2px;
    padding: 3px 6px;
    color: #E7EAED;
    selection-background-color: #FF8C00;
    selection-color: #14181C;
}
QComboBox:hover, QSpinBox:hover, QLineEdit:hover, QDoubleSpinBox:hover {
    border-color: #5A6572;
}
QComboBox::drop-down {
    border: none;
    width: 16px;
}
QComboBox QAbstractItemView {
    background: #262C33;
    border: 1px solid #4A5561;
    selection-background-color: #FF8C00;
    selection-color: #14181C;
    outline: 0;
}
QLineEdit:focus, QSpinBox:focus, QComboBox:focus, QDoubleSpinBox:focus {
    border-color: #FF8C00;
}
QCheckBox {
    spacing: 6px;
    color: #D7DDE3;
}
QCheckBox::indicator {
    width: 14px;
    height: 14px;
    border: 1px solid #4A5561;
    border-radius: 2px;
    background: #1A1F24;
}
QCheckBox::indicator:hover {
    border-color: #FF8C00;
}
QCheckBox::indicator:checked {
    background: #FF8C00;
    border-color: #FF8C00;
    image: url(:/icons/check.svg);
}
QCheckBox::indicator:disabled {
    background: #1E2328;
    border-color: #333B44;
}

QSlider::groove:horizontal {
    height: 5px;
    background: #14181C;
    border: 1px solid #3E4853;
    border-radius: 2px;
}
QSlider::sub-page:horizontal {
    background: #FF8C00;
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: #D7DDE3;
    border: 1px solid #FFFFFF;
    width: 11px;
    margin: -5px 0;
    border-radius: 2px;
}
QSlider::handle:horizontal:hover {
    background: #FFFFFF;
}
QSlider::groove:vertical {
    width: 12px;
    background: #14181C;
    border: 1px solid #3E4853;
    border-radius: 2px;
}
QSlider::sub-page:vertical {
    background: #14181C;
}
QSlider::add-page:vertical {
    background: #FF8C00;
    border-radius: 2px;
}
QSlider::handle:vertical {
    background: #D7DDE3;
    border: 1px solid #FFFFFF;
    height: 16px;
    margin: 0 -6px;
    border-radius: 2px;
}
QSlider#tbar::groove:vertical {
    width: 14px;
    background: #14181C;
    border: 1px solid #4A5561;
}
QSlider#tbar::add-page:vertical {
    background: #FF8C00;
}
QSlider#tbar::handle:vertical {
    background: #FFFFFF;
    border: 1px solid #FFB45C;
    height: 22px;
    margin: 0 -7px;
    border-radius: 2px;
}

QScrollBar:vertical {
    background: #1A1F24;
    width: 10px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: #4A5561;
    min-height: 24px;
    border-radius: 4px;
    margin: 2px;
}
QScrollBar::handle:vertical:hover {
    background: #5A6572;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar:horizontal {
    background: #1A1F24;
    height: 10px;
}
QScrollBar::handle:horizontal {
    background: #4A5561;
    min-width: 24px;
    border-radius: 4px;
    margin: 2px;
}
QScrollBar::handle:horizontal:hover {
    background: #5A6572;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}

QStatusBar {
    background: #14181C;
    border-top: 1px solid #2C333A;
}
QStatusBar QLabel {
    color: #8E98A3;
    font-size: 10px;
    padding: 0 8px;
}
QLabel#statusOnAir {
    color: #E85E75;
    font-weight: 700;
}

QFrame#monitorFrame {
    background: #0B0D12;
    border: 1px solid #3E4853;
    border-radius: 1px;
}
QFrame#monitorFrame[role="program"] {
    border-color: #006400;
}
QFrame#monitorFrame[role="preview"] {
    border-color: #FF8C00;
}

QFrame#transitionBar {
    background: #262C33;
    border: 1px solid #3E4853;
    border-radius: 2px;
}
QLabel#controlLabel {
    color: #8E98A3;
    font-size: 9px;
    font-weight: 700;
    letter-spacing: 1px;
}
QLabel#mixReadout {
    font-family: "Consolas";
    color: #8FB3FF;
    font-size: 11px;
}

QFrame#mixerPanel {
    background: #262C33;
    border-left: 1px solid #3E4853;
}
QFrame#channelStrip {
    background: #1A1F24;
    border: 1px solid #333B44;
    border-radius: 2px;
}
QLabel#channelName {
    color: #D7DDE3;
    font-size: 10px;
}
QLabel#channelDb {
    font-family: "Consolas";
    color: #8E98A3;
    font-size: 9px;
}
QToolButton#muteButton {
    background: #2C333A;
    border: 1px solid #4A5561;
    border-radius: 2px;
    color: #8E98A3;
    font-size: 9px;
    font-weight: 700;
    padding: 0;
}
QToolButton#muteButton:checked {
    background: #A1172E;
    border-color: #C01C37;
    color: #FFFFFF;
}

QFrame#inputBar {
    background: #262C33;
    border-top: 1px solid #3E4853;
    border-bottom: 1px solid #3E4853;
}
QLabel#inputCount {
    color: #8E98A3;
    font-size: 10px;
}
QLineEdit#inputSearch {
    background: #1A1F24;
    border: 1px solid #3E4853;
    border-radius: 2px;
    padding: 3px 6px;
    font-size: 10px;
}
QToolButton#filterChip {
    background: #2C333A;
    border: 1px solid #3E4853;
    border-radius: 2px;
    padding: 3px 9px;
    color: #AEB7C0;
    font-size: 10px;
}
QToolButton#filterChip:hover {
    background: #3A434D;
}
QToolButton#filterChip:checked {
    background: #3B2A17;
    border-color: #FF8C00;
    color: #FFB45C;
    font-weight: 700;
}
QScrollArea#inputScroll {
    background: transparent;
    border: none;
}
QWidget#inputStrip {
    background: transparent;
}
QFrame#inputTile {
    background: #1A1F24;
    border: 1px solid #333B44;
    border-radius: 2px;
}
QFrame#inputTile:hover {
    border-color: #5A6572;
}
QFrame#inputTile[selected="true"] {
    border: 1px solid #FF8C00;
}
QLabel#inputTileHeader {
    color: #FFFFFF;
    font-size: 10px;
    font-weight: 700;
    background: #333B44;
    padding-left: 5px;
}
QLabel#inputTileHeader[role="program"] {
    background: #006400;
}
QLabel#inputTileHeader[role="preview"] {
    background: #FF8C00;
    color: #14181C;
}
QLabel#inputTileThumb {
    background: #000000;
    border: none;
}
QToolButton#tileAction {
    background: #2C333A;
    border: 1px solid #3E4853;
    border-radius: 2px;
    color: #C6CDD4;
    font-size: 9px;
    font-weight: 700;
    padding: 0;
}
QToolButton#tileAction:hover {
    background: #3A434D;
    border-color: #FF8C00;
    color: #FFFFFF;
}
QLabel#inputEmpty {
    color: #6E7781;
    font-size: 11px;
    padding: 40px;
}

QLabel#pillRec {
    background: #2C333A;
    border: 1px solid #3E4853;
    border-radius: 8px;
    padding: 2px 8px;
    color: #6E7781;
    font-size: 9px;
    font-weight: 700;
}
QLabel#pillRec[live="true"] {
    background: #A1172E;
    border-color: #C01C37;
    color: #FFFFFF;
}
QLabel#pillStream {
    background: #2C333A;
    border: 1px solid #3E4853;
    border-radius: 8px;
    padding: 2px 8px;
    color: #6E7781;
    font-size: 9px;
    font-weight: 700;
}
QLabel#pillStream[live="true"] {
    background: #2D6CDF;
    border-color: #4A85F0;
    color: #FFFFFF;
}
)QSS");
}

} // namespace mixvera::ui::theme
