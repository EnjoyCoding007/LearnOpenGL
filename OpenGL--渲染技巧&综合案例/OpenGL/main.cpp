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
#include "StopWatch.h"
#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

#define SPHERE_NUMS 30

GLMatrixStack       mvMatrixStack;
GLMatrixStack       pMatrixStack;

//几何变换管线
GLGeometryTransform transformPipeline;

GLShaderManager     shaderManager;
GLFrame             cameraFrame; //观察者/相机坐标系
GLFrame             objectFrame; //地球的模型空间坐标系
GLFrame             sphereFrames[SPHERE_NUMS];//30个随机球的模型空间坐标系
GLFrustum           viewFrustum; //视景体

GLTriangleBatch     earthBatch;//地球
GLTriangleBatch     sphereBatch;//小球
GLBatch             floorBatch;//地板

//纹理数组
GLuint uiTextures[3];

void init();
void changeSize(GLint w,GLint h);
void display(void);
void specialKeys(int key, int x, int y);
bool loadTexture(const char *szFileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode, bool isTGA);


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
    glutInitWindowSize(1200,800);
    glutCreateWindow("GL_Earth");
    
    glutReshapeFunc(changeSize);
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
    //删除纹理
    glDeleteTextures(3, uiTextures);
    return 0;
}

//一次性初始化操作
void init()
{
    //设置背影颜色
    glClearColor(0.0f,0.0f,0.0f,1.0f);
    //初始化着色管理器
    shaderManager.InitializeStockShaders();
    //开启深度测试
    glEnable(GL_DEPTH_TEST);
    //开启背面剔除
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    
    //地球
    gltMakeSphere(earthBatch, 0.5, 50, 100);
    //小球
    gltMakeSphere(sphereBatch, 0.1, 28, 14);
    
    //设置地板顶点数据&地板纹理
    GLfloat texSize = 10.0f;
    floorBatch.Begin(GL_TRIANGLE_FAN, 4,1);
    floorBatch.MultiTexCoord2f(0, 0.0f, 0.0f);
    floorBatch.Vertex3f(-25.f, -0.41f, 25.0f);
    floorBatch.MultiTexCoord2f(0, texSize, 0.0f);
    floorBatch.Vertex3f(25.0f, -0.41f, 25.f);
    floorBatch.MultiTexCoord2f(0, texSize, texSize);
    floorBatch.Vertex3f(25.0f, -0.41f, -25.0f);
    floorBatch.MultiTexCoord2f(0, 0.0f, texSize);
    floorBatch.Vertex3f(-25.0f, -0.41f, -25.0f);
    floorBatch.End();
    
    //随机小球顶点坐标
    for (int i = 0; i < SPHERE_NUMS; i++) {
        
        //y轴不变，X,Z产生随机值
        GLfloat x = ((GLfloat)((rand() % 400) - 200 ) * 0.06f);
        GLfloat z = ((GLfloat)((rand() % 400) - 200 ) * 0.06f);
        
        //在y方向，将球体设置为0.0的位置，这使得它们看起来是飘浮在眼睛的高度
        //对spheres数组中的每一个顶点，设置顶点数据
        sphereFrames[i].SetOrigin(x, 0.0f, z);
    }
    
    //命名纹理对象
    glGenTextures(3, uiTextures);
    
    //将 TGA/BMP 文件加载为2D纹理。
    glBindTexture(GL_TEXTURE_2D, uiTextures[0]);
    loadTexture("water.bmp", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT,false);
    
    
    glBindTexture(GL_TEXTURE_2D, uiTextures[1]);
    loadTexture("earth.bmp", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE,false);
    
    
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    loadTexture("moonlike.tga", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE,true);
    
}

bool loadTexture(const char *szFileName, GLenum minFilter, GLenum magFilter, GLenum wrapMode, bool isTGA)
{
    GLbyte *pBits;
    int nWidth, nHeight, nComponents;
    GLenum eFormat = 0;
    
    //1.读取纹理数据
    if (isTGA) {
        pBits = gltReadTGABits(szFileName, &nWidth, &nHeight, &nComponents, &eFormat);
    }else{
        pBits = gltReadBMPBits(szFileName, &nWidth, &nHeight);
    }
    if(pBits == NULL)
        return false;
    
    //2、设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    
    //3.载入纹理
    glTexImage2D(GL_TEXTURE_2D, 0, GL_COMPRESSED_RGB, nWidth, nHeight, 0,
                 GL_BGR, GL_UNSIGNED_BYTE, pBits);
    
    //释放pBits
    free(pBits);
    
    //只有minFilter 等于以下四种模式之一，才可以生成Mip贴图
    //GL_NEAREST_MIPMAP_NEAREST 具有非常好的性能，并且闪烁现象非常弱
    //GL_LINEAR_MIPMAP_NEAREST 常常用于对游戏进行加速，它使用了高质量的线性过滤器
    //GL_LINEAR_MIPMAP_LINEAR 和 GL_NEAREST_MIPMAP_LINEAR 过滤器在Mip层之间执行了一些额外的插值，以消除他们之间的过滤痕迹。
    //GL_LINEAR_MIPMAP_LINEAR 三线性Mip贴图。纹理过滤的黄金准则，具有最高的精度。
    if(minFilter == GL_LINEAR_MIPMAP_LINEAR ||
       minFilter == GL_LINEAR_MIPMAP_NEAREST ||
       minFilter == GL_NEAREST_MIPMAP_LINEAR ||
       minFilter == GL_NEAREST_MIPMAP_NEAREST)
        //4.加载Mip,纹理生成所有的Mip层
        glGenerateMipmap(GL_TEXTURE_2D);
    
    return true;
}

