#define _CRT_SECURE_NO_WARNINGS // 安全警告解除，出现在了sprintf上，改了也没用，直接用sprintf_s了
#include <easyx.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <math.h>
#include <stdarg.h>
#include<chrono>
#include <time.h>
#include<graphics.h>
#pragma comment(lib, "Winmm.lib")


#define ScreenWidth 400
#define ScreenHeight 800

#define PlaneSize 50

#define EnemyNum 8
#define EnemySpeed 1.0

#define BulletNum 10

bool doubleBulletActive = false;
std::chrono::steady_clock::time_point doubleBulletEnd;

typedef struct Pos // 坐标参
{
	int x;
	int y;
}pos;

typedef struct Plane // 飞机参
{
	pos PlanePos;
	pos PlaneBulletPos[BulletNum * 2];
	int bulletlen;
	int bulletspeed;
	int hp;//血量
    int id; // unique id for enemy or plane
	int PlaneBulletId[BulletNum * 2]; // ids for each bullet slot
    std::chrono::steady_clock::time_point lastHitTime;
}plane;
plane MyPlane;
plane EnemyPlane[EnemyNum];// 因为有多个敌机，所以用数组存储敌机数据
int enemylen;// 记录当前生成敌机数量
int nextBulletId = 1;
int nextEnemyId = 1;
static time_t start, end;// 敌机生成计时器

//IMAGE img[3];
int score = 0;
// 简单的文件日志（用于调试）
void gamelog(const char* fmt, ...)
{
	FILE* f = fopen("game_log.txt", "a");
	if (!f) return;
	va_list args;
	va_start(args, fmt);
	vfprintf(f, fmt, args);
	fprintf(f, "\n");
	va_end(args);
	fclose(f);
}

void initgame();
void drawgame();
void updategame();
void initenemy();
bool areInierSecting(pos c1, pos c2, int radius);
void destroyenemy();
void destroybullet();
void startgame();
void pausegame();
void startDoubleBullet();

int main()
{
	//loadimage(&img[0], "img/背景.png", ScreenWidth,ScreenHeight);
	//loadimage(&img[1], "img/飞机.png", PlaneSize, PlaneSize);
	//loadimage(&img[2], "img/敌机.png"，PlaneSize, PlaneSize);
	initgame();     // 先初始化图形和游戏状态（会调用 initgraph）
	startgame();    // 现在可以安全调用 BeginBatchDraw()
	while (true)
	{
		drawgame();
		updategame();
		Sleep(1000 / 60);// 控制帧率，60帧每秒，帧率越高游戏运行越快
	}
	return 0;
}

void initgame()// 初始化游戏数据，相当于重置游戏，重新开始游戏时调用，或者第一次进入游戏时调用
{
	initgraph(ScreenWidth, ScreenHeight);
	score = 0;

	// clear previous log
	{
		FILE* f = fopen("game_log.txt", "w");
		if (f) {
			fprintf(f, "--- Game Start ---\n");
			fclose(f);
		}
	}

	srand((unsigned)time(NULL));

	MyPlane.bulletlen = 0;
	MyPlane.bulletspeed = 3;
	MyPlane.PlanePos.x = ScreenWidth / 2 - PlaneSize / 2;
	MyPlane.PlanePos.y = ScreenHeight - PlaneSize;
    // 重置ID计数器
	nextBulletId = 1;
	nextEnemyId = 1;
	
	enemylen = 0;
	start = time(NULL);
}

