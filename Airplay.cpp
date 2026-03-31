#include <easyx.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <time.h>
#pragma comment(lib, "Winmm.lib")
#define _CRT_SECURE_NO_WARNINGS //安全警告解除，出现在了sprintf上，改了也没用，直接用sprintf_s了

#define ScreenWidth 400
#define ScreenHeight 800

#define PlaneSize 50

#define EnemyNum 8
#define EnemySpeed 1.0

#define BulletNum 10

typedef struct Pos //坐标参
{
    int x;
    int y;
}pos;

typedef struct Plane //飞机参
{
    pos PlanePos;
	pos PlaneBulletPos[BulletNum];
    int bulletlen;
	int bulletspeed;
}plane;
plane MyPlane;
plane EnemyPlane[EnemyNum];//因为有多个敌机，所以用数组存储敌机数据
int enemylen;//记录当前生成敌机数量
static time_t start, end;//敌机生成计时器

//IMAGE img[3];
int score = 0;
void initgame();
void drawgame();
void updategame();
void initenemy();
bool areInierSecting(pos c1, pos c2, int radius);
void destroyenemy();
void destroybullet();

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
		Sleep(1000 / 60);//控制帧率，60帧每秒，帧率越高游戏运行越快
	}
	return 0;
}

void initgame()//初始化游戏数据，相当于重置游戏，重新开始游戏时调用，或者第一次进入游戏时调用
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

void drawgame()//游戏界面的绘制函数
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

void updategame()//更新函数，游戏的核心逻辑存放地，操作，敌机生成，子弹移动，碰撞检测等都在这里实现
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
	//基于控制台输入的发射方式，已废弃
	//if (_kbhit())
	//{
	//	if (_getch() == ' ')
	//	{
	//		if(MyPlane.bulletlen < BulletNum)
	//		{
	//			//PlaySound("img/1.wav", NULL, SND_FILENAME | SND_ASYNC |SND_NOWAIT);
	//			MyPlane.PlaneBulletPos[MyPlane.bulletlen].x = MyPlane.PlanePos.x;
	//			MyPlane.PlaneBulletPos[MyPlane.bulletlen].y = MyPlane.PlanePos.y - PlaneSize / 2;
	//			MyPlane.bulletlen++;
	//		}
	//	}
	//}
	// 鼠标左键发射
	if (GetAsyncKeyState(VK_LBUTTON) & 1)
	{
		if (MyPlane.bulletlen < BulletNum)
		{
			// 从机头发射
			MyPlane.PlaneBulletPos[MyPlane.bulletlen].x = MyPlane.PlanePos.x;
			MyPlane.PlaneBulletPos[MyPlane.bulletlen].y = MyPlane.PlanePos.y - PlaneSize / 2;
			MyPlane.bulletlen++;
		}
	}

	for (int i = 0;i < enemylen;i++)//敌机移动
	{
		EnemyPlane[i].PlanePos.y += 2;
	}
	//子弹移动
	for (int i = 0; i < MyPlane.bulletlen; ++i) {
        MyPlane.PlaneBulletPos[i].y -= MyPlane.bulletspeed;
        if (MyPlane.PlaneBulletPos[i].y < -10) {
            MyPlane.PlaneBulletPos[i] = MyPlane.PlaneBulletPos[MyPlane.bulletlen - 1];
            MyPlane.bulletlen--;
            i--;
        }
    }
	initenemy();
	destroyenemy();
	destroybullet();
}

void initenemy()//敌人生成函数
{
	end = time(NULL);//计时初始化，每次调用initenemy都会更新end的值，保证每次生成敌机的时间间隔都能被正确计算
	double elapsed = difftime(end, start);//计算时间差，单位为秒
	if (elapsed >= EnemySpeed)
	{
		if (enemylen < EnemyNum)
		{
			EnemyPlane[enemylen].PlanePos.x = rand() % (ScreenWidth - 2 * PlaneSize) + PlaneSize / 2;
			EnemyPlane[enemylen].PlanePos.y = -PlaneSize;
			enemylen++;
			start = time(NULL); // 重置计时，保证间隔生效
		}
	}
}

void destroyenemy()//杀敌函数
{
	for (int i = 0; i < enemylen; i++)
	{
		if (areInierSecting(MyPlane.PlanePos, EnemyPlane[i].PlanePos, PlaneSize/3))
		{
			if (IDYES == MessageBox(GetHWnd(), "Game Over , Try Again ?", "Warning", MB_YESNO))//提示框，询问玩家是否重新开始游戏
			{
				initgame();
			}
			else
			{
				exit(0);
			}
		}

		if (EnemyPlane[i].PlanePos.y > ScreenHeight)
		{
			for (int j = i; j < enemylen - 1; j++)
			{
				EnemyPlane[j] = EnemyPlane[j + 1];
			}
			enemylen--;//刷新敌机数量，因为敌机已经被销毁了，所以数量要减1，否则会出现越界访问；更新敌机生成的关键
			i--;//刷新敌机索引
		}

	}
}

bool areInierSecting(pos c1, pos c2, int radius)//检测碰撞
{
    int dx = c1.x - c2.x;
    int dy = c1.y - c2.y;
    return (dx*dx + dy*dy) <= radius*radius;
}

void destroybullet()//子弹销毁函数，检测子弹与敌机的碰撞，并销毁子弹和敌机；越界销毁子弹
{
	for (int i = 0; i < MyPlane.bulletlen; i++)
	{
		for(int j = 0; j < enemylen; j++)
		{
			if(areInierSecting(MyPlane.PlaneBulletPos[i], EnemyPlane[j].PlanePos, PlaneSize/4+PlaneSize/3))
			{
				for (int x = i; x < MyPlane.bulletlen - 1; x++)
				{
					MyPlane.PlaneBulletPos[x] = MyPlane.PlaneBulletPos[x + 1];
				}
				for (int x = j;x < enemylen - 1; x++)
				{
					EnemyPlane[x] = EnemyPlane[x + 1];
				}
				enemylen--;
				MyPlane.bulletlen--;
				j--;
				score += 100;
			}
		}
		if (MyPlane.PlaneBulletPos[i].y < 0)
		{
			for (int x = i; x < MyPlane.bulletlen - 1; x++)
			{
				MyPlane.PlaneBulletPos[x] = MyPlane.PlaneBulletPos[x + 1];
			}
			MyPlane.bulletlen--;
			i--;
		}
	}
}
