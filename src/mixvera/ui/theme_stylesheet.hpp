// Generated from the 0.3 stylesheet; edit tokens, not literal colours.
#pragma once

namespace mixvera::ui::theme {

inline constexpr const char* kStyleSheetTemplate = R"QSS(
* {
    font-family: "Segoe UI", "Microsoft YaHei UI", sans-serif;
    font-size: 12px;
    color: {{text}};
}

QMainWindow, QWidget#root {
    background: {{window}};
}
QDialog {
    background: {{window}};
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
    background: {{field}};
    border: 1px solid {{border}};
    border-radius: 2px;
    color: {{text}};
    selection-background-color: {{accent}};
    selection-color: {{onAccent}};
}
QToolTip {
    background: {{surfaceAlt}};
    color: {{text}};
    border: 1px solid {{borderStrong}};
    padding: 4px 6px;
}

QMenuBar {
    background: {{window}};
    border-bottom: 1px solid {{surfaceAlt}};
    padding: 2px 4px;
}
QMenuBar::item {
    padding: 4px 10px;
    background: transparent;
}
QMenuBar::item:selected {
    background: {{raised}};
    border-radius: 2px;
}
QMenu {
    background: {{surface}};
    border: 1px solid {{borderStrong}};
    padding: 4px;
}
QMenu::item {
    padding: 6px 26px 6px 18px;
    border-radius: 2px;
}
QMenu::item:selected {
    background: {{accent}};
    color: {{onAccent}};
}
QMenu::separator {
    height: 1px;
    background: {{border}};
    margin: 4px 8px;
}

QSplitter::handle {
    background: {{window}};
}
QSplitter::handle:horizontal {
    width: 5px;
}
QSplitter::handle:vertical {
    height: 5px;
}

QFrame#topBar {
    background: {{surface}};
    border-bottom: 1px solid {{border}};
}
QLabel#brandMark {
    background: {{accent}};
    color: {{onAccent}};
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
    color: {{textStrong}};
}
QLabel#brandSubtitle {
    color: {{textMuted}};
    font-size: 9px;
    letter-spacing: 1px;
}
QLabel#projectName {
    color: {{textSecondary}};
    font-size: 12px;
}
QToolButton#topToggle {
    background: {{surfaceAlt}};
    border: 1px solid {{border}};
    border-radius: 2px;
    padding: 4px 10px;
    color: {{textSecondary}};
}
QToolButton#topToggle:hover {
    background: {{hover}};
    border-color: {{borderStrong}};
}
QToolButton#topToggle:checked {
    background: {{accentSoft}};
    border-color: {{accent}};
    color: {{accentHover}};
    font-weight: 700;
}
QPushButton#topButton {
    background: {{surfaceAlt}};
    border: 1px solid {{border}};
    border-radius: 2px;
    padding: 4px 10px;
    color: {{textSecondary}};
}
QPushButton#topButton:hover {
    background: {{hover}};
    border-color: {{borderStrong}};
}
QLabel#pill {
    background: {{surfaceAlt}};
    border: 1px solid {{border}};
    border-radius: 8px;
    padding: 2px 8px;
    color: {{textMuted}};
    font-size: 9px;
    font-weight: 700;
}
QLabel#clock {
    font-family: "Consolas";
    color: {{text}};
    font-size: 12px;
}

QFrame#dockPanel {
    background: {{surface}};
    border: 1px solid {{border}};
    border-radius: 2px;
}
QLabel#dockTitle {
    color: {{textMuted}};
    font-size: 9px;
    font-weight: 700;
    letter-spacing: 1px;
}
QToolButton#dockButton {
    background: {{raised}};
    border: 1px solid {{borderStrong}};
    border-radius: 2px;
    color: {{textSecondary}};
    padding: 1px 7px;
    font-size: 11px;
}
QToolButton#dockButton:hover {
    background: {{border}};
    border-color: {{borderHover}};
}
QToolButton#dockButton:pressed {
    background: {{accent}};
    color: {{onAccent}};
}
QToolButton#dockButton:disabled {
    color: {{textDim}};
}

QListWidget#sceneList, QListWidget#sourceList {
    background: {{field}};
    border: 1px solid {{raised}};
    border-radius: 2px;
    outline: 0;
}
QListWidget#sceneList::item, QListWidget#sourceList::item {
    padding: 5px 8px;
    border-bottom: 1px solid {{surface}};
    color: {{textSecondary}};
}
QListWidget#sceneList::item:selected, QListWidget#sourceList::item:selected {
    background: {{selection}};
    color: {{textStrong}};
    border-left: 2px solid {{accent}};
}
QListWidget#sceneList::item:hover, QListWidget#sourceList::item:hover {
    background: {{surfaceAlt}};
}

