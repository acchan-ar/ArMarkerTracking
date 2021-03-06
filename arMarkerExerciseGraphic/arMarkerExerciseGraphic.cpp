// arMarkerExerciseGraphic.cpp : アプリケーションのエントリ ポイントを定義します。
//
#define GLFW_INCLUDE_GLU
#include "stdafx.h"
#include <math.h>
#include <cstdlib>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include "DrawPrimitives.h"
#include "MarkerTracker.h"

using namespace std;

void initialize();
void renderSphere(GLfloat, GLfloat, GLfloat, double, const GLfloat*);
void renderCarrot();
void Ground();
void renderSnowman(int);
void display(GLFWwindow*, const cv::Mat&);

const int g_windowWidth = 640;
const int g_windowHeight = 480;

/*---------------------------------
	物質質感の定義
----------------------------------*/

static const GLfloat snowMaterial[] = { 1.0f, 1.0f, 1.0f, 1.0f };
static const GLfloat redMaterial[] = { 1.0f, 0.0f, 0.0f, 1.0f };
static const GLfloat eyeMaterial[] = { 0.0f, 0.0f, 0.0f, 1.0f };
static const GLfloat carrotMaterial[] = { 1.0f, 0.7f, 0.0f, 1.0f };

/*-------------------------------------
	光源
--------------------------------------*/

static const GLfloat aLightColor[] = { 0.3f, 0.3f, 0.8f, 1.0f };// 光源の色。
static const GLfloat aLight0pos[] = { 1.0, 0.0, 3.0, 1.0 };// 光源0の位置。
static const GLfloat aLight1pos[] = { 2.0, 0.0, -3.0, 1.0 };// 光源1の位置。


/*-------------------------------------
	マーカーid
	
	id:4000		id:3362

	1000		0000
	0000		1011
	0110		0100
	0001		0100

--------------------------------------*/

/*-------------------------------------
	main
--------------------------------------*/

int main()
{
	// GLFW を初期化する
	if (glfwInit() == GL_FALSE)return 1;

	// プログラム終了時の処理を登録する
	atexit(glfwTerminate);

	// ウィンドウを作成する
	GLFWwindow *const window(glfwCreateWindow(640, 480, "Hello!", NULL, NULL));
	if (window == NULL) {
		glfwTerminate();
		return 1;
	}
	// 作成したウィンドウを OpenGL の処理対象にする
	glfwMakeContextCurrent(window);

	// GLEW を初期化する
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)return 1;

	//OpenCVの設定
	cv::Mat img_bgr;

	cv::VideoCapture cap(1); // デフォルトカメラをオープン
	if (!cap.isOpened())return -1;  // 成功したかどうかをチェック
	//img_bgr = cv::imread("test05.jpg");

	const double kMarkerSize = 0.028;// [m]
	MarkerTracker markerTracker(kMarkerSize);

	map<int, vector<float>> markers;//idとマーカーの変換行列のセット
	
	//変換後の行列
	float glMatrix[16] =
	{
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0
	};
	cv::Mat reverseMat = (cv::Mat_<float>(4,4) << 
		1.0, 0.0, 0.0, 0.0,
		0.0, -1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0 );
		

	initialize(); // 初期化

	// ウィンドウが開いている間繰り返す
	while (glfwWindowShouldClose(window) == GL_FALSE)
	{

		/*-------------------------------------
			マーカーをトラッキング
		--------------------------------------*/

		//カメラ画像の読み込み
		cap >> img_bgr;
		if (img_bgr.empty()) {
			cout << "can't read img_bgr." << endl;
			continue;
		}

		//マーカーを探す
		markers.clear();
		markerTracker.findMarker(img_bgr, markers);
		if (markers.empty()) {
			continue;
		};
		


		/*-------------------------------------
			描画の準備
		--------------------------------------*/

		// ウィンドウを消去する
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, g_windowWidth, g_windowHeight);  //ビューポートの設定

		//プロジェクション設定
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();  //行列の初期化
		gluPerspective(30.0, (double)g_windowWidth / (double)g_windowHeight, 0.1, 1000.0); //視野の設定
		glLightfv(GL_LIGHT0, GL_POSITION, aLight0pos);// 光源0の位置設定 （＊重要 視点の位置を設定した後に行う） 
		glLightfv(GL_LIGHT1, GL_POSITION, aLight1pos);// 光源1の位置設定


		//バックグラウンドの描写
		display(window, img_bgr);


		/*-------------------------------------
			各マーカーに描画する
		--------------------------------------*/

		for (auto marker = markers.begin(); marker != markers.end(); marker++) {
			int id = marker->first;
			vector<float> resultMatrix = marker->second;

			cv::Mat cvResultMat(4, 4, CV_32F, &resultMatrix.front());
			cvResultMat = reverseMat * cvResultMat * reverseMat;

			//アフィン行列を入れる
			glMatrix[0] = cvResultMat.at<float>(0, 0);
			glMatrix[1] = cvResultMat.at<float>(1, 0);
			glMatrix[2] = cvResultMat.at<float>(2, 0);

			glMatrix[4] = cvResultMat.at<float>(0, 1);
			glMatrix[5] = cvResultMat.at<float>(1, 1);
			glMatrix[6] = cvResultMat.at<float>(2, 1);

			glMatrix[8] = cvResultMat.at<float>(0, 2);
			glMatrix[9] = cvResultMat.at<float>(1, 2);
			glMatrix[10] = cvResultMat.at<float>(2, 2);


			//移動ベクトルを入れる
			float ajast = 0.45f;
			
			glMatrix[12] = cvResultMat.at<float>(0, 3) * ajast;
			glMatrix[13] = cvResultMat.at<float>(1, 3) * ajast;
			glMatrix[14] = cvResultMat.at<float>(2, 3);

			//雪だるまの描写
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();

			glPushMatrix();
			glMultMatrixf(glMatrix);
			glRotated(180, 1, 0, 0);
			glScalef(0.005f, 0.005f, 0.005f);
			renderSnowman(id);
			glPopMatrix();
		}
		
		// カラーバッファを入れ替える
		glfwSwapBuffers(window);

		// イベントを取り出す
		//glfwWaitEvents();
	}
}


