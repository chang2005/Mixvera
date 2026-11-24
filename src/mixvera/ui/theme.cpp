#include "mixvera/ui/theme.hpp"

namespace mixvera::ui::theme {

QString applicationStyleSheet()
{
    return QStringLiteral(R"QSS(
QWidget {
    color: #dbe6f2;
    font-family: "Segoe UI";
    font-size: 12px;
}

QMainWindow, QWidget#root {
    background: #06090e;
}

QFrame#appHeader {
    background: #0a0f16;
    border: 1px solid #18222e;
    border-radius: 10px;
}

QLabel#brandMark {
    background: #53c8ff;
    color: #041018;
    font-family: "Bahnschrift";
    font-size: 19px;
    font-weight: bold;
    border-radius: 8px;
}

QLabel#brandTitle {
    color: #f2f7ff;
    font-family: "Bahnschrift";
    font-size: 21px;
    font-weight: bold;
}

QLabel#brandSubtitle {
    color: #6f8298;
    font-family: "Bahnschrift";
    font-size: 9px;
}

QLabel#pill {
    background: #101923;
    border: 1px solid #223140;
    border-radius: 9px;
    color: #9db0c4;
    font-family: "Bahnschrift";
    font-size: 10px;
    padding: 4px 10px;
}

QLabel#onAirPill {
    background: #2b1116;
    border: 1px solid #6d2630;
    border-radius: 9px;
    color: #ff8a95;
    font-family: "Bahnschrift";
    font-size: 10px;
    font-weight: bold;
    padding: 4px 11px;
}

QLabel#clock {
    color: #e7f0fa;
    font-family: "Consolas";
    font-size: 14px;
}

QFrame#monitorCard {
    background: #0a0f16;
    border: 1px solid #1b2634;
    border-radius: 10px;
}

QFrame#monitorCard[accent="program"] {
    border-top: 2px solid #ff4d5e;
}

QFrame#monitorCard[accent="preview"] {
    border-top: 2px solid #38d996;
}

QFrame#accentBar[accent="program"] {
    background: #ff4d5e;
    border-radius: 1px;
}

QFrame#accentBar[accent="preview"] {
    background: #38d996;
    border-radius: 1px;
}

QLabel#monitorTitle {
    color: #e9f2fb;
    font-family: "Bahnschrift";
    font-size: 11px;
    font-weight: bold;
}

QLabel#monitorSource {
    color: #7d91a6;
    font-family: "Bahnschrift";
    font-size: 10px;
}

QLabel#monitorStatus {
    font-family: "Bahnschrift";
    font-size: 9px;
    font-weight: bold;
}

QLabel#monitorStatus[accent="program"] {
    color: #ff7a86;
}

QLabel#monitorStatus[accent="preview"] {
    color: #62e3a9;
}

QLabel#monitorMeta {
    color: #64788d;
    font-family: "Consolas";
    font-size: 9px;
}

QFrame#transitionCard {
    background: #0b1119;
    border: 1px solid #1b2634;
    border-radius: 10px;
}

QLabel#controlLabel {
    color: #72869b;
    font-family: "Bahnschrift";
    font-size: 9px;
}

QLabel#mixReadout {
    color: #8ee7ff;
    font-family: "Consolas";
    font-size: 13px;
}

QPushButton {
    background: #141d28;
    border: 1px solid #273545;
    border-radius: 5px;
    color: #dbe6f2;
    font-family: "Bahnschrift";
    font-size: 11px;
    font-weight: bold;
    padding: 7px 14px;
}

QPushButton:hover {
    background: #1b2836;
    border-color: #3a4d63;
}

QPushButton:pressed {
    background: #0e1620;
}

QPushButton#cutButton {
    background: #ff4d5e;
    border: 1px solid #ff6b79;
    color: #180609;
    padding: 9px 30px;
}

QPushButton#cutButton:hover {
    background: #ff6574;
}

QPushButton#autoButton {
    background: #123a4c;
    border: 1px solid #1f6280;
    color: #8ee7ff;
    padding: 9px 30px;
}

QPushButton#autoButton:hover {
    background: #17495f;
    border-color: #2a82a8;
}

QPushButton#autoButton:disabled {
    background: #0d2029;
    color: #4f6c7b;
    border-color: #1a323e;
}

QComboBox, QSpinBox {
    background: #0f1721;
    border: 1px solid #253444;
    border-radius: 4px;
    color: #dbe6f2;
    font-family: "Bahnschrift";
    font-size: 10px;
    padding: 5px 8px;
    min-height: 18px;
}

QComboBox:hover, QSpinBox:hover {
    border-color: #3a4d63;
}

QComboBox QAbstractItemView {
    background: #0f1721;
    border: 1px solid #2a3a4b;
    color: #dbe6f2;
    selection-background-color: #1d3848;
    outline: none;
}

QSlider#tbar::groove:horizontal {
    height: 8px;
    background: #0d151e;
    border: 1px solid #22303f;
    border-radius: 4px;
}

QSlider#tbar::sub-page:horizontal {
    background: #1e4c62;
    border-radius: 4px;
}

QSlider#tbar::handle:horizontal {
    background: #53c8ff;
    border: 1px solid #9ae8ff;
    width: 20px;
    margin: -7px 0;
    border-radius: 4px;
}

QSlider#tbar::handle:horizontal:hover {
    background: #7ad9ff;
}