QPushButton {
    background: {{raised}};
    border: 1px solid {{borderStrong}};
    border-radius: 2px;
    padding: 5px 14px;
    color: {{text}};
}
QPushButton:hover {
    background: {{border}};
    border-color: {{borderHover}};
}
QPushButton:pressed {
    background: {{accent}};
    border-color: {{accent}};
    color: {{onAccent}};
}
QPushButton:disabled {
    color: {{textDim}};
    background: {{disabled}};
}
QPushButton#cutButton {
    background: {{dangerDark}};
    border: 1px solid {{danger}};
    color: {{onDanger}};
    font-weight: 700;
    font-size: 13px;
}
QPushButton#cutButton:hover {
    background: {{danger}};
}
QPushButton#autoButton {
    background: {{info}};
    border: 1px solid {{infoHover}};
    color: {{onInfo}};
    font-weight: 700;
    font-size: 13px;
}
QPushButton#autoButton:hover {
    background: {{infoHover}};
}
QPushButton#autoButton:disabled {
    background: {{infoDark}};
    color: {{infoTextDim}};
}
QPushButton#ftbButton {
    min-width: 48px;
}
QPushButton#ftbButton:checked {
    background: {{dangerDark}};
    border-color: {{danger}};
    color: {{textStrong}};
    font-weight: 700;
}
QPushButton#transitionType {
    background: {{surfaceAlt}};
    border: 1px solid {{border}};
    border-radius: 2px;
    padding: 3px 6px;
    color: {{textSoft}};
    font-size: 10px;
}
QPushButton#transitionType:hover {
    background: {{hover}};
}
QPushButton#transitionType:checked {
    background: {{accent}};
    border-color: {{accentHover}};
    color: {{onAccent}};
    font-weight: 700;
}
QPushButton#recordButton {
    background: {{surfaceAlt}};
    border: 1px solid {{danger}};
    color: {{dangerSoft}};
    font-weight: 700;
    padding: 6px 18px;
}
QPushButton#recordButton:checked {
    background: {{danger}};
    color: {{onDanger}};
}
QPushButton#streamButton {
    background: {{surfaceAlt}};
    border: 1px solid {{info}};
    color: {{infoSoft}};
    font-weight: 700;
    padding: 6px 18px;
}
QPushButton#streamButton:checked {
    background: {{info}};
    color: {{onInfo}};
}
QPushButton#footerButton {
    background: {{raised}};
    border: 1px solid {{borderStrong}};
    border-radius: 2px;
    padding: 6px 18px;
    font-weight: 600;
}
QPushButton#footerButton:hover {
    background: {{border}};
}
QLabel#footerHint {
    color: {{textDim}};
    font-size: 10px;
}

QComboBox, QSpinBox, QLineEdit, QDoubleSpinBox {
    background: {{field}};
    border: 1px solid {{border}};
    border-radius: 2px;
    padding: 3px 6px;
    color: {{text}};
    selection-background-color: {{accent}};
    selection-color: {{onAccent}};
}
QComboBox:hover, QSpinBox:hover, QLineEdit:hover, QDoubleSpinBox:hover {
    border-color: {{borderHover}};
}
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    border: none;
    border-left: 1px solid {{border}};
    width: 18px;
    background: {{surfaceAlt}};
    border-top-right-radius: 2px;
    border-bottom-right-radius: 2px;
}
QComboBox::down-arrow {
    image: {{chevronDown}};
    width: 10px;
    height: 10px;
}
QComboBox::down-arrow:on {
    image: {{chevronUp}};
}
QSpinBox, QDoubleSpinBox {
    padding-right: 20px;
}
QSpinBox::up-button, QDoubleSpinBox::up-button {
    subcontrol-origin: border;
    subcontrol-position: top right;
    width: 17px;
    background: {{surfaceAlt}};
    border-left: 1px solid {{border}};
    border-bottom: 1px solid {{border}};
    border-top-right-radius: 2px;
}
QSpinBox::down-button, QDoubleSpinBox::down-button {
    subcontrol-origin: border;
    subcontrol-position: bottom right;
    width: 17px;
    background: {{surfaceAlt}};
    border-left: 1px solid {{border}};
    border-bottom-right-radius: 2px;
}
QSpinBox::up-button:hover, QSpinBox::down-button:hover,
QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover {
    background: {{hover}};
}
QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {
    image: {{chevronUp}};
    width: 8px;
    height: 8px;
}
QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {
    image: {{chevronDown}};
    width: 8px;
    height: 8px;
}
QComboBox QAbstractItemView {
    background: {{surface}};
    border: 1px solid {{borderStrong}};
    selection-background-color: {{accent}};
    selection-color: {{onAccent}};
    outline: 0;
}
QLineEdit:focus, QSpinBox:focus, QComboBox:focus, QDoubleSpinBox:focus {
    border-color: {{accent}};
}
QCheckBox {
    spacing: 6px;
    color: {{textSecondary}};
}
QCheckBox::indicator {
    width: 14px;
    height: 14px;
    border: 1px solid {{borderStrong}};
    border-radius: 2px;
    background: {{field}};
}
QCheckBox::indicator:hover {
    border-color: {{accent}};
}
QCheckBox::indicator:checked {
    background: {{accent}};
    border-color: {{accent}};
    image: {{checkIcon}};
}
QCheckBox::indicator:disabled {
    background: {{window}};
    border-color: {{raised}};
}

