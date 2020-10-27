import QtQuick 2.0
import QtCharts 2.0

/*  ChartView {
      width: 800
      height: 600
      theme: ChartView.ChartThemeBrownSand
      antialiasing: true

      PieSeries {
          id: pieSeries
          PieSlice { label: "eaten"; value: 94.9 }
          PieSlice { label: "not yet eaten"; value: 5.1 }
      }
  }
  */

ChartView {
    width: 600
    height: 400
    legend.visible: false
    title: "Dive depth"
//  anchors.fill: parent
    legend.alignment: Qt.AlignBottom
    antialiasing: true
//    required model

    BarSeries {
        id: mySeries
        barWidth: 0.8
//        required property variant
        axisX: BarCategoryAxis { 
            categories: ["0-5m", "5-10m", "10-15m", "15-20m", "20-30m", "30-40m" ]
            titleText: "Depth (m)"
        }

        axisY: ValueAxis {
            id:axisY
            min: 0
            max: 70
            tickCount: 8
            labelFormat: "%d"
            titleText: "No. dives"
        }

         BarSet { 
            label: "count"
            values: [2, 14, 47, 62, 15, 4] 
//            values: parent.modelData
         }
    }
}
