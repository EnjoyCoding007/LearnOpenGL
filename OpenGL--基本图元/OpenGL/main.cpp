//
//  main.cpp
//  OpenGL
//
//  Created by Sharon on 2019/4/19.
//  Copyright © 2019 Sharon. All rights reserved.
//
#include "GLTools.h"
#include "GLMatrixStack.h"
#include "GLFrame.h"
#include "GLFrustum.h"
#include "GLGeometryTransform.h"

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

/*
 GLMatrixStack 使用矩阵堆栈变换矩阵
 
 GLMatrixStack 构造函数允许指定堆栈的最大深度、默认的堆栈深度为64. 其在初始化时已经在堆栈中包含了单元矩阵。
 GLMatrixStack::GLMatrixStack(int iStackDepth = 64);
 
 //在栈顶载入单元矩阵
 void GLMatrixStack::LoadIndentiy(void);
 
 //在栈顶部载入任何矩阵
 void GLMatrixStack::LoadMatrix(const M3DMatrix44f m);
 */
GLMatrixStack       mvMatrixStack;
GLMatrixStack       pMatrixStack;

//几何变换管线
GLGeometryTransform transformPipeline;

GLShaderManager     shaderManager;
GLFrame             cameraFrame; //观察者坐标系
GLFrame             objectFrame; //物体坐标系
GLFrustum           viewFrustum; //视景体

//不同图元对应的容器
GLBatch             pointBatch;
GLBatch             lineBatch;
GLBatch             lineStripBatch;
GLBatch             lineLoopBatch;
GLBatch             triangleBatch;
GLBatch             triangleStripBatch;
GLBatch             triangleFanBatch;

//颜色
GLfloat vBlack[] = {0.0f, 0.0f, 0.0f, 1.0f};
GLfloat vRed[] = {0.9f, 0.2f, 0.2f, 1.0f};

void init();
void changeSize(GLint w,GLint h);
void display(void);
void specialKeys(int key, int x, int y);
void spaceKeyPressFunc(unsigned char key, int x, int y);

//敲空格键次数
GLint step = 0;

int main(int argc,char* argv[])
{
    //设置当前工作目录，针对MAC OS X
    gltSetWorkingDirectory(argv[0]);
    
    //初始化GLUT库
    glutInit(&argc, argv);
    
    /*初始化双缓冲窗口，GLUT_DOUBLE、GLUT_RGBA、GLUT_DEPTH、GLUT_STENCIL分别指：
     双缓冲窗口、RGBA颜色模式、深度测试、模板缓冲区*/
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH|GLUT_STENCIL);
    
    //窗口大小、标题
    glutInitWindowSize(800,600);
    glutCreateWindow("GL_POINTS");
    
    glutReshapeFunc(changeSize);
    //点击空格键调用
    glutKeyboardFunc(spaceKeyPressFunc);
    //特殊键位调用（上下左右）
    glutSpecialFunc(specialKeys);
    glutDisplayFunc(display);
    
    //判断glew库是否初始化成功，确保能正常使用 OpenGL 框架
    GLenum err = glewInit();
    if(GLEW_OK != err) {
        fprintf(stderr,"glew error:%s\n",glewGetErrorString(err));
        return 1;
    }
    
    init();
    
    glutMainLoop();
    
    return 0;
}

