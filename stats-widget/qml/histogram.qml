import QtQuick 2.0
import QtCharts 2.0
import QtQuick.Layouts 1.1

ChartView {
	id:graph
	implicitWidth: 600
	implicitHeight: 400
	legend.visible: false
	title: ""
	legend.alignment: Qt.AlignBottom
	antialiasing: true
//	required model

	DropArea {
		anchors.fill: parent
	}

/*
// ================== BAR GRAPH =====================
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
         
         
         
     }  // barSeries
       
    
	Rectangle {
		id:summary
		radius: 5
		height: Math.round(graph.height/6)
		width: Math.round(graph.width/6)
		x: Math.round(graph.width/6)
		y: summary.height
		color: "gray"
		z: mouseArea.drag.active ||  mouseArea.pressed ? 2 : 1
		Drag.active: mouseArea.drag.active
		property point beginDrag

		GridLayout {
			id: grid
			columns: 2
			anchors.fill: parent

			Text {
				id: maxlabl
				text: " Max:"
				color: "white"
			}

			Text {
				id: maxtext
				text: "36.4m"
				color: "white"
			}

			Text {
				id:meanlabl
				text: " Mean:"
				color: "white"
			}

			Text {
				id: meantext
				text: "26.7m"
				color: "white"
			}

			Text {
				id: minlabl
				text: " Min:"
				color: "white"
			}

			Text {
				id: mintext
				text: "2.7m"
				color: "white"
			}

		} //grid

		MouseArea {
			id: mouseArea
			anchors.fill: parent
			drag.target: parent
			onPressed: {
				summary.beginDrag = Qt.point(summary.x, summary.y);
			}
		}  // mousearea

	}  // rectangle
// ============ END (BAR GRAPH) ===============
*/


// =============== HISTOGRAM ===========================
    AreaSeries {
        name: "DiveDepth"
        upperSeries: LineSeries {
            XYPoint { x: 0.5; y: 0 }
            XYPoint { x: 0.5; y: 2 }
            XYPoint { x: 4.5; y: 2 }
            XYPoint { x: 4.5; y: 0 }
            XYPoint { x: 5.5; y: 0 }
            XYPoint { x: 5.5; y: 16 }
            XYPoint { x: 9.5; y: 16 }
            XYPoint { x: 9.5; y: 0 }
            XYPoint { x: 10.5; y: 0 }
            XYPoint { x: 10.5; y: 21 }
            XYPoint { x: 14.5; y: 21 }
            XYPoint { x: 14.5; y: 0 }
            XYPoint { x: 15.5; y: 0 }
            XYPoint { x: 15.5; y: 27 }
            XYPoint { x: 19.5; y: 27 }
            XYPoint { x: 19.5; y: 0 }
            XYPoint { x: 20.5; y: 0 }
            XYPoint { x: 20.5; y: 37 }
            XYPoint { x: 24.5; y: 37 }
            XYPoint { x: 24.5; y: 0 }
            XYPoint { x: 25.5; y: 0 }
            XYPoint { x: 25.5; y: 29 }
            XYPoint { x: 29.5; y: 29 }
            XYPoint { x: 29.5; y: 0 }
            XYPoint { x: 30.5; y: 0 }
            XYPoint { x: 30.5; y: 6 }
            XYPoint { x: 34.5; y: 6 }
            XYPoint { x: 34.5; y: 0 }
            XYPoint { x: 35.5; y: 0 }
            XYPoint { x: 35.5; y: 2 }
            XYPoint { x: 39.5; y: 2 }
            XYPoint { x: 39.5; y: 0 }
        }
        
        
        axisY: ValueAxis {
            id:axisY
            min: 0
            max: 40
            tickCount: 5
            labelFormat: "%d"
            titleText: "No. dives"
        }

        axisX: ValueAxis {
            id:axisX
            min: 0
            max: 40
            tickCount: 9
            labelFormat: "%d"
            titleText: "Depth (m)"
        }

    }

	LineSeries {  // Draw red line wat mean value
		color:"red"
		name: "Mean"
		width: 2
		XYPoint { x: 21.1; y: 0 }
		XYPoint { x: 21.1; y: 40 }
	}

	Rectangle {
		id:summary
		radius: 5
		implicitHeight: Math.round(graph.height/6)
		implicitWidth: Math.round(graph.width/6)
		x: Math.round(graph.width/6)
		y: summary.height
		color: "gray"
		z: mouseArea.drag.active ||  mouseArea.pressed ? 2 : 1
		Drag.active: mouseArea.drag.active
		property point beginDrag

		GridLayout {
			id: grid
			columns: 2
			anchors.fill: parent

			Text {
				id: maxlabl
				text: " Max:"
				color: "white"
			}

			Text {
				id:maxval
				text: "36.4m"
				color: "white"
			}

			Text {
				id:meanlabl
				text: " Mean:"
				color: "white"
			}

			Text {
				id:meanval
				text: "26.7m"
				color: "white"
			}

			Text {
				id:minlabl
				text: " Min:"
				color: "white"
			}

			Text {
				id:minval
				text: "2.7m"
				color: "white"
			}

		} //grid

		MouseArea {
			id: mouseArea
			anchors.fill: parent
			drag.target: parent
			onPressed: {
				summary.beginDrag = Qt.point(summary.x, summary.y);
			}
		}

	} // rectangle
// ======================= END of HISOGRAM ===========================

} // ChartView

