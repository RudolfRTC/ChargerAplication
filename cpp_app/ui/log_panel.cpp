#include "ui/log_panel.h"

#include <QDateTime>
#include <QFileDialog>
#include <QFile>
#include <QHBoxLayout>
#include <QVBoxLayout>

LogPanel::LogPanel(QWidget* parent)
    : QGroupBox("Log", parent)
{
    auto* layout = new QVBoxLayout(this);

    m_textEdit = new QPlainTextEdit;
    m_textEdit->setReadOnly(true);
    m_textEdit->setMaximumBlockCount(5000);
    layout->addWidget(m_textEdit);

    auto* btnRow = new QHBoxLayout;
    m_saveBtn = new QPushButton("Save Log");
    m_clearBtn = new QPushButton("Clear");
    btnRow->addWidget(m_saveBtn);
    btnRow->addWidget(m_clearBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    connect(m_saveBtn, &QPushButton::clicked, this, &LogPanel::saveLog);
    connect(m_clearBtn, &QPushButton::clicked, m_textEdit, &QPlainTextEdit::clear);
}

void LogPanel::append(const QString& text) {
    QString ts = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_textEdit->appendPlainText(QString("[%1] %2").arg(ts, text));
}

void LogPanel::saveLog() {
    QString path = QFileDialog::getSaveFileName(
        this, "Save Log",
        QString("obc_log_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "Text files (*.txt);;All files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(m_textEdit->toPlainText().toUtf8());
        file.close();
    }
}
