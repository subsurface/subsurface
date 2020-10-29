import QtQuick 2.0
import QtCharts 2.0

ChartView {
	id:graph
	implicitWidth: 600
	implicitHeight: 400
	legend.visible: false
	title: ""
	legend.alignment: Qt.AlignBottom
	antialiasing: true
//	required model


// =============== X -Y SCATTER GRAPH ===========================
    ScatterSeries {
    borderColor:"white"
    borderWidth: 1
    
        name: "DiveDepth"
        
            XYPoint { x: 5.5; y: 12 }
            XYPoint { x: 5.9; y: 16 }
            XYPoint { x: 9.0; y: 16 }
            XYPoint { x: 9.54; y: 14 }
            XYPoint { x: 10.0; y: 15}
            XYPoint { x: 10.6; y: 21 }
            XYPoint { x: 14.2; y: 15.6 }
            XYPoint { x: 14.7; y: 16.1}
            XYPoint { x: 15.1; y: 16.3 }
            XYPoint { x: 15.8; y: 18.5 }
            XYPoint { x: 19.0; y: 16.0 }
            XYPoint { x: 19.8; y: 14.8 }
            XYPoint { x: 20.1; y: 22.7 }
            XYPoint { x: 22.5; y: 16.1 }
            XYPoint { x: 24.2; y: 14.9 }
            XYPoint { x: 25.0; y: 17.4 }
            XYPoint { x: 25.5; y: 15.7 }
            XYPoint { x: 27.5; y: 18.1 }
            XYPoint { x: 28.4; y: 21.0 }
            XYPoint { x: 29.3; y: 17.9 }
            XYPoint { x: 30.5; y: 16.3 }
            XYPoint { x: 32.5; y: 17.5 }
            XYPoint { x: 34.6; y: 18.1 }
            XYPoint { x: 34.9; y: 16.4 }
            XYPoint { x: 35.5; y: 18.5 }
            XYPoint { x: 37.4; y: 20.6 }
            XYPoint { x: 38.5; y: 19.8 }
            XYPoint { x: 39.5; y: 22.6}

        axisY: ValueAxis {
            id:axisY
            min: 10
            max: 25
            tickCount: 4
            labelFormat: "%d"
            titleText: "SAC (l/min)"
        }

        axisX: ValueAxis {
            id:axisX
            min: 0
            max: 40
            tickCount: 9
            labelFormat: "%d"
            titleText: "Depth (m)"
        }
    } // scatterseries

// ======================= END of GRAPH ===========================

} // ChartView