void drawgame()// 游戏界面的绘制函数
{
	BeginBatchDraw();

	// 清屏（灰色背景）
	setfillcolor(RGB(128, 128, 128));
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
		solidpolygon(epts, 3);// 画实心三角形
		polygon(epts, 3);// 画空心三角形，实现边框效果
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

void updategame()// 更新函数，游戏的核心逻辑存放地，操作，敌机生成，子弹移动，碰撞检测等都在这里实现
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
	if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
	{
		pausegame();
	}
	if (GetAsyncKeyState('B') & 0x8000)
	{
		startDoubleBullet();
	}

	if (doubleBulletActive) {// 双倍子弹持续时间检测
		auto now = std::chrono::steady_clock::now();
		if (now >= doubleBulletEnd) {
			doubleBulletActive = false;
		}
	}
	// 基于控制台输入的发射方式，已废弃
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
	if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
	{
		const int MAX_BULLETS = BulletNum * 2;
		if (doubleBulletActive) {
			// 需要至少两个空位
			if (MyPlane.bulletlen <= MAX_BULLETS - 2) {
               MyPlane.PlaneBulletPos[MyPlane.bulletlen].x = MyPlane.PlanePos.x - PlaneSize / 4;
				MyPlane.PlaneBulletPos[MyPlane.bulletlen].y = MyPlane.PlanePos.y - PlaneSize / 2;
				MyPlane.PlaneBulletId[MyPlane.bulletlen] = nextBulletId++;
				MyPlane.bulletlen++;
				MyPlane.PlaneBulletPos[MyPlane.bulletlen].x = MyPlane.PlanePos.x + PlaneSize / 4;
				MyPlane.PlaneBulletPos[MyPlane.bulletlen].y = MyPlane.PlanePos.y - PlaneSize / 2;
				MyPlane.PlaneBulletId[MyPlane.bulletlen] = nextBulletId++;
				MyPlane.bulletlen++;
			}
		}
		else {
			if (MyPlane.bulletlen < BulletNum * 2) { // 或者使用 MAX_BULLETS
               MyPlane.PlaneBulletPos[MyPlane.bulletlen].x = MyPlane.PlanePos.x;
				MyPlane.PlaneBulletPos[MyPlane.bulletlen].y = MyPlane.PlanePos.y - PlaneSize / 2;
				MyPlane.PlaneBulletId[MyPlane.bulletlen] = nextBulletId++;
				MyPlane.bulletlen++;
			}
		}
	}
		for (int i = 0; i < enemylen; i++)// 敌机移动
		{
			EnemyPlane[i].PlanePos.y += 2;
		}
		// 子弹移动
		for (int i = 0; i < MyPlane.bulletlen; ++i) {
			MyPlane.PlaneBulletPos[i].y -= MyPlane.bulletspeed;
			if (MyPlane.PlaneBulletPos[i].y < -10) {
              MyPlane.PlaneBulletPos[i] = MyPlane.PlaneBulletPos[MyPlane.bulletlen - 1];
				MyPlane.PlaneBulletId[i] = MyPlane.PlaneBulletId[MyPlane.bulletlen - 1];
				MyPlane.bulletlen--;
				i--;
			}
		}
		initenemy();
		destroyenemy();
		destroybullet();
	
}

void initenemy()// 敌人生成函数
{
	end = time(NULL);// 计时初始化，每次调用initenemy都会更新end的值，保证每次生成敌机的时间间隔都能被正确计算
	double elapsed = difftime(end, start);// 计算时间差，单位为秒
	if (elapsed >= EnemySpeed)
	{
		if (enemylen < EnemyNum)
		{
			EnemyPlane[enemylen].PlanePos.x = rand() % (ScreenWidth - 2 * PlaneSize) + PlaneSize / 2;
			EnemyPlane[enemylen].PlanePos.y = -PlaneSize;
         // 初始化新生成敌机的生命值
			EnemyPlane[enemylen].hp = 3;
            EnemyPlane[enemylen].id = nextEnemyId++;
			gamelog("Spawn enemy id=%d idx=%d at (%d,%d) hp=%d", EnemyPlane[enemylen].id, enemylen, EnemyPlane[enemylen].PlanePos.x, EnemyPlane[enemylen].PlanePos.y, EnemyPlane[enemylen].hp);
			enemylen++;
			start = time(NULL); // 重置计时，保证间隔生效
		}
	}
}

void destroyenemy()// 杀敌函数
{
	for (int i = 0; i < enemylen; i++)
	{
		if (areInierSecting(MyPlane.PlanePos, EnemyPlane[i].PlanePos, PlaneSize / 3))
		{
			if (IDYES == MessageBox(GetHWnd(), "Game Over , Try Again ?", "Warning", MB_YESNO))// 提示框，询问玩家是否重新开始游戏
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
			enemylen--;// 刷新敌机数量，因为敌机已经被销毁了，所以数量要减1，否则会出现越界访问；更新敌机生成的关键
			i--;// 刷新敌机索引
		}

	}
}

bool areInierSecting(pos c1, pos c2, int radius)// 检测碰撞
{
	int dx = c1.x - c2.x;
	int dy = c1.y - c2.y;
	return (dx * dx + dy * dy) <= radius * radius;
}