QSlider::groove:horizontal {
    height: 5px;
    background: {{fieldDeep}};
    border: 1px solid {{border}};
    border-radius: 2px;
}
QSlider::sub-page:horizontal {
    background: {{accent}};
    border-radius: 2px;
}
QSlider::handle:horizontal {
    background: {{textSecondary}};
    border: 1px solid {{textStrong}};
    width: 11px;
    margin: -5px 0;
    border-radius: 2px;
}
QSlider::handle:horizontal:hover {
    background: {{textStrong}};
}
QSlider::groove:vertical {
    width: 12px;
    background: {{fieldDeep}};
    border: 1px solid {{border}};
    border-radius: 2px;
}
QSlider::sub-page:vertical {
    background: {{fieldDeep}};
}
QSlider::add-page:vertical {
    background: {{accent}};
    border-radius: 2px;
}
QSlider::handle:vertical {
    background: {{textSecondary}};
    border: 1px solid {{textStrong}};
    height: 16px;
    margin: 0 -6px;
    border-radius: 2px;
}
QSlider#tbar::groove:vertical {
    width: 14px;
    background: {{fieldDeep}};
    border: 1px solid {{borderStrong}};
}
QSlider#tbar::add-page:vertical {
    background: {{accent}};
}
QSlider#tbar::handle:vertical {
    background: {{textStrong}};
    border: 1px solid {{accentHover}};
    height: 22px;
    margin: 0 -7px;
    border-radius: 2px;
}

QScrollBar:vertical {
    background: {{field}};
    width: 10px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: {{borderStrong}};
    min-height: 24px;
    border-radius: 4px;
    margin: 2px;
}
QScrollBar::handle:vertical:hover {
    background: {{borderHover}};
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar:horizontal {
    background: {{field}};
    height: 10px;
}
QScrollBar::handle:horizontal {
    background: {{borderStrong}};
    min-width: 24px;
    border-radius: 4px;
    margin: 2px;
}
QScrollBar::handle:horizontal:hover {
    background: {{borderHover}};
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}

QStatusBar {
    background: {{fieldDeep}};
    border-top: 1px solid {{surfaceAlt}};
}
QStatusBar QLabel {
    color: {{textMuted}};
    font-size: 10px;
    padding: 0 8px;
}
QLabel#statusOnAir {
    color: {{textMuted}};
    font-weight: 700;
}
QLabel#statusOnAir[live="true"] {
    color: {{dangerSoft}};
}

QFrame#monitorFrame {
    background: {{monitor}};
    border: 1px solid {{border}};
    border-radius: 1px;
}
QFrame#monitorFrame[role="program"] {
    border-color: {{program}};
}
QFrame#monitorFrame[role="preview"] {
    border-color: {{accent}};
}

QFrame#transitionBar {
    background: {{surface}};
    border: 1px solid {{border}};
    border-radius: 2px;
}
QLabel#controlLabel {
    color: {{textMuted}};
    font-size: 9px;
    font-weight: 700;
    letter-spacing: 1px;
}
QLabel#mixReadout {
    font-family: "Consolas";
    color: {{infoSoft}};
    font-size: 11px;
}

QFrame#mixerPanel {
    background: {{surface}};
    border-left: 1px solid {{border}};
}
QFrame#channelStrip {
    background: {{field}};
    border: 1px solid {{raised}};
    border-radius: 2px;
}
QLabel#channelName {
    color: {{textSecondary}};
    font-size: 10px;
}
QLabel#channelDb {
    font-family: "Consolas";
    color: {{textMuted}};
    font-size: 9px;
}
QToolButton#muteButton {
    background: {{surfaceAlt}};
    border: 1px solid {{borderStrong}};
    border-radius: 2px;
    color: {{textMuted}};
    font-size: 9px;
    font-weight: 700;
    padding: 0;
}
QToolButton#muteButton:checked {
    background: {{dangerDark}};
    border-color: {{danger}};
    color: {{textStrong}};
}