QFrame#sideCard {
    background: #0b1119;
    border: 1px solid #1b2634;
    border-radius: 10px;
}

QLabel#cardTitle {
    color: #a9bbcd;
    font-family: "Bahnschrift";
    font-size: 10px;
    font-weight: bold;
}

QLabel#cardHint {
    color: #5d7186;
    font-family: "Consolas";
    font-size: 9px;
}

QFrame#multiviewTile {
    background: #080c12;
    border: 1px solid #1a2431;
    border-radius: 6px;
}

QFrame#multiviewTile[state="program"] {
    border: 2px solid #ff4d5e;
    background: #120a0d;
}

QFrame#multiviewTile[state="preview"] {
    border: 2px solid #38d996;
    background: #08120e;
}

QLabel#tileCaption {
    color: #93a6ba;
    font-family: "Bahnschrift";
    font-size: 9px;
}

QFrame#outputRow {
    background: #0a1017;
    border: 1px solid #18222e;
    border-radius: 6px;
}

QLabel#outputName {
    color: #d7e3f0;
    font-family: "Bahnschrift";
    font-size: 11px;
}

QLabel#outputId {
    color: #5d7186;
    font-family: "Consolas";
    font-size: 8px;
}

QLabel#outputState {
    color: #6e8297;
    font-family: "Bahnschrift";
    font-size: 9px;
}

QLabel#statusDot {
    background: #344252;
    border-radius: 4px;
}

QLabel#statusDot[running="true"] {
    background: #38d996;
}

QLabel#statusDot[recording="true"] {
    background: #ff4d5e;
}

QPushButton#outputButton {
    font-size: 9px;
    padding: 5px 12px;
}

QPushButton#outputButton[running="true"] {
    background: #251116;
    border-color: #5d2630;
    color: #ff9aa3;
}

QPushButton#recButton {
    background: #1c1114;
    border-color: #4c222a;
    color: #ff9aa3;
    padding: 6px 14px;
}

QPushButton#recButton:checked {
    background: #ff4d5e;
    border-color: #ff7a86;
    color: #180609;
}

QPushButton#saveReplayButton {
    background: #102b38;
    border-color: #1f566c;
    color: #8ee7ff;
    padding: 6px 14px;
}

QFrame#divider {
    background: #1a2431;
    border: none;
}

QLabel#replayState {
    color: #8ee7ff;
    font-family: "Consolas";
    font-size: 9px;
}

QFrame#mixerStrip, QFrame#masterStrip {
    background: #0c1219;
    border: 1px solid #1b2532;
    border-radius: 8px;
}

QFrame#masterStrip {
    background: #0d151f;
    border-color: #243545;
}

QLabel#mixerName {
    color: #c7d5e4;
    font-family: "Bahnschrift";
    font-size: 10px;
    font-weight: bold;
}

QLabel#mixerGain {
    color: #7f93a8;
    font-family: "Consolas";
    font-size: 9px;
}

QLabel#busBadge {
    color: #53c8ff;
    font-family: "Consolas";
    font-size: 9px;
}

QSlider#mixerFader::groove:vertical, QSlider#masterFader::groove:vertical {
    width: 4px;
    background: #18222e;
    border-radius: 2px;
}

QSlider#mixerFader::sub-page:vertical, QSlider#masterFader::sub-page:vertical {
    background: #14232c;
    border-radius: 2px;
}

QSlider#mixerFader::add-page:vertical, QSlider#masterFader::add-page:vertical {
    background: #2b6f8b;
    border-radius: 2px;
}

QSlider#mixerFader::handle:vertical, QSlider#masterFader::handle:vertical {
    background: #53c8ff;
    border: 1px solid #9ae8ff;
    height: 16px;
    margin: 0 -8px;
    border-radius: 3px;
}

QSlider#masterFader::handle:vertical {
    background: #38d996;
    border-color: #8cf0c5;
}

QPushButton#muteButton {
    font-size: 9px;
    padding: 4px 6px;
}

QPushButton#muteButton:checked {
    background: #5a1f28;
    border-color: #8a2d3a;
    color: #ffb3ba;
}

QFrame#routingPanel {
    background: #0a0f16;
    border: 1px solid #1a2431;
    border-radius: 8px;
}

QLabel#routeHeader {
    color: #66798e;
    font-family: "Bahnschrift";
    font-size: 8px;
}

QLabel#routeSource {
    color: #aebfd0;
    font-family: "Bahnschrift";
    font-size: 9px;
}

QPushButton#routeButton {
    background: #0e1620;
    border: 1px solid #22303f;
    border-radius: 3px;
    padding: 0;
    min-height: 16px;
}

QPushButton#routeButton:hover {
    border-color: #3a4d63;
}

QPushButton#routeButton:checked {
    background: #1d4a5f;
    border: 1px solid #53c8ff;
}

QStatusBar {
    background: #070b10;
    border-top: 1px solid #161f2a;
    color: #6f8298;
}

QStatusBar::item {
    border: none;
}

QStatusBar QLabel {
    color: #6f8298;
    font-family: "Consolas";
    font-size: 9px;
    padding: 0 8px;
}

QToolTip {
    background: #111a24;
    color: #dbe6f2;
    border: 1px solid #2a3a4b;
    padding: 4px;
}
)QSS");
}

} // namespace mixvera::ui::theme
