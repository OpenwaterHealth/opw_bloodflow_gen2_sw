import QtQuick 2.0

import QtQuick 2.15
import Qt.labs.qmlmodels 1.0
import QtQuick.Shapes 1.0
import QtQuick.TreeView 2.15
import QtQuick.Controls 2.0

TreeViewTemplate {
    id: control

    /*
        Note: if you need to tweak this style beyond the styleHints, either
        just assign a custom delegate directly to your TreeView, or copy this file
        into your project and use it as a starting point for your custom version.
    */
    styleHints.indent: 25
    styleHints.columnPadding: 20
    styleHints.foregroundOdd: "black"
    styleHints.backgroundOdd: "transparent"
    styleHints.foregroundEven: "black"
    styleHints.backgroundEven: "transparent"
    styleHints.foregroundCurrent: "black"
    styleHints.backgroundCurrent: "#C2E2FE"
    styleHints.foregroundHovered: "black"
    styleHints.backgroundHovered: "#C2E2FE"
    styleHints.overlay:  navigationMode === TreeViewTemplate.Table ? Qt.rgba(0, 0, 0, 0.5) : "transparent"
    styleHints.overlayHovered: "transparent"
    styleHints.indicator: "black"
    styleHints.indicatorHovered: "transparent"

    QtObject {
        // Don't leak API that might not be support
        // by all styles into the public API.
        id: d
        property int hoveredColumn: -1
        property int hoveredRow: -1

        function bgColor(column, row) {
            if (row === d.hoveredRow
                    && (column === d.hoveredColumn || navigationMode === TreeViewTemplate.List)
                    && styleHints.backgroundHovered.a > 0)
                return styleHints.backgroundHovered
            else if (row === currentIndex.row)
                return styleHints.backgroundCurrent
            else if (row % 2)
                return styleHints.backgroundOdd
            else
                return styleHints.backgroundEven
        }

        function fgColor(column, row) {
            if (row === d.hoveredRow
                    && (column === d.hoveredColumn || navigationMode === TreeViewTemplate.List)
                    && styleHints.foregroundHovered.a > 0)
                return styleHints.foregroundHovered
            else if (row === currentIndex.row)
                return styleHints.foregroundCurrent
            else if (row % 2)
                return styleHints.foregroundOdd
            else
                return styleHints.foregroundEven
        }

        function indicatorColor(column, row) {
            if (row === d.hoveredRow
                    && (column === d.hoveredColumn || navigationMode === TreeViewTemplate.List)
                    && styleHints.indicatorHovered.a > 0)
                return styleHints.indicatorHovered
            else
                return styleHints.indicator
        }

        function overlayColor() {
            if (d.hoveredRow === currentIndex.row
                    && (d.hoveredColumn === currentIndex.column || navigationMode === TreeViewTemplate.List)
                    && styleHints.overlayHovered.a > 0)
                return styleHints.overlayHovered
            else
                return styleHints.overlay

        }
    }

    delegate: DelegateChooser {
        DelegateChoice {
            // The column where the tree is drawn
            column: 0

            Rectangle {
                id: treeNode
                implicitWidth: treeNodeLabel.x + treeNodeLabel.width + (styleHints.columnPadding / 2)
                implicitHeight: Math.max(indicator.height, treeNodeLabel.height, checkBox.height)
                color: d.bgColor(column, row)

                property TreeViewTemplate view: TreeViewTemplate.view
                property bool hasChildren: TreeViewTemplate.hasChildren
                property bool isExpanded: TreeViewTemplate.isExpanded
                property int depth: TreeViewTemplate.depth

                Image {
                    id: indicator
                    x: depth * styleHints.indent + 50
                    width: implicitWidth
                    anchors.verticalCenter: parent.verticalCenter
                    source: {
                        if (hasChildren)
                            isExpanded ? "qrc:/images/folderopen.png" : "qrc:/images/folderclosed.png"
                        else
                            "qrc:/images/file.png"
                    }
                }
                CheckBox {
                    id: checkBox
                    anchors {
                        top: parent.top
                    }



                 }

                TapHandler {
                    onTapped: {
                        if (hasChildren)
                            control.toggleExpanded(row)
                        else
                            checkBox.toggle()

                    }
                }
                Text {
                    id: treeNodeLabel
                    x: indicator.x + Math.max(styleHints.indent, indicator.width * 1.5 + checkBox.width)
                    anchors.verticalCenter: parent.verticalCenter
                    clip: true
                    color: d.fgColor(column, row)
                    //font: styleHints.font
                    text: model.display
                    font.pointSize: 16
                }
            }
        }
    }

    Shape {
        id: overlayShape
        z: 10
        property point currentPos: currentItem ? mapToItem(overlayShape, Qt.point(currentItem.x, currentItem.y)) : Qt.point(0, 0)
        visible: currentItem != null

        ShapePath {
            id: path
            fillColor: "transparent"
            strokeColor: d.overlayColor()
            strokeWidth: 1
            strokeStyle: ShapePath.DashLine
            dashPattern: [1, 2]
            startX: currentItem ? currentItem.x + strokeWidth : 0
            startY: currentItem ? currentItem.y + strokeWidth : 0
            property real endX: currentItem ? currentItem.width + startX - (strokeWidth * 2) : 0
            property real endY: currentItem ? currentItem.height + startY - (strokeWidth * 2) : 0
            PathLine { x: path.endX; y: path.startY }
            PathLine { x: path.endX; y: path.endY }
            PathLine { x: path.startX; y: path.endY }
            PathLine { x: path.startX; y: path.startY }
        }
    }
}