//一次性初始化操作
void init()
{
    //设置背影颜色
    glClearColor(1.0f,1.0f,1.0f,1.0f);
    //初始化着色管理器
    shaderManager.InitializeStockShaders();
    //开启深度测试
    glEnable(GL_DEPTH_TEST);
    //设置几何变换管线以使用矩阵堆栈
    transformPipeline.SetMatrixStacks(mvMatrixStack, pMatrixStack);
    //移动观察者
    cameraFrame.MoveForward(-30.f);
    
    
    //三角形顶点，数组vVerts包含所有顶点的笛卡尔坐标（ x,y,z ）
    GLfloat vVerts[] = {
        -3.f, -2.5f, 0.0f,
        0.f, 2.5f, 0.0f,
        3.f, -2.5f, 0.0f
    };
    
    //点的形式
    pointBatch.Begin(GL_POINTS,3);
    pointBatch.CopyVertexData3f(vVerts);
    pointBatch.End();
    //通过线的形式
    lineBatch.Begin(GL_LINES, 3);
    lineBatch.CopyVertexData3f(vVerts);
    lineBatch.End();
    //通过线段的形式
    lineStripBatch.Begin(GL_LINE_STRIP, 3);
    lineStripBatch.CopyVertexData3f(vVerts);
    lineStripBatch.End();
    //通过线环的形式
    lineLoopBatch.Begin(GL_LINE_LOOP, 3);
    lineLoopBatch.CopyVertexData3f(vVerts);
    lineLoopBatch.End();
    
    //通过三角形创建金字塔（四角锥）
    GLfloat vPyramid[12][3] = {
        -3.0f, -2.5f, -3.0f,
        3.0f, -2.5f, -3.0f,
        0.0f, 2.5f, 0.0f,
        
        -3.0f, -2.5f, 3.0f,
        3.0f, -2.5f, 3.0f,
        0.0f, 2.5f, 0.0f,
        
        3.0f, -2.5f, -3.0f,
        3.0f, -2.5f, 3.0f,
        0.0f, 2.5f, 0.0f,
        
        -3.0f, -2.5f, 3.0f,
        -3.0f, -2.5f, -3.0f,
        0.0f, 2.5f, 0.0f,
    };
    
    //GL_TRIANGLES 每3个顶点定义一个新的三角形
    triangleBatch.Begin(GL_TRIANGLES, 12);
    triangleBatch.CopyVertexData3f(vPyramid);
    triangleBatch.End();
    
    // 伞面
    GLfloat vPoints[100][3];
    int nVerts = 0;
    //半径
    GLfloat r = 7.0f;
    // 三角形个数
    GLint vCount = 28;
    
    //原点(x,y,z) = (0,0,-5.0);
    vPoints[nVerts][0] = 0.0f;
    vPoints[nVerts][1] = 0.0f;
    vPoints[nVerts][2] = -5.0f;
    //M3D_2PI 就是2Pi = 360°
    for(GLdouble angle = 0; angle <= M3D_2PI; angle += M3D_2PI / vCount) {
        
        nVerts++;
        /*
         弧长=半径*角度,这里的角度需要转换成弧度制
         */
        //x点坐标 cos(angle) * 半径
        vPoints[nVerts][0] = float(cos(angle)) * r;
        //y点坐标 sin(angle) * 半径
        vPoints[nVerts][1] = float(sin(angle)) * r;
        //z点的坐标
        vPoints[nVerts][2] = 0.0f;
    }
    
    //GL_TRIANGLE_FAN 以一个圆心为中心呈扇形排列，共用相邻顶点的一组三角形
    triangleFanBatch.Begin(GL_TRIANGLE_FAN, vCount+2);
    triangleFanBatch.CopyVertexData3f(vPoints);
    triangleFanBatch.End();
    
    //三角形条带
    int iCounter = 0;
    //半径
    GLfloat radius = 6.0f;
    for(GLdouble angle = 0.0f; angle <= M3D_2PI; angle += M3D_2PI/18)
    {
        //获取圆形的顶点的X,Y
        GLfloat x = radius * sin(angle);
        GLfloat y = radius * cos(angle);
        
        //绘制2个三角形（他们的x,y顶点一样，只是z点不一样）
        vPoints[iCounter][0] = x;
        vPoints[iCounter][1] = y;
        vPoints[iCounter][2] = -1.0;
        iCounter++;
        
        vPoints[iCounter][0] = x;
        vPoints[iCounter][1] = y;
        vPoints[iCounter][2] = 1.0;
        iCounter++;
        
        printf("%f,%f,%f \n",vPoints[iCounter][0],vPoints[iCounter][1],vPoints[iCounter][2]);
    }
    
    // GL_TRIANGLE_STRIP 共用一个条带（strip）上的顶点的一组三角形
    triangleStripBatch.Begin(GL_TRIANGLE_STRIP, iCounter);
    triangleStripBatch.CopyVertexData3f(vPoints);
    triangleStripBatch.End();
}

void drawWireFrameBatch(GLBatch* pBatch)
{
    /*------------填充色----------------*/
    /*
     参数1：着色器
     参数2：为几何图形变换指定 4 * 4 变换矩阵
     参数3：颜色值
     */
    
    shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(), vRed);
    pBatch->Draw();
    
    /*-----------加边框-------------------*/
    /*
     glEnable(GLenum mode); 用于启用各种功能。功能由参数决定
     参数列表：http://blog.csdn.net/augusdi/article/details/23747081
     注意：glEnable() 不能写在glBegin() 和 glEnd()中间
     
     GL_POLYGON_OFFSET_LINE  根据函数glPolygonOffset的设置，启用线的深度偏移
     GL_LINE_SMOOTH          执行后，过虑线点的锯齿
     GL_BLEND                启用颜色混合。例如实现半透明效果
     GL_DEPTH_TEST           启用深度测试 根据坐标的远近自动隐藏被遮住的图形（材料
    
     glDisable(GLenum mode); 用于关闭指定的功能 功能由参数决定
     */
    
    // 画边框
    glPolygonOffset(-1.0f, -1.0f);
    // 偏移深度，在同一位置要绘制填充和边线，会产生z冲突，所以要偏移
    glEnable(GL_POLYGON_OFFSET_LINE);
    
    // 画反锯齿，让黑边好看些
    glEnable(GL_LINE_SMOOTH);
    
    //绘制线框几何黑色版 三种模式，实心，边框，点，可以作用在正面，背面，或者两面
    //通过调用glPolygonMode将多边形正面或者背面设为线框模式，实现线框渲染
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    //设置线条宽度
    glLineWidth(2.0f);
    
    shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(), vBlack);
    pBatch->Draw();
    
    // 复原原本的设置
    ////通过调用glPolygonMode将多边形正面或者背面设为全部填充模式
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
    glDisable(GL_LINE_SMOOTH);
}

