#include "ui/graph_panel.h"
#include "ui/theme.h"

#include <QDateTime>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>
#include <chrono>
#include <cmath>
#include <algorithm>

static const QMap<QString, int> WINDOW_OPTIONS = {
    {"1 min",  60},
    {"5 min",  300},
    {"10 min", 600},
    {"30 min", 1800},
};

static double monoNowGraph() {
    return std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

GraphPanel::GraphPanel(QWidget* parent)
    : QGroupBox("Live Graph", parent)
{
    m_t0 = monoNowGraph();

    auto* layout = new QVBoxLayout(this);

    // Controls row
    auto* ctrlRow = new QHBoxLayout;
    m_pauseBtn = new QPushButton("Pause");
    m_pauseBtn->setCheckable(true);
    connect(m_pauseBtn, &QPushButton::toggled, this, &GraphPanel::onPauseToggled);
    ctrlRow->addWidget(m_pauseBtn);

    m_clearBtn = new QPushButton("Clear");
    connect(m_clearBtn, &QPushButton::clicked, this, &GraphPanel::clearData);
    ctrlRow->addWidget(m_clearBtn);

    ctrlRow->addWidget(new QLabel("Window:"));
    m_windowCombo = new QComboBox;
    m_windowCombo->addItems({"1 min", "5 min", "10 min", "30 min"});
    m_windowCombo->setCurrentText("10 min");
    connect(m_windowCombo, &QComboBox::currentTextChanged, this, &GraphPanel::onWindowChanged);
    ctrlRow->addWidget(m_windowCombo);

    m_exportBtn = new QPushButton("Export CSV");
    connect(m_exportBtn, &QPushButton::clicked, this, &GraphPanel::exportCsv);
    ctrlRow->addWidget(m_exportBtn);
    ctrlRow->addStretch();
    layout->addLayout(ctrlRow);

    // Voltage chart
    m_voltChart = new QChart;
    setupChart(m_voltChart, "Voltage (V)");
    m_voutSeries = new QLineSeries;
    m_voutSeries->setName("Vout");
    m_voutSeries->setColor(QColor(Theme::CYAN));
    m_vinSeries = new QLineSeries;
    m_vinSeries->setName("Vin");
    m_vinSeries->setColor(QColor(Theme::MAGENTA));
    m_voltChart->addSeries(m_voutSeries);
    m_voltChart->addSeries(m_vinSeries);

    m_voltXAxis = new QValueAxis;
    m_voltXAxis->setTitleText("s");
    m_voltXAxis->setLabelsColor(QColor(Theme::TEXT_DIM));
    m_voltXAxis->setGridLineColor(QColor(Theme::BORDER));
    m_voltYAxis = new QValueAxis;
    m_voltYAxis->setTitleText("V");
    m_voltYAxis->setLabelsColor(QColor(Theme::TEXT_DIM));
    m_voltYAxis->setGridLineColor(QColor(Theme::BORDER));
    m_voltChart->addAxis(m_voltXAxis, Qt::AlignBottom);
    m_voltChart->addAxis(m_voltYAxis, Qt::AlignLeft);
    m_voutSeries->attachAxis(m_voltXAxis);
    m_voutSeries->attachAxis(m_voltYAxis);
    m_vinSeries->attachAxis(m_voltXAxis);
    m_vinSeries->attachAxis(m_voltYAxis);

    m_voltView = new QChartView(m_voltChart);
    m_voltView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(m_voltView, 1);

    // Current chart
    m_currChart = new QChart;
    setupChart(m_currChart, "Current (A)");
    m_ioutSeries = new QLineSeries;
    m_ioutSeries->setName("Iout");
    m_ioutSeries->setColor(QColor(Theme::GREEN));
    m_currChart->addSeries(m_ioutSeries);

    m_currXAxis = new QValueAxis;
    m_currXAxis->setTitleText("s");
    m_currXAxis->setLabelsColor(QColor(Theme::TEXT_DIM));
    m_currXAxis->setGridLineColor(QColor(Theme::BORDER));
    m_currYAxis = new QValueAxis;
    m_currYAxis->setTitleText("A");
    m_currYAxis->setLabelsColor(QColor(Theme::TEXT_DIM));
    m_currYAxis->setGridLineColor(QColor(Theme::BORDER));
    m_currChart->addAxis(m_currXAxis, Qt::AlignBottom);
    m_currChart->addAxis(m_currYAxis, Qt::AlignLeft);
    m_ioutSeries->attachAxis(m_currXAxis);
    m_ioutSeries->attachAxis(m_currYAxis);

    m_currView = new QChartView(m_currChart);
    m_currView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(m_currView, 1);

    // Temperature chart
    m_tempChart = new QChart;
    setupChart(m_tempChart, QString::fromUtf8("Temperature (\u00b0C)"));
    m_tempSeries = new QLineSeries;
    m_tempSeries->setName("Temp");
    m_tempSeries->setColor(QColor(Theme::ORANGE));
    m_tempChart->addSeries(m_tempSeries);

    m_tempXAxis = new QValueAxis;
    m_tempXAxis->setTitleText("s");
    m_tempXAxis->setLabelsColor(QColor(Theme::TEXT_DIM));
    m_tempXAxis->setGridLineColor(QColor(Theme::BORDER));
    m_tempYAxis = new QValueAxis;
    m_tempYAxis->setTitleText(QString::fromUtf8("\u00b0C"));
    m_tempYAxis->setLabelsColor(QColor(Theme::TEXT_DIM));
    m_tempYAxis->setGridLineColor(QColor(Theme::BORDER));
    m_tempChart->addAxis(m_tempXAxis, Qt::AlignBottom);
    m_tempChart->addAxis(m_tempYAxis, Qt::AlignLeft);
    m_tempSeries->attachAxis(m_tempXAxis);
    m_tempSeries->attachAxis(m_tempYAxis);

    m_tempView = new QChartView(m_tempChart);
    m_tempView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(m_tempView, 1);

    // Redraw timer (~15 Hz)
    m_redrawTimer = new QTimer(this);
    connect(m_redrawTimer, &QTimer::timeout, this, &GraphPanel::redraw);
    m_redrawTimer->start(66);
}

void GraphPanel::setupChart(QChart* chart, const QString& title) {
    chart->setTitle(title);
    chart->setTitleBrush(QBrush(QColor(Theme::TEXT_DIM)));
    chart->setBackgroundBrush(QBrush(QColor(Theme::BG_DEEP)));
    chart->setPlotAreaBackgroundBrush(QBrush(QColor("#0d1117")));
    chart->setPlotAreaBackgroundVisible(true);
    chart->legend()->setLabelColor(QColor(Theme::TEXT_DIM));
    chart->legend()->setAlignment(Qt::AlignTop);
    chart->setMargins(QMargins(4, 4, 4, 4));
}

void GraphPanel::addPoint(const Message2& msg) {
    double t = monoNowGraph() - m_t0;
    m_ts.push_back(t);
    m_vout.push_back(msg.output_voltage);
    m_vin.push_back(msg.input_voltage);
    m_iout.push_back(msg.output_current);
    m_temp.push_back(msg.temperature);
    m_status.push_back(msg.status.toByte());

    // Enforce max size
    while (m_ts.size() > MAX_POINTS) {
        m_ts.pop_front();
        m_vout.pop_front();
        m_vin.pop_front();
        m_iout.pop_front();
        m_temp.pop_front();
        m_status.pop_front();
    }
    m_dataDirty = true;
}

void GraphPanel::addEventMarker(const QString& label, const QString& severity) {
    double t = monoNowGraph() - m_t0;
    m_markerData.emplace_back(t, label, severity);
}

void GraphPanel::redraw() {
    if (m_paused || m_ts.empty() || !m_dataDirty)
        return;
    m_dataDirty = false;

    double now = m_ts.back();
    double cutoff = now - m_windowSec;

    // Find start index
    size_t startIdx = 0;
    for (size_t i = 0; i < m_ts.size(); ++i) {
        if (m_ts[i] >= cutoff) { startIdx = i; break; }
    }

    // Build point lists
    QList<QPointF> voutPts, vinPts, ioutPts, tempPts;
    double vMin = 1e9, vMax = -1e9;
    double iMin = 1e9, iMax = -1e9;
    double tMin = 1e9, tMax = -1e9;

    for (size_t i = startIdx; i < m_ts.size(); ++i) {
        double t = m_ts[i];
        voutPts.append(QPointF(t, m_vout[i]));
        vinPts.append(QPointF(t, m_vin[i]));
        ioutPts.append(QPointF(t, m_iout[i]));
        tempPts.append(QPointF(t, m_temp[i]));

        double vmax_i = std::max(m_vout[i], m_vin[i]);
        double vmin_i = std::min(m_vout[i], m_vin[i]);
        vMin = std::min(vMin, vmin_i);
        vMax = std::max(vMax, vmax_i);
        iMin = std::min(iMin, m_iout[i]);
        iMax = std::max(iMax, m_iout[i]);
        tMin = std::min(tMin, m_temp[i]);
        tMax = std::max(tMax, m_temp[i]);
    }

    m_voutSeries->replace(voutPts);
    m_vinSeries->replace(vinPts);
    m_ioutSeries->replace(ioutPts);
    m_tempSeries->replace(tempPts);

    // Update axes
    double margin = 5.0;
    m_voltXAxis->setRange(cutoff, now);
    m_voltYAxis->setRange(vMin - margin, vMax + margin);
    m_currXAxis->setRange(cutoff, now);
    m_currYAxis->setRange(iMin - margin, iMax + margin);
    m_tempXAxis->setRange(cutoff, now);
    m_tempYAxis->setRange(tMin - margin, tMax + margin);
}

void GraphPanel::onPauseToggled(bool checked) {
    m_paused = checked;
    m_pauseBtn->setText(checked ? "Resume" : "Pause");
}

void GraphPanel::onWindowChanged(const QString& text) {
    auto it = WINDOW_OPTIONS.find(text);
    m_windowSec = (it != WINDOW_OPTIONS.end()) ? it.value() : 600;
    m_dataDirty = true;
}

void GraphPanel::clearData() {
    m_ts.clear();
    m_vout.clear();
    m_vin.clear();
    m_iout.clear();
    m_temp.clear();
    m_status.clear();
    m_markerData.clear();
    m_t0 = monoNowGraph();

    m_voutSeries->clear();
    m_vinSeries->clear();
    m_ioutSeries->clear();
    m_tempSeries->clear();
}

void GraphPanel::exportCsv() {
    QString path = QFileDialog::getSaveFileName(
        this, "Export CSV",
        QString("obc_data_%1.csv").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
        "CSV files (*.csv);;All files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << "timestamp_s,Vout_V,Iout_A,Vin_V,Temp_C,status_flags\n";
    for (size_t i = 0; i < m_ts.size(); ++i) {
        out << QString::number(m_ts[i], 'f', 3) << ","
            << QString::number(m_vout[i], 'f', 1) << ","
            << QString::number(m_iout[i], 'f', 1) << ","
            << QString::number(m_vin[i], 'f', 1) << ","
            << QString::number(m_temp[i], 'f', 1) << ","
            << QString("0x%1").arg(m_status[i], 2, 16, QChar('0')) << "\n";
    }

    if (!m_markerData.empty()) {
        out << "\n# EVENTS\n";
        out << "timestamp_s,event_label,severity\n";
        for (const auto& [t, label, sev] : m_markerData) {
            out << QString::number(t, 'f', 3) << "," << label << "," << sev << "\n";
        }
    }
    file.close();
}
