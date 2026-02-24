#include "ui/Theme.h"

#include <QApplication>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QPalette>
#include <QStyleHints>

namespace Theme {

bool isDark()
{
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

void carregarFontes()
{
    QFontDatabase::addApplicationFont(":/fonts/Inter-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-Medium.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-SemiBold.ttf");
}

QString stylesheet(bool dark)
{
    // ── Paleta ────────────────────────────────────────────────────────────────
    const QString bg        = dark ? "#18181A" : "#F5F4F0";
    const QString surface   = dark ? "#222225" : "#FFFFFF";
    const QString border    = dark ? "#32323A" : "#E0DED8";
    const QString textPri   = dark ? "#F0EFED" : "#18181A";
    const QString textSec   = dark ? "#9A9895" : "#6E6D6A";
    const QString accent    = dark ? "#22A367" : "#166F4A";
    const QString accentHov = dark ? "#1D8F5A" : "#105C3D";
    const QString hover     = dark ? "#2C2C30" : "#EEEDE8";
    const QString inputBg   = dark ? "#2A2A2E" : "#FFFFFF";
    const QString negative  = dark ? "#E85555" : "#C94040";
    const QString scrollBg  = dark ? "#2A2A2E" : "#E8E6E0";
    const QString scrollHan = dark ? "#4A4A52" : "#C0BDB6";

    return QString(R"(
/* ── Base ─────────────────────────────────────────────────────────────── */
QWidget {
    background-color: %1;
    color: %3;
    font-family: "Inter";
    font-size: 13px;
    font-weight: 400;
}

QMainWindow, QDialog {
    background-color: %1;
}

/* ── Tabs ──────────────────────────────────────────────────────────────── */
QTabWidget::pane {
    border: 1px solid %5;
    border-top: none;
    background-color: %2;
    border-radius: 0px 0px 8px 8px;
}

QTabBar::tab {
    background-color: transparent;
    color: %4;
    padding: 9px 20px;
    font-size: 13px;
    font-weight: 500;
    border-bottom: 2px solid transparent;
    margin-right: 2px;
}

QTabBar::tab:selected {
    color: %6;
    border-bottom: 2px solid %6;
}

QTabBar::tab:hover:!selected {
    color: %3;
    background-color: %8;
}

/* ── Botão Primário ────────────────────────────────────────────────────── */
QPushButton {
    background-color: %6;
    color: #FFFFFF;
    border: none;
    border-radius: 6px;
    padding: 7px 16px;
    font-size: 13px;
    font-weight: 500;
    min-height: 32px;
}

QPushButton:hover {
    background-color: %7;
}

QPushButton:pressed {
    background-color: %7;
    padding-top: 8px;
    padding-bottom: 6px;
}

QPushButton:disabled {
    background-color: %5;
    color: %4;
}

/* Botão secundário (destrutivo) — aplicar via setProperty("secondary", true) */
QPushButton[secondary="true"] {
    background-color: transparent;
    color: %3;
    border: 1px solid %5;
}

QPushButton[secondary="true"]:hover {
    background-color: %8;
}

/* ── Inputs ────────────────────────────────────────────────────────────── */
QLineEdit, QComboBox {
    background-color: %9;
    color: %3;
    border: 1px solid %5;
    border-radius: 6px;
    padding: 6px 10px;
    font-size: 13px;
    min-height: 32px;
    selection-background-color: %6;
}

QLineEdit:focus, QComboBox:focus {
    border: 1px solid %6;
}

QLineEdit:disabled, QComboBox:disabled {
    color: %4;
}

QComboBox::drop-down {
    border: none;
    width: 20px;
}

QComboBox::down-arrow {
    width: 10px;
    height: 10px;
}

QComboBox QAbstractItemView {
    background-color: %2;
    border: 1px solid %5;
    border-radius: 6px;
    selection-background-color: %8;
    selection-color: %3;
    padding: 4px;
}

/* ── Tabela ────────────────────────────────────────────────────────────── */
QTableWidget {
    background-color: %2;
    alternate-background-color: %8;
    color: %3;
    border: 1px solid %5;
    border-radius: 8px;
    gridline-color: %5;
    font-size: 13px;
}

QTableWidget::item {
    padding: 0px 10px;
    min-height: 36px;
    border: none;
}

QTableWidget::item:selected {
    background-color: %6;
    color: #FFFFFF;
}

QTableWidget::item:hover:!selected {
    background-color: %8;
}

QHeaderView::section {
    background-color: %2;
    color: %4;
    font-size: 12px;
    font-weight: 500;
    padding: 8px 10px;
    border: none;
    border-bottom: 1px solid %5;
}

QHeaderView::section:first {
    border-top-left-radius: 8px;
}

QHeaderView::section:last {
    border-top-right-radius: 8px;
}

/* ── Lista ─────────────────────────────────────────────────────────────── */
QListWidget {
    background-color: %2;
    border: 1px solid %5;
    border-radius: 8px;
    font-size: 13px;
    outline: none;
}

QListWidget::item {
    padding: 8px 12px;
    border-radius: 4px;
    margin: 2px 4px;
}

QListWidget::item:selected {
    background-color: %6;
    color: #FFFFFF;
}

QListWidget::item:hover:!selected {
    background-color: %8;
}

/* ── Labels ────────────────────────────────────────────────────────────── */
QLabel {
    background-color: transparent;
    color: %3;
}

QLabel[secondary="true"] {
    color: %4;
    font-size: 12px;
}

/* ── Scrollbars ────────────────────────────────────────────────────────── */
QScrollBar:vertical {
    background-color: %10;
    width: 8px;
    border-radius: 4px;
    margin: 0;
}

QScrollBar::handle:vertical {
    background-color: %11;
    border-radius: 4px;
    min-height: 32px;
}

QScrollBar::handle:vertical:hover {
    background-color: %6;
}

QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }

QScrollBar:horizontal {
    background-color: %10;
    height: 8px;
    border-radius: 4px;
    margin: 0;
}

QScrollBar::handle:horizontal {
    background-color: %11;
    border-radius: 4px;
    min-width: 32px;
}

QScrollBar::handle:horizontal:hover {
    background-color: %6;
}

QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

/* ── MessageBox ────────────────────────────────────────────────────────── */
QMessageBox {
    background-color: %2;
}

QMessageBox QLabel {
    font-size: 13px;
}

/* ── CheckBox ──────────────────────────────────────────────────────────── */
QCheckBox {
    spacing: 8px;
    font-size: 13px;
}

QCheckBox::indicator {
    width: 16px;
    height: 16px;
    border: 1px solid %5;
    border-radius: 4px;
    background-color: %9;
}

QCheckBox::indicator:checked {
    background-color: %6;
    border-color: %6;
}
)")
    .arg(bg)        // %1
    .arg(surface)   // %2
    .arg(textPri)   // %3
    .arg(textSec)   // %4
    .arg(border)    // %5
    .arg(accent)    // %6
    .arg(accentHov) // %7
    .arg(hover)     // %8
    .arg(inputBg)   // %9
    .arg(scrollBg)  // %10
    .arg(scrollHan); // %11
}

void aplicar()
{
    carregarFontes();

    QFont fontePadrao("Inter", 13);
    QApplication::setFont(fontePadrao);

    qApp->setStyleSheet(stylesheet(isDark()));

    // Reaplica automaticamente se o sistema trocar de tema
    QObject::connect(
        QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
        qApp, [](Qt::ColorScheme) {
            qApp->setStyleSheet(stylesheet(isDark()));
        }
    );
}

} // namespace Theme