//渲染
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    //压栈
    mvMatrixStack.PushMatrix();
    
    //获取矩阵
    M3DMatrix44f mCamera;
    cameraFrame.GetCameraMatrix(mCamera);
    
    //矩阵乘以堆栈的顶部单元矩阵， 将物体坐标系转换到观察者坐标系。相乘的结果存在堆栈的顶部
    mvMatrixStack.MultMatrix(mCamera);
    
    M3DMatrix44f mObjectFrame;
    objectFrame.GetMatrix(mObjectFrame);
    
    mvMatrixStack.MultMatrix(mObjectFrame);
    
    if (step<=3) {
        shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(), vBlack);
    }
    
    switch(step) {
        case 0:
            //设置点的大小
            glPointSize(5.0f);
            pointBatch.Draw();
            glPointSize(1.0f);
            break;
        case 1:
            //设置线的宽度
            glLineWidth(2.0f);
            lineBatch.Draw();
            glLineWidth(1.0f);
            break;
        case 2:
            glLineWidth(2.0f);
            lineStripBatch.Draw();
            glLineWidth(1.0f);
            break;
        case 3:
            glLineWidth(2.0f);
            lineLoopBatch.Draw();
            glLineWidth(1.0f);
            break;
        case 4:
            drawWireFrameBatch(&triangleBatch);
            break;
        case 5:
            drawWireFrameBatch(&triangleFanBatch);
            break;
        case 6:
            drawWireFrameBatch(&triangleStripBatch);
            break;
    }
    
    //还原到以前的模型视图矩阵 --> 单位矩阵
    mvMatrixStack.PopMatrix();
    
    // 进行缓冲区交换
    glutSwapBuffers();
}

//窗口大小改变时通知新的size，其中0,0代表窗口中视口的左下角坐标，w，h代表像素
void changeSize(GLint w,GLint h)
{
    glViewport(0,0, w, h);
    viewFrustum.SetPerspective(40.f, GLfloat(w)/GLfloat(h), 1.0f, 500.f);
    pMatrixStack.LoadMatrix(viewFrustum.GetProjectionMatrix());
}

//根据空格次数。切换不同的“窗口名称”
void spaceKeyPressFunc(unsigned char key, int x, int y)
{
    if(key == 32)
    {
        step++;
        
        if(step > 6)
            step = 0;
    }
    
    switch(step)
    {
        case 0:
            glutSetWindowTitle("GL_POINTS");
            break;
        case 1:
            glutSetWindowTitle("GL_LINES");
            break;
        case 2:
            glutSetWindowTitle("GL_LINE_STRIP");
            break;
        case 3:
            glutSetWindowTitle("GL_LINE_LOOP");
            break;
        case 4:
            glutSetWindowTitle("GL_TRIANGLES");
            break;
        case 5:
            glutSetWindowTitle("GL_TRIANGLE_STRIP");
            break;
        case 6:
            glutSetWindowTitle("GL_TRIANGLE_FAN");
            break;
    }
    glutPostRedisplay();
}

//特殊键位处理（上、下、左、右移动）
void specialKeys(int key, int x, int y)
{
    
    if(key == GLUT_KEY_UP)
        //围绕一个指定的X,Y,Z轴旋转。
        objectFrame.RotateWorld(m3dDegToRad(-5.0f), 1.0f, 0.0f, 0.0f);
    
    if(key == GLUT_KEY_DOWN)
        objectFrame.RotateWorld(m3dDegToRad(5.0f), 1.0f, 0.0f, 0.0f);
    
    if(key == GLUT_KEY_LEFT)
        objectFrame.RotateWorld(m3dDegToRad(-5.0f), 0.0f, 1.0f, 0.0f);
    
    if(key == GLUT_KEY_RIGHT)
        objectFrame.RotateWorld(m3dDegToRad(5.0f), 0.0f, 1.0f, 0.0f);
    
    glutPostRedisplay();
}

