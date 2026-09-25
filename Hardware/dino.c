#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "Key.h"
#include <stdlib.h>
#include <math.h>
#include "Delay.h"

struct Object_Position{
	uint8_t minX,minY,maxX,maxY;
};//记录物体x，y最大和最小值，用于碰撞判断

/*显示分数函数，部分设计在此.c文件下面的Dino_Tick(void)中
 *Score记录分数的累计值
 *
 */
int Score;
void Show_Score(void)
{
	OLED_ShowNum(98,0,Score,5,OLED_6X8);
}

/*处理地面移动的设计，部分设计在此.c文件下面的Dino_Tick(void)中
 *这个地面移动的函数设计有点绕，可以多看几遍视频
 *实现每隔地面每隔20ms向左移动一个像素点
 *在中断函数中使用Ground_Count和Ground_Pos进行计数和判断
 *显示地面的数组一共有256个数据
 *直接调用了OLED.c中的OLED_DisplayBuf[7][128]进行显示处理
 *设计中，变量i是OLED横向128个像素依次显示的标志位，由于OLED横向只有128个像素，而地面数组有256个数据，
 *因而显示过程使用变量Ground_Pos对数组的运行进行分段（前128和后128），分情况进行地面移动的显示设计
 *Ground_Pos为地面数组移动的标志位
 *
 */
uint16_t Ground_Pos;
void Show_Ground(void)
{
	if(Ground_Pos<128)//前半段0~127在前，后边段128~255在后
	{
			for(uint8_t i=0;i<128;i++)
		{
			OLED_DisplayBuf[7][i]=Ground[i+Ground_Pos];
		}
	}
	else//前半段0~127在后，后半段128~255在前
	{
			for(uint8_t i=0;i<255-Ground_Pos;i++)
		{
			OLED_DisplayBuf[7][i]=Ground[i+Ground_Pos];
		}
			for(uint8_t i=255-Ground_Pos;i<128;i++)
		{
			OLED_DisplayBuf[7][i]=Ground[i-(255-Ground_Pos)];
		}
	}
	
}


/*障碍物显示移动函数，部分设计在此.c文件下面的Dino_Tick(void)中
 *实现三种不同障碍物的随机显示，当一个前一个障碍物移出屏幕时，后一个才出现
 *barrier_flag为随机数，0~2，分别代表三个障碍物
 *Barrier_Pos为障碍物移动的标志位
 *
 *
 */
uint8_t barrier_flag;
uint8_t Barrier_Pos;
struct Object_Position barrier;//障碍物的坐标

void Show_Barrier(void)
{
	if(Barrier_Pos>=143)//等一个障碍物移出屏幕后，再调用随机函数进行障碍物生成，否则障碍物在移动过程中会一直变化
	{
		barrier_flag=rand()%3;
	}
	
	OLED_ShowImage(127-Barrier_Pos,44,16,18,Barrier[barrier_flag]);
	
	//障碍物的坐标赋值
	barrier.minX=127-Barrier_Pos;
	barrier.maxX=143-Barrier_Pos;
	barrier.minY=44;
	barrier.maxY=62;
}

/*显示云朵移动的函数，部分设计在此.c文件下面的Dino_Tick(void)中
 *Cloud_Pos为云朵移动的标志位
 *
 */
uint8_t Cloud_Pos;
void Show_Cloud(void)
{
	OLED_ShowImage(127-Cloud_Pos,9,16,8,Cloud);
}

/*小恐龙移动和跳跃函数，部分设计在此.c文件下面的Dino_Tick(void)中
 *跳跃动作完成一次需要时间为1s
 */
uint8_t dino_jump_flag=0;//0:奔跑，1:跳跃
extern uint8_t KeyNum;
uint16_t jump_t;
uint8_t Jump_Pos;
extern double pi;

struct Object_Position dino;//小恐龙的坐标

void Show_Dino(void)
{
	KeyNum=Key_GetNum();
	if(KeyNum==1)dino_jump_flag=1;
	Jump_Pos=28*sin((float)(pi*jump_t/1000));//小恐龙跳跃的轨迹函数，sin函数
	
	if(dino_jump_flag==0)//每隔50ms进行小恐龙有左右脚变化的移动
	{
		if(Cloud_Pos%2==0)OLED_ShowImage(0,44,16,18,Dino[0]);//后腿着地
		else OLED_ShowImage(0,44,16,18,Dino[1]);//前腿着地
	}
	
	else//跳跃显示
	{
		OLED_ShowImage(0,44-Jump_Pos,16,18,Dino[2]);
	}
	//小恐龙的坐标赋值，横坐标不变化，纵坐标变化
	dino.minX=0;
	dino.maxX=16;
	dino.minY=44-Jump_Pos;
	dino.maxY=62-Jump_Pos;
}

/*碰撞检测函数
 *原理：看if函数的条件，画两个a，b的矩形且位置重叠的图进行辅助理解
 *
 *
 */
int isColliding(struct Object_Position *a,struct Object_Position *b)
{
	if((a->maxX>b->minX)&&(a->minX<b->maxX)&&(a->maxY>b->minY)&&(a->minY<b->maxY))
	{
		OLED_Clear();
		OLED_ShowString(28,24,"Game Over",OLED_8X16);
		OLED_Update();
		Delay_s(1);
		OLED_Clear();
		OLED_Update();
		return 1;
	}
	return 0;
}

/*小恐龙游戏所有UI调用显示函数
 *执行到硬件
 *
 *
 */
int DinoGame_Animation(void)
{
	while(1)
	{
		int return_flag;
		OLED_Clear();
		Show_Score();
		Show_Ground();
		Show_Barrier();
		Show_Cloud();
		Show_Dino();
		OLED_Update();
		return_flag=isColliding(&dino,&barrier);
		if(return_flag==1)//当碰撞发生时，直接退出游戏
		{
			return 0;
		}
	}
	
}

/*小恐龙游戏中断函数调用的计时函数
 *
 *
 */
void Dino_Tick(void)
{
	static uint8_t Score_Count,Ground_Count,Cloud_Count;//云层、地面、分数的记数变量
	Score_Count++;
	Ground_Count++;
	Cloud_Count++;

	if(Score_Count>=100)//每隔100ms，Score加一，分数加一
	{
		Score_Count=0;
		Score++;
	}
	
	if(Ground_Count>=20)//每隔20ms，Ground_Pos加一次（地面移动一次）
	{
		Ground_Count=0;
		Ground_Pos++;
		Barrier_Pos++;
		if(Ground_Pos>=256)Ground_Pos=0;//表示地面数组完全执行了一遍
		if(Barrier_Pos>=144)Barrier_Pos=0;//表示障碍物从右开始移动到完全移出屏幕
	}
	
	if(Cloud_Count>=50)//每隔50ms
	{
		Cloud_Count=0;
		Cloud_Pos++;
		if(Cloud_Pos>=200)Cloud_Pos=0;//200的原因为UP想要云朵移出屏幕一段时间再出现
	}
	
	if(dino_jump_flag==1)
	{
		jump_t++;
		if(jump_t>=1000)
		{
			jump_t=0;
			dino_jump_flag=0;
		}
	}

}

//每次进入游戏，所有标志位进行清零
void DinoGame_Pos_Init(void)
{
	Score=Ground_Pos=Barrier_Pos=Cloud_Pos=Jump_Pos=0;
}