QFrame#inputBar {
    background: {{surface}};
    border-top: 1px solid {{border}};
    border-bottom: 1px solid {{border}};
}
QLabel#inputCount {
    color: {{textMuted}};
    font-size: 10px;
}
QLineEdit#inputSearch {
    background: {{field}};
    border: 1px solid {{border}};
    border-radius: 2px;
    padding: 3px 6px;
    font-size: 10px;
}
QToolButton#filterChip {
    background: {{surfaceAlt}};
    border: 1px solid {{border}};
    border-radius: 2px;
    padding: 3px 9px;
    color: {{textSoft}};
    font-size: 10px;
}
QToolButton#filterChip:hover {
    background: {{hover}};
}
QToolButton#filterChip:checked {
    background: {{accentSoft}};
    border-color: {{accent}};
    color: {{accentHover}};
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
    background: {{field}};
    border: 1px solid {{raised}};
    border-radius: 2px;
}
QFrame#inputTile:hover {
    border-color: {{borderHover}};
}
QFrame#inputTile[selected="true"] {
    border: 1px solid {{accent}};
}
QLabel#inputTileHeader {
    color: {{textStrong}};
    font-size: 10px;
    font-weight: 700;
    background: {{raised}};
    padding-left: 5px;
}
QLabel#inputTileHeader[role="program"] {
    background: {{program}};
    color: {{onProgram}};
}
QLabel#inputTileHeader[role="preview"] {
    background: {{preview}};
    color: {{onPreview}};
}
QLabel#inputTileThumb {
    background: {{black}};
    border: none;
}
QToolButton#tileAction {
    background: {{surfaceAlt}};
    border: 1px solid {{border}};
    border-radius: 2px;
    color: {{textSecondary}};
    font-size: 9px;
    font-weight: 700;
    padding: 0;
}
QToolButton#tileAction:hover {
    background: {{hover}};
    border-color: {{accent}};
    color: {{textStrong}};
}
QLabel#inputEmpty {
    color: {{textDim}};
    font-size: 11px;
    padding: 40px;
}

QLabel#pillRec {
    background: {{surfaceAlt}};
    border: 1px solid {{border}};
    border-radius: 8px;
    padding: 2px 8px;
    color: {{textDim}};
    font-size: 9px;
    font-weight: 700;
}
QLabel#pillRec[live="true"] {
    background: {{dangerDark}};
    border-color: {{danger}};
    color: {{textStrong}};
}
QLabel#pillStream {
    background: {{surfaceAlt}};
    border: 1px solid {{border}};
    border-radius: 8px;
    padding: 2px 8px;
    color: {{textDim}};
    font-size: 9px;
    font-weight: 700;
}
QLabel#pillStream[live="true"] {
    background: {{info}};
    border-color: {{infoHover}};
    color: {{textStrong}};
}

QFrame#footerBar {
    background: {{surface}};
    border-top: 1px solid {{border}};
}