/*-------------------------------------
	OpenGLの初期化
--------------------------------------*/
void initialize() {

	glClearColor(0.8f, 0.8f, 0.9f, 1.0f);// 背景色の設定
	glClearDepth(1.0);
	glEnable(GL_DEPTH_TEST);// デプスバッファの有効化。	
	glEnable(GL_CULL_FACE);// カリングの有効化。
	glCullFace(GL_FRONT);// カリング。


	glEnable(GL_LIGHTING);// ライティングの有効化。
	glEnable(GL_LIGHT0);// 光源0 を有効化。
	glEnable(GL_LIGHT1);// 光源1 を有効化。
	
	glLightfv(GL_LIGHT1, GL_DIFFUSE, aLightColor);// 光源1の色を設定。
	glLightfv(GL_LIGHT1, GL_SPECULAR, aLightColor);// 光源1の色を設定。
	
	glfwSwapInterval(1);// モニタとの同期

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

}
/*-------------------------------------
バックグラウンドの描写
--------------------------------------*/
void display(GLFWwindow* window, const cv::Mat &img_bgr)
{
	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();  //行列の初期化
	gluOrtho2D(0.0, g_windowWidth, 0.0, g_windowHeight);//(0,0)を左下、(640,480)を右上とする
	glRasterPos2i(0, g_windowHeight - 1);//(0,479)を基準にする（左上）
	glPixelZoom(1, -1);
	glDrawPixels(g_windowWidth, g_windowHeight, GL_BGR_EXT, GL_UNSIGNED_BYTE, img_bgr.data);
	glPopMatrix();

	glEnable(GL_DEPTH_TEST);
	glClear(GL_DEPTH_BUFFER_BIT);
}
/*-------------------------------
	球の描写
--------------------------------*/
void renderSphere(GLfloat x, GLfloat y, GLfloat z, double r, const GLfloat* material)
{
	glPushMatrix();

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, material);
	glTranslatef(x, y, z);
	glRotatef(0, 0, 0, 0);
	drawSphere(r, 50, 50);	

	glPopMatrix();
}


/*-------------------------------------
	人参の描写
--------------------------------------*/

void renderCarrot() {
	glPushMatrix();

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, carrotMaterial);
	glTranslatef(0.0f, 0.0f, 1.2f);
	glRotatef(90, 0, 1, 0);
	drawCone(0.3f, 2.0f, 5, 5);

	glPopMatrix();
}

/*----------------------------------------------------
   大地の描画
----------------------------------------------------*/
void Ground(void) {
	glPushMatrix();

	double ground_max_x = 300.0;
	double ground_max_y = 300.0;
	//glColor3d(0.8, 0.8, 0.8);  // 大地の色
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, snowMaterial);
	glBegin(GL_LINES);
	for (double ly = -ground_max_y; ly <= ground_max_y; ly += 1.0) {
		if (ly == 0) {
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, carrotMaterial);
		}
		else {
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, snowMaterial);
		}
		glVertex3d(-ground_max_x, ly, 0);
		glVertex3d(ground_max_x, ly, 0);
	}
	for (double lx = -ground_max_x; lx <= ground_max_x; lx += 1.0) {
		glVertex3d(lx, ground_max_y, 0);
		glVertex3d(lx, -ground_max_y, 0);
	}
	glEnd();

	glPopMatrix();
}


/*-------------------------------------
	雪だるまの描写
--------------------------------------*/
void renderSnowman(int id) {
	system("cls");
	cout << id << endl;
	if (id == 4200) {
		renderSphere(0.0f, 0.0f, 0.0f, 1.0, redMaterial);//体の下の部分
		renderSphere(0.0f, 0.0f, 1.0f, 0.7, redMaterial);//体の上の部分
		renderSphere(0.7f, 0.3f, 1.4f, 0.1, eyeMaterial);
		renderSphere(0.7f, -0.3f, 1.4f, 0.1, eyeMaterial);
		renderCarrot();
	}
	else {
		renderSphere(0.0f, 0.0f, 0.0f, 1.0, snowMaterial);//体の下の部分
		renderSphere(0.0f, 0.0f, 1.0f, 0.7, snowMaterial);//体の上の部分
		renderSphere(0.7f, 0.3f, 1.4f, 0.1, eyeMaterial);
		renderSphere(0.7f, -0.3f, 1.4f, 0.1, eyeMaterial);
		renderCarrot();
	}
}