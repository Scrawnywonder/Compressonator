//=====================================================================
// Copyright 2016 (c), Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
/// \file acImageView.h
/// \version 2.21
//
//=====================================================================

#ifndef _ACIMAGEVIEW_H
#define _ACIMAGEVIEW_H

// Qt
#include <QtWidgets>
#include <QGraphicsSceneEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGLWidget>

// Local
#include "acCustomGraphics.h"
#include "acTableImageDataModel.h"
#include "cpImageLoader.h"

#define AC_IMAGE_MAX_ZOOM  9999
#define AC_IMAGE_MIN_ZOOM  10

#define DEFAULT_BRIGHTNESS_LEVEL 6

// ----------------------------------------------------------------------------------
// Class Name:          acImageView
// General Description: This class represent an image item widget that has builtin  
//                      properties for navigation, zoom and panning
// Creation Date:       13/8/2015
// ----------------------------------------------------------------------------------
class acImageView : public QWidget
{
    Q_OBJECT

public:
    acImageView(const QString fileName = "", QWidget *parent = 0, QImage** image = NULL, CMipImages *MipImages = NULL);
    ~acImageView();

    CImageLoader                     *m_imageloader;

    CMipImages                       *m_MipImages;            // The Main Image data
    acCustomGraphicsImageItem        *m_imageItem;            // The displayed Graphics item of m_image
    acCustomGraphicsView             *m_imageGraphicsView;    // View displayed to user
    acCustomGraphicsScene            *m_graphicsScene;        // Scene hosting the image

    QImage                           *m_navImage;             // The navigation Image 
    acCustomGraphicsNavImageItem     *m_imageItemNav;         // The displayed Graphics item of m_image
    acCustomGraphicsScene            *m_graphicsSceneNav;     // Scene hosting the navigation image
    acCustomGraphicsNavWindow        *m_navWindow;            // Navigation windows inside a navigation image view 

    QGridLayout                      *m_layout;               // This Widgets layout is a Single GraphicsView

    QGraphicsLineItem                *m_linex;                
    QGraphicsLineItem                *m_liney;


    QWidget                          *m_navigateButton;       // Navigation button     
    bool                              m_navVisible;           // Navigation visible states (default : false = which is not shown)
    bool                              m_isDiffView;

    QGraphicsItem *                  m_tableViewitem;         // Table Image Data View item
    QTableView*                      m_tableView;             // Qt Table View Widget
    acTableImageDataModel*           m_myModel;               // Table Image Data model for the view above
    //Reserved: GPUDecode         

    int                              m_ImageScale;            // Current scale of the image 100 = original size

    bool                             m_MouseHandDown;         // Set when mouse hand is active
    QPointF                          m_lastMousePos;          // Set when user moves mouse and has Mouse HandDown

                                                              // Get Views ScrollBars
    QScrollBar * m_xPos;                                      // horizontalScrollBar();
    int m_defaultXPos_minimum;
    int m_defaultXPos_maximum;

    QScrollBar * m_yPos;                                      // verticalScrollBar();
    int m_defaultYPos_minimum;
    int m_defaultYPos_maximum;

    void resizeEvent(QResizeEvent *e);
    void enableNavigation(bool enable);
    void showVirtualCursor();
    void hideVirtualCursor();
    void showTableView(bool display);
    void centerImage();
    //Reserved: GPUDecode
    bool IsImageBoundedToView(QPointF *mousePos);

private:
    int  m_imageOrientation;                                 // Tracks Image Rotation from 0 - North (upright) to 1 - East 2 - South 3 - West
    bool m_localMipImages;
    int  m_ImageIndex;                                       // QImage[] index

public slots:
    void onVirtualMouseMoveEvent(QPointF *pos, QPointF *localPos, int onID);     //   
    void onResetImageView();                                  // 
    void onToggleChannelR();                                  // Channel Red
    void onToggleChannelG();                                  // Channel Green
    void onToggleChannelB();                                  // Channel Blue
    void onToggleChannelA();                                  // Channel Alpha
    void onToggleGrayScale();                                 // Gray Scale
    void onToggleImageBrightnessUp();                         // Increase image Brightness
    void onToggleImageBrightnessDown();                       // Decrease image Brightness
    void onInvertImage();                                     // Invert Image
    void onMirrorHorizontal();                                // Mirror Image Horizontal
    void onMirrorVirtical();                                  // Mirror Image Virtical
    void onRotateRight();                                     // Rotate Image Right 90 Degrees
    void onRotateLeft();                                      // Rotate Image Left  90 Degrees
    void onViewImageOriginalSize();                           // Image Original Size
    void onFitInWindow();                                     // Image Original Size
    void onGridBackground(int enable);                        // Display a backround image
    void onImageLevelChanged(int MipLevel);                   // Switch between QImage[] - On Compressonator its MipLevels
    void onMouseHandDown();                                   // Signaled when user pressed Left Mouse Key Down and Moving cursor
    void onMouseHandD();                                      // Signaled when user releases Left Mouse Key Down and Mouse Cursor was down
    void onWheelScaleUp(QPointF &pos);                        // Zoom Image In   
    void onWheelScaleDown(QPointF &pos);                      // Zoom Image Out
    void onSetScale(int value);
    void ManageScrollBars();

private slots:
    void onacImageViewMousePosition(QPointF *pos, int ID);   // connects to SIGNAL graphicsscene::scenemouseposition   
    void onMouseDoubleClickEvent(QMouseEvent * e);
    void onNavigateClicked();

Q_SIGNALS:
    void acImageViewMousePosition(QPointF *scenePos, QPointF *localPos, int ID);     // Signals to user the current mouse position in the view
    void acImageViewVirtualMousePosition(QPointF *scenePos, QPointF *localPos, int ID);
    void acScaleChanged(int value);

};


static const char * const qt_navigate_png[] = {""};


#endif // _ACIMAGEVIEW_H