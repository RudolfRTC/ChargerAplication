#pragma once

#include <QGroupBox>
#include <QPlainTextEdit>
#include <QPushButton>

class LogPanel : public QGroupBox {
    Q_OBJECT
public:
    explicit LogPanel(QWidget* parent = nullptr);

public slots:
    void append(const QString& text);

private slots:
    void saveLog();

private:
    QPlainTextEdit* m_textEdit;
    QPushButton*    m_saveBtn;
    QPushButton*    m_clearBtn;
};