QFrame#mixerPanel {
    background: {{window}};
    border-left: 1px solid {{border}};
}
QLabel#audioDeviceChip {
    background: {{surfaceAlt}};
    border: 1px solid {{border}};
    border-radius: 7px;
    padding: 1px 8px;
    color: {{textMuted}};
    font-size: 9px;
    font-weight: 700;
}
QLabel#audioDeviceChip[active="true"] {
    border-color: {{success}};
    color: {{success}};
}
QFrame#channelStrip {
    background: {{surface}};
    border: 1px solid {{border}};
    border-radius: 3px;
}
QFrame#channelStrip[onAir="true"] {
    border-color: {{program}};
}
QFrame#channelStrip[dimmed="true"] {
    background: {{window}};
    border-color: {{surfaceAlt}};
}
QFrame#channelStrip[dimmed="true"] QLabel {
    color: {{textDim}};
}
QFrame#masterStrip {
    background: {{surfaceAlt}};
    border: 1px solid {{borderStrong}};
    border-radius: 3px;
}
QLabel#channelIndex {
    color: {{textDim}};
    font-family: "Consolas";
    font-size: 9px;
}
QLabel#channelName {
    color: {{textSecondary}};
    font-size: 10px;
    font-weight: 600;
}
QLabel#channelDb {
    font-family: "Consolas";
    color: {{textMuted}};
    font-size: 9px;
}
QLabel#meterScale {
    color: {{textDim}};
    font-family: "Consolas";
    font-size: 8px;
}
QToolButton#muteButton, QToolButton#soloButton {
    background: {{surfaceAlt}};
    border: 1px solid {{borderStrong}};
    border-radius: 2px;
    color: {{textSecondary}};
    font-size: 9px;
    font-weight: 700;
    padding: 0;
}
QToolButton#muteButton:hover, QToolButton#soloButton:hover {
    background: {{hover}};
    color: {{text}};
}
QToolButton#muteButton:checked {
    background: {{dangerDark}};
    border-color: {{danger}};
    color: {{onDanger}};
}
QToolButton#soloButton:checked {
    background: {{warning}};
    border-color: {{warning}};
    color: {{black}};
}
QSlider#channelFader::groove:vertical, QSlider#masterFader::groove:vertical {
    width: 8px;
    background: {{fieldDeep}};
    border: 1px solid {{borderStrong}};
    border-radius: 3px;
}
QSlider#channelFader::add-page:vertical, QSlider#masterFader::add-page:vertical {
    background: {{accent}};
    border-radius: 3px;
}
QSlider#channelFader::sub-page:vertical, QSlider#masterFader::sub-page:vertical {
    background: {{fieldDeep}};
}
QSlider#channelFader::handle:vertical, QSlider#masterFader::handle:vertical {
    background: {{textSecondary}};
    border: 1px solid {{textStrong}};
    height: 14px;
    margin: 0 -5px;
    border-radius: 2px;
}
QLabel#mixerHint {
    color: {{textMuted}};
    font-size: 9px;
}

QDialog#settingsDialog {
    background: {{window}};
}
QLabel#settingsTitle {
    font-family: "Bahnschrift";
    font-size: 18px;
    font-weight: 700;
    color: {{textStrong}};
}
QLabel#settingsSubtitle {
    color: {{textMuted}};
    font-size: 11px;
}
QListWidget#settingsNav {
    background: {{surface}};
    border: 1px solid {{border}};
    border-radius: 3px;
    outline: 0;
    padding: 4px;
}
QListWidget#settingsNav::item {
    padding: 9px 12px;
    border-radius: 3px;
    color: {{textSecondary}};
}
QListWidget#settingsNav::item:hover {
    background: {{hover}};
}
QListWidget#settingsNav::item:selected {
    background: {{selection}};
    color: {{textStrong}};
    border-left: 2px solid {{accent}};
}
QStackedWidget#settingsPages {
    background: transparent;
}
QLabel#settingsPageTitle {
    font-size: 15px;
    font-weight: 700;
    color: {{textStrong}};
}
QLabel#settingsPageHint {
    color: {{textMuted}};
    font-size: 11px;
}
QFrame#settingsCard {
    background: {{surface}};
    border: 1px solid {{border}};
    border-radius: 3px;
}
QLabel#cardTitle {
    color: {{textMuted}};
    font-size: 9px;
    font-weight: 700;
    letter-spacing: 1px;
}
QPushButton#primaryButton {
    background: {{accent}};
    border: 1px solid {{accentHover}};
    color: {{onAccent}};
    font-weight: 700;
    padding: 6px 18px;
}
QPushButton#primaryButton:hover {
    background: {{accentHover}};
}
QPushButton#ghostButton {
    background: transparent;
    border: 1px solid {{border}};
    color: {{textSecondary}};
    padding: 6px 14px;
}
QPushButton#ghostButton:hover {
    background: {{hover}};
    border-color: {{borderHover}};
}
QFrame#themeCard {
    background: {{surface}};
    border: 1px solid {{border}};
    border-radius: 3px;
}
QFrame#themeCard:hover {
    border-color: {{borderHover}};
}
QFrame#themeCard[selected="true"] {
    border: 2px solid {{accent}};
}
QLabel#themeName {
    color: {{textStrong}};
    font-weight: 700;
}
QLabel#themeHint {
    color: {{textMuted}};
    font-size: 10px;
}
QLabel#aboutKey {
    color: {{textMuted}};
    font-size: 10px;
}
QLabel#aboutValue {
    color: {{textSecondary}};
    font-size: 11px;
}
QLabel#shortcutKey {
    font-family: "Consolas";
    background: {{field}};
    border: 1px solid {{border}};
    border-radius: 2px;
    padding: 2px 7px;
    color: {{textSecondary}};
    font-size: 10px;
}
QFrame#settingsFooter {
    background: {{surface}};
    border-top: 1px solid {{border}};
}
)QSS";

} // namespace mixvera::ui::theme