void drawSomething(GLfloat yRot)
{
    //1.定义光源位置&漫反射颜色
    static GLfloat vWhite[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    static GLfloat vLightPos[] = { 3.0f, 0.0f, 3.0f, 3.0f };
    
    //2.绘制静止悬浮小球
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    for(int i = 0; i < SPHERE_NUMS; i++) {
        mvMatrixStack.PushMatrix();
        mvMatrixStack.MultMatrix(sphereFrames[i]);
        shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                     mvMatrixStack.GetMatrix(),
                                     transformPipeline.GetProjectionMatrix(),
                                     vLightPos,
                                     vWhite,
                                     0);
        sphereBatch.Draw();
        mvMatrixStack.PopMatrix();
    }
    
    //3.绘制地球
    mvMatrixStack.Translate(0.0f, 0.2f, -2.5f);
    mvMatrixStack.Rotate(-90, 1.0f, 0.0f, 0.0f);
    mvMatrixStack.PushMatrix();
    mvMatrixStack.Rotate(yRot, 0.0f, 0.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, uiTextures[1]);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                 mvMatrixStack.GetMatrix(),
                                 transformPipeline.GetProjectionMatrix(),
                                 vLightPos,
                                 vWhite,
                                 0);
    earthBatch.Draw();
    mvMatrixStack.PopMatrix();
    
    //4.绘制公转小球（公转自转)
    mvMatrixStack.Rotate(-90, 1.0f, 0.0f, 0.0f);
    mvMatrixStack.PushMatrix();
    mvMatrixStack.Rotate(yRot * 2.0f, 0.0f, 1.0f, 0.0f);
    mvMatrixStack.Translate(0.8f, 0.0f, 0.0f);
    glBindTexture(GL_TEXTURE_2D, uiTextures[2]);
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_POINT_LIGHT_DIFF,
                                 mvMatrixStack.GetMatrix(),
                                 transformPipeline.GetProjectionMatrix(),
                                 vLightPos,
                                 vWhite,
                                 0);
    sphereBatch.Draw();
    mvMatrixStack.PopMatrix();
    
}

//渲染
void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    //地板颜色值
    static GLfloat vFloorColor[] = { 0.0f, 1.0f, 1.0f, 0.6f};
    
    //动画周期
    static CStopWatch    rotTimer;
    float yRot = rotTimer.GetElapsedSeconds() * 60.0f;
    // 压栈
    mvMatrixStack.PushMatrix();
    
    //设置观察者矩阵
    M3DMatrix44f mCamera;
    cameraFrame.GetCameraMatrix(mCamera);
    mvMatrixStack.MultMatrix(mCamera);
    
    //压栈
    mvMatrixStack.PushMatrix();
    
    //---添加反光效果---
    //翻转Y轴
    mvMatrixStack.Scale(1.0f, -1.0f, 1.0f);
    //镜面世界围绕Y轴平移一定间距
    mvMatrixStack.Translate(0.0f, 0.8f, 0.0f);
    
    //指定顺时针为正面
    glFrontFace(GL_CW);
    
    //绘制地面以下其他部分
    drawSomething(yRot);
    
    //恢复为逆时针为正面
    glFrontFace(GL_CCW);
    
    //镜面绘制结束，恢复矩阵
    mvMatrixStack.PopMatrix();
    
    //开启混合功能(绘制地板) 因为需要半透明效果
    glEnable(GL_BLEND);
    
    //指定颜色混合方程式
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    //绑定地面纹理
    glBindTexture(GL_TEXTURE_2D, uiTextures[0]);
    
    //纹理调整着色器
    shaderManager.UseStockShader(GLT_SHADER_TEXTURE_MODULATE,
                                 transformPipeline.GetModelViewProjectionMatrix(),
                                 vFloorColor,
                                 0);
    //开始绘制地板
    floorBatch.Draw();
    
    //取消混合
    glDisable(GL_BLEND);
    
    //绘制地面以上部分
    drawSomething(yRot);
    
    //绘制完，恢复矩阵
    mvMatrixStack.PopMatrix();
    
    //交换缓存区
    glutSwapBuffers();
    
    //提交重新渲染
    glutPostRedisplay();
    
}

//窗口大小改变时通知新的size，其中0,0代表窗口中视口的左下角坐标，w，h代表像素
void changeSize(GLint w,GLint h)
{
    glViewport(0,0, w, h);
    viewFrustum.SetPerspective(40.f, GLfloat(w)/GLfloat(h), 1.0f, 500.f);
    pMatrixStack.LoadMatrix(viewFrustum.GetProjectionMatrix());
    mvMatrixStack.LoadIdentity();
    transformPipeline.SetMatrixStacks(mvMatrixStack, pMatrixStack);
}


//特殊键位处理（上、下、左、右移动）
void specialKeys(int key, int x, int y)
{
    
    float linear = 0.1f;
    float angular = float(m3dDegToRad(5.0f));
    
    if (key == GLUT_KEY_UP) {
        
        //MoveForward 平移
        cameraFrame.MoveForward(linear);
    }
    
    if (key == GLUT_KEY_DOWN) {
        cameraFrame.MoveForward(-linear);
    }
    
    if (key == GLUT_KEY_LEFT) {
        //RotateWorld 旋转
        cameraFrame.RotateWorld(angular, 0.0f, 1.0f, 0.0f);
    }
    
    if (key == GLUT_KEY_RIGHT) {
        cameraFrame.RotateWorld(-angular, 0.0f, 1.0f, 0.0f);
    }
    
    glutPostRedisplay();
}

