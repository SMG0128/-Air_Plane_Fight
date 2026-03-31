#include <easyx.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <time.h>
#pragma comment(lib, "Winmm.lib")
#define _CRT_SECURE_NO_WARNINGS

#define ScreenWidth 400
#define ScreenHeight 800

#define PlaneSize 50

#define EnemyNum 8
#define EnemySpeed 1.0

#define BulletNum 10

typedef struct Pos
{
    int x;
    int y;
}pos;

typedef struct Plane
{
    pos PlanePos;
	pos PlaneBulletPos[BulletNum];
    int bulletlen;
	int bulletspeed;
}plane;
plane MyPlane;
plane EnemyPlane[EnemyNum];
int enemylen;
static time_t start, end;

//IMAGE img[3];
int score = 0;
void initgame();
void drawgame();
void updategame();
void initenemy();

int main()
{
	//loadimage(&img[0], "img/背景.png", ScreenWidth,ScreenHeight);
	//loadimage(&img[1], "img/飞机.png", PlaneSize, PlaneSize);
	//loadimage(&img[2], "img/敌机.png"，PlaneSize, PlaneSize);


	initgame();
	while (true)
	{
		drawgame();
		updategame();
		Sleep(1000/60);
	}
	return 0;
}

void initgame()
{
	initgraph(ScreenWidth, ScreenHeight);
	score = 0;

	srand((unsigned)time(NULL));
	
	MyPlane.bulletlen = 0;
	MyPlane.bulletspeed = 3;
	MyPlane.PlanePos.x = ScreenWidth / 2 - PlaneSize / 2;
	MyPlane.PlanePos.y = ScreenHeight - PlaneSize;

	enemylen = 0;
	start = time(NULL);
}

void drawgame()
{
	BeginBatchDraw();

    // 清屏（黑色背景）
    setfillcolor(RGB(128,128,128));
    solidrectangle(0, 0, ScreenWidth, ScreenHeight);

    // 我方飞机：蓝色正三角形，中心在 MyPlane.PlanePos
    {
		POINT pts[3];           // 1. 定义3个点 → 三角形需要3个顶点
		int cx = MyPlane.PlanePos.x;  // 2. 飞机中心点 X 坐标
		int cy = MyPlane.PlanePos.y;  // 3. 飞机中心点 Y 坐标
		int half = PlaneSize / 2;     // 4. 飞机大小的一半（用来算顶点）
        pts[0].x = cx;       pts[0].y = cy - half; // 上顶点
        pts[1].x = cx - half; pts[1].y = cy + half; // 左下
        pts[2].x = cx + half; pts[2].y = cy + half; // 右下
        setfillcolor(RGB(0, 120, 255));
        setlinecolor(RGB(0, 120, 255));
        solidpolygon(pts, 3);
    }

    // 敌方飞机：红色倒三角形
    for (int i = 0; i < enemylen; i++)
    {
        POINT epts[3];
        int cx = EnemyPlane[i].PlanePos.x;
        int cy = EnemyPlane[i].PlanePos.y;
        int half = PlaneSize / 2;
        epts[0].x = cx - half; epts[0].y = cy - half; // 左上
        epts[1].x = cx + half; epts[1].y = cy - half; // 右上
        epts[2].x = cx;        epts[2].y = cy + half; // 下顶点（倒三角）
		setlinecolor(BLACK);
		setfillcolor(RED);
		setlinestyle(PS_SOLID, 2); // 边框加粗 2 像素（更明显）
        solidpolygon(epts, 3);//画实心三角形
		polygon(epts, 3);//画空心三角形，实现边框效果
    }

    // 子弹
    setfillcolor(WHITE);
    setlinecolor(WHITE);
    for (int i = 0; i < MyPlane.bulletlen; i++)
    {
        solidcircle(MyPlane.PlaneBulletPos[i].x, MyPlane.PlaneBulletPos[i].y, PlaneSize / 4);
    }

    // 分数文本
    RECT rect = { 0 , PlaneSize, ScreenWidth , ScreenHeight };
    setbkmode(TRANSPARENT);
    char str[30];
    sprintf_s(str, "Score: %d", score);
    settextcolor(WHITE);
    drawtext(str, &rect, DT_CENTER | DT_TOP);

    EndBatchDraw();
}

void updategame()
{
	if (GetAsyncKeyState('W') & 0x8000)
	{
		MyPlane.PlanePos.y -= 4;
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{
		MyPlane.PlanePos.y += 4;
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		MyPlane.PlanePos.x -= 4;
	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		MyPlane.PlanePos.x += 4;
	}
	if (_kbhit())
	{
		if (_getch() == ' ')
		{
			if(MyPlane.bulletlen < BulletNum)
			{
				//PlaySound("img/1.wav", NULL, SND_FILENAME | SND_ASYNC |SND_NOWAIT);
				MyPlane.PlaneBulletPos[MyPlane.bulletlen] = MyPlane.PlanePos;
				MyPlane.bulletlen++;
			}
		}
	}

	for (int i = 0;i < enemylen;i++)
	{
		EnemyPlane[i].PlanePos.y += 2;
	}

	for (int i = 0; i < MyPlane.bulletlen; i++)
	{
		MyPlane.PlaneBulletPos[i].y -= MyPlane.bulletspeed;
	}
	initenemy();
}

void initenemy()
{
	end = time(NULL);
	double elapsed = difftime(end, start);
	if (elapsed >= EnemySpeed)
	{
		if(enemylen < EnemyNum)
		{
			EnemyPlane[enemylen].PlanePos.x = rand() % (ScreenWidth - 2 * PlaneSize) + PlaneSize / 2;
			EnemyPlane[enemylen].PlanePos.y = -PlaneSize;
			enemylen++;
		}
	}
}