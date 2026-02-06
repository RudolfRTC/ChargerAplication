#pragma once

#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QTimer>
#include <QLabel>
#include <deque>
#include <vector>
#include <tuple>

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>

#include "can_protocol.h"

class GraphPanel : public QGroupBox {
    Q_OBJECT
public:
    explicit GraphPanel(QWidget* parent = nullptr);

    void addPoint(const Message2& msg);
    void addEventMarker(const QString& label, const QString& severity = "info");

private slots:
    void onPauseToggled(bool checked);
    void onWindowChanged(const QString& text);
    void clearData();
    void exportCsv();
    void redraw();

private:
    double m_t0;
    bool m_paused = false;
    int m_windowSec = 600;
    bool m_dataDirty = false;

    static constexpr int MAX_POINTS = 3600;

    std::deque<double> m_ts;
    std::deque<double> m_vout;
    std::deque<double> m_vin;
    std::deque<double> m_iout;
    std::deque<double> m_temp;
    std::deque<int>    m_status;

    // Event markers: (timestamp, label, severity)
    std::vector<std::tuple<double, QString, QString>> m_markerData;

    // Charts
    QChart*      m_voltChart;
    QChartView*  m_voltView;
    QLineSeries* m_voutSeries;
    QLineSeries* m_vinSeries;
    QValueAxis*  m_voltXAxis;
    QValueAxis*  m_voltYAxis;

    QChart*      m_currChart;
    QChartView*  m_currView;
    QLineSeries* m_ioutSeries;
    QValueAxis*  m_currXAxis;
    QValueAxis*  m_currYAxis;

    QChart*      m_tempChart;
    QChartView*  m_tempView;
    QLineSeries* m_tempSeries;
    QValueAxis*  m_tempXAxis;
    QValueAxis*  m_tempYAxis;

    QPushButton* m_pauseBtn;
    QPushButton* m_clearBtn;
    QComboBox*   m_windowCombo;
    QPushButton* m_exportBtn;

    QTimer*      m_redrawTimer;

    void setupChart(QChart* chart, const QString& title);
};