void destroybullet()// 子弹销毁函数，检测子弹与敌机的碰撞，并销毁子弹和敌机；越界销毁子弹
{
	for (int i = 0; i < MyPlane.bulletlen; i++)
	{
		if (MyPlane.PlaneBulletPos[i].y < 0) continue;
		for (int j = 0; j < enemylen; j++)
		{
			if (EnemyPlane[j].hp <= 0) continue;
			if (areInierSecting(MyPlane.PlaneBulletPos[i], EnemyPlane[j].PlanePos, PlaneSize / 4 + PlaneSize / 3))
			{
                auto now = std::chrono::steady_clock::now();
				// ignore hits that occur too quickly on the same enemy (debounce)
				if (now - EnemyPlane[j].lastHitTime < std::chrono::milliseconds(80)) {
					gamelog("Ignored rapid hit on Enemy id=%d idx=%d", EnemyPlane[j].id, j);
					continue;
				}
                // capture positions for accurate logging before we modify arrays
				int bx = MyPlane.PlaneBulletPos[i].x;
				int by = MyPlane.PlaneBulletPos[i].y;
				int ex = EnemyPlane[j].PlanePos.x;
				int ey = EnemyPlane[j].PlanePos.y;
				EnemyPlane[j].hp--;
                EnemyPlane[j].lastHitTime = now;
				gamelog("Bullet id=%d idx=%d (%d,%d) hit Enemy id=%d idx=%d (%d,%d), hp now %d",
					MyPlane.PlaneBulletId[i], i, bx, by, EnemyPlane[j].id, j, ex, ey, EnemyPlane[j].hp);
                // 立即移除该子弹，防止索引重用导致同一帧或后续检测中再次命中
          MyPlane.PlaneBulletPos[i] = MyPlane.PlaneBulletPos[MyPlane.bulletlen - 1];
			MyPlane.PlaneBulletId[i] = MyPlane.PlaneBulletId[MyPlane.bulletlen - 1];
			MyPlane.bulletlen--;
			i--;
				break;
			}
		}
	}
	int newenemylen = 0;
	for (int j = 0; j < enemylen; j++)
	{
        if (EnemyPlane[j].hp > 0)
		{
			EnemyPlane[newenemylen] = EnemyPlane[j];
			// keep id consistent
			EnemyPlane[newenemylen].id = EnemyPlane[j].id;
			newenemylen++;
		}
		else
		{
		   score += 100;// 每击落一架敌机，得100分
			gamelog("Enemy id=%d idx=%d destroyed, score=%d", EnemyPlane[j].id, j, score);
		}
	}
	// 更新当前敌机数量，移除已被击落的敌机
	enemylen = newenemylen;

	int newbulletlen = 0;
	for (int i = 0; i < MyPlane.bulletlen; i++)
	{
		if (MyPlane.PlaneBulletPos[i].y >= 0)
		{
            MyPlane.PlaneBulletPos[newbulletlen] = MyPlane.PlaneBulletPos[i];
			MyPlane.PlaneBulletId[newbulletlen] = MyPlane.PlaneBulletId[i];
			newbulletlen++;
		}
	}
	// 更新当前子弹数量，移除已出界或被标记为销毁的子弹
	MyPlane.bulletlen = newbulletlen;
	
}

void startgame()
{
	BeginBatchDraw();
	setfillcolor(RGB(128, 128, 128));
	solidrectangle(0, 0, ScreenWidth, ScreenHeight);
	RECT rect = { 0 , 0, ScreenWidth , ScreenHeight };
	setbkmode(TRANSPARENT);
	settextcolor(WHITE);
	drawtext("Press Any Key To Start", &rect, DT_CENTER | DT_VCENTER);
	EndBatchDraw();
	while (!_kbhit());
}

void pausegame()
{
	BeginBatchDraw();
	setfillcolor(RGB(128, 128, 128));
	solidrectangle(0, 0, ScreenWidth, ScreenHeight);
	RECT rect = { 0 , 0, ScreenWidth , ScreenHeight };
	setbkmode(TRANSPARENT);
	settextcolor(WHITE);
	drawtext("Press Any Key To Back To The Game Or Push Esc For More Option", &rect, DT_CENTER | DT_VCENTER);
	EndBatchDraw();
	//if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
	//{
	//	if (IDYES == MessageBox(GetHWnd(), "Leave Or Try Again?", "Choice", MB_YESNO))
	//	{
	//		exit(0);
	//	}
	//	else
	//	{
	//		initgame();
	//		startgame();
	//	}
	//}
	int ret = MessageBox(GetHWnd(), "Leave Or Try Again?", "Game Paused", MB_YESNO);
	if (ret == IDYES) exit(0);
	else { initgame(); startgame(); }

	while (!_kbhit());
}

void startDoubleBullet() {
	doubleBulletActive = true;
	doubleBulletEnd = std::chrono::steady_clock::now() + std::chrono::seconds(5);
}