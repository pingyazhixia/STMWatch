#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "MyRTC.h"                                                                                                                                                                                                     
#include "Key.h"
#include "LED.h"
#include "SetTime.h"
#include "menu.h"
#include "MPU6050.h"
#include "Delay.h"
#include <math.h>
#include "dino.h"
#include "AD.h"


uint8_t KeyNum;

void Peripheral_Init(void)//外设初始化
{
	MyRTC_Init();
	Key_Init();
	LED_Init();
	MPU6050_Init();
	AD_Init();
}



/*----------------------------------首页时钟-------------------------------------*/
uint16_t ADValue;//AD值
float VBAT;
int Battery_Capacity;//电量

void Show_Battery(void)
{
	int sum;
	for(int i=0;i<3000;i++)
	{
		ADValue=AD_GetValue();
		sum+=ADValue;
		
	}
	ADValue=sum/3000;
	VBAT=(float)ADValue/4095*3.3;
	Battery_Capacity=(ADValue-3276)*100/819;
	if(Battery_Capacity<0)Battery_Capacity=0;
	
	OLED_ShowNum(85,4,Battery_Capacity,3,OLED_6X8);
	OLED_ShowChar(103,4,'%',OLED_6X8);
	
	if(Battery_Capacity==100)OLED_ShowImage(110,0,16,16,Battery);//满电时的电池显示
	else if(Battery_Capacity>=10&&Battery_Capacity<100)//有电但不满电的显示（使用清屏函数进行实现
	{
		OLED_ShowImage(110,0,16,16,Battery);
		OLED_ClearArea((112+Battery_Capacity/10),5,(10-Battery_Capacity/10),6);
		OLED_ClearArea(85,4,6,8);
	}
	
	else//没电的显示
	{
		OLED_ShowImage(110,0,16,16,Battery);
		OLED_ClearArea(112,5,10,6);
		OLED_ClearArea(85,4,12,8);
	}
};


void Show_Clock_UI(void)
{
	MyRTC_ReadTime();
	OLED_Printf(0,0,OLED_6X8,"%d-%d-%d",MyRTC_Time[0],MyRTC_Time[1],MyRTC_Time[2]);//显示左上角年月日
	OLED_Printf(16,16,OLED_12X24,"%02d:%02d:%02d",MyRTC_Time[3],MyRTC_Time[4],MyRTC_Time[5]);//显示中间的时分秒
	OLED_ShowString(0,48,"菜单",OLED_8X16);
	OLED_ShowString(96,48,"设置",OLED_8X16);
}
//%d-%d-%d解释：`%d`：输出**十进制整数**，数字是多少就打印多少，不会补 0；`-`：就是原样输出的横杠符号
//%02d:%02d:%02d
//`d`：十进制整数
//`2`：**最少占 2 个字符宽度**
//`0`：如果数字不足 2 位，**左边补数字 0**；如果≥2 位，正常原样输出。
//`:`：原样输出冒号，用来分隔 时：分: 秒。

/*首部页面的“设置”“菜单”两个选项的光标选择函数
 *用标志位clkflag的值来进行判断：1：选择“菜单”；2：选择“设置”；3：确认选择功能
 *标志位clkflag还承担光标显示的标志作用
 *
 */
int clkflag=1;

int First_Page_Clock(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		
		if(KeyNum==1)//上一项
		{
			clkflag--;
			if(clkflag<=0)clkflag=2;//意思为“菜单”的上一项为“设置”
		}
		else if(KeyNum==2)//下一项
		{
			clkflag++;
			if(clkflag>=3)clkflag=1;//意思为“设置”的上一项为“菜单”
		}
		else if(KeyNum==3)//确认，退出First_Page_Clock(void)函数并把clkflag的值返回（后在主函数中赋值给clkflag1）
		{
			OLED_Clear();
			OLED_Update();
			return clkflag;
		}
		else if(KeyNum==4)//关机
		{
			GPIO_ResetBits(GPIOB, GPIO_Pin_13);
			GPIO_SetBits(GPIOB, GPIO_Pin_12);
		};
		switch(clkflag)
		{
			case 1://光标在菜单的两个字上面时
				Show_Clock_UI();
				OLED_ReverseArea(0,48,32,16);//屏幕显示反相函数
				OLED_Update();
				break;
			
			case 2://光标在设置的两个字上面时
				Show_Clock_UI();
				OLED_ReverseArea(96,48,32,16);
				OLED_Update();
				break;
		}
	}
}

/*----------------------------------设置界面-------------------------------------*/


//进入“设置”界面后的界面显示函数，显示“返回”图标以及"日期时间设置"字样
void Show_SettingPage_UI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
	OLED_ShowString(0,16,"日期时间设置",OLED_8X16);
}

/*在“设置”界面的使用光标进行选择的设置函数
 *与First_Page_Clock(void)功能和代码上相似
 *标志位setflag_temp（实时更新初始为0）来进行执行功能：1：退出SettingPage(void)；2：跳转到日期设置函数
 *标志位setflag承担光标显示的标志作用
 *同时，初始setflag=1，意味着光标初始在“返回”图标上
 */
int setflag=1;
int SettingPage(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		uint8_t setflag_temp=0;
		if(KeyNum==1)//上一项
		{
			setflag--;
			if(setflag<=0)setflag=2;//越下界判断
		}
		else if(KeyNum==2)//下一项
		{
			setflag++;
			if(setflag>=3)setflag=1;//越上界判断
		}
		else if(KeyNum==3)//确认
		{
			OLED_Clear();
			OLED_Update();
			setflag_temp=setflag;
		}
		
		if(setflag_temp==1){return 0;}//退出SettingPage(void)函数
		else if(setflag_temp==2){SetTime();}//进入日期设置函数
		
		switch(setflag)
		{
			case 1:
				Show_SettingPage_UI();//由于初始setflag=1，因而函数第一次循环运行会在此处，因而可以刷新界面
				OLED_ReverseArea(0,0,16,16);
				OLED_Update();
				break;
			
			case 2:
				Show_SettingPage_UI();
				OLED_ReverseArea(0,16,96,16);
				OLED_Update();
				break;
		}
	}
}


/*----------------------------------滑动菜单界面-------------------------------------*/


/*菜单界面滑动函数
 *选择框在中间不动，代表功能的七个图标动
 *
 */
//以下变量可以尝试整合成一个结构体
uint8_t pre_selection;//上次选择的选项
uint8_t target_selection;//目标选项
uint8_t x_pre=48;//上次选项的x坐标，也就是在屏幕中间
uint8_t Speed=4;//图标移动的速度
uint8_t move_flag;//开始移动的标志位，1表示开始移动，0表示停止移动

void Menu_Animation(void)
{
	OLED_Clear();
	OLED_ShowImage(42,10,44,44,Frame);//选择框在中间不动
	
	//左移动画设计
	if(pre_selection<target_selection)
	{
		x_pre-=Speed;
		if(x_pre==0)
		{
			pre_selection++;
			move_flag=0;
			x_pre=48;
		}
	}
	
	//右移动画设计
	if(pre_selection>target_selection)
	{
		x_pre+=Speed;
		if(x_pre==96)
		{
			pre_selection--;
			move_flag=0;
			x_pre=48;
		}
	}
	
	if(pre_selection>=1)
	{
		OLED_ShowImage(x_pre-48,16,32,32,Menu_Graph[pre_selection-1]);
	}
	
	if(pre_selection>=2)
	{
		OLED_ShowImage(x_pre-96,16,32,32,Menu_Graph[pre_selection-2]);
	}
	
	OLED_ShowImage(x_pre,16,32,32,Menu_Graph[pre_selection]);
	OLED_ShowImage(x_pre+48,16,32,32,Menu_Graph[pre_selection+1]);
	OLED_ShowImage(x_pre+96,16,32,32,Menu_Graph[pre_selection+2]);
	//以上为动画移动设计，有些绕，可以多看几遍
	OLED_Update();
}

/*按键控制设定pre_selection和target_selection的值
 *
 *
 */
void Set_Selection(uint8_t move_flag,uint8_t Pre_Selection,uint8_t Target_Selection)
{
	if(move_flag==1)
	{
		pre_selection=Pre_Selection;
		target_selection=Target_Selection;
		
	}
	Menu_Animation();
}

/*菜单到具体功能的转场函数
 *丝滑过度使用
 *但从具体功能回到菜单不行
 *
 *
 */
void MenuToFunction(void)
{
	for(uint8_t i=0;i<=6;i++)
	{
		OLED_Clear();//记得使用oled函数时必须清屏
			if(pre_selection>=1)
		{
			OLED_ShowImage(x_pre-48,16+8*i,32,32,Menu_Graph[pre_selection-1]);
		}
		
		
		OLED_ShowImage(x_pre,16+8*i,32,32,Menu_Graph[pre_selection]);
		OLED_ShowImage(x_pre+48,16+8*i,32,32,Menu_Graph[pre_selection+1]);
		
		OLED_Update();
	}
	
}



/*“菜单”界面UI函数
 *显示七个功能的UI
 *
 *
 */
uint8_t menu_flag=1;//光标的标志位
int Menu(void)
{
	move_flag=1;//开始移动的标志位，1表示开始移动，0表示停止移动
	uint8_t DirectFlag=2;//置1：移动到上一项；置2：移动到下一项
	while(1)
	{
		KeyNum=Key_GetNum();
		uint8_t menu_flag_temp=0;
		if(KeyNum==1)//上一项
		{
			DirectFlag=1;
			move_flag=1;
			menu_flag--;
			if(menu_flag<=0)menu_flag=7;
		}
		else if(KeyNum==2)//下一项
		{
			DirectFlag=2;
			move_flag=1;
			menu_flag++;
			if(menu_flag>=8)menu_flag=1;
		}
		else if(KeyNum==3)//确认
		{
			OLED_Clear();
			OLED_Update();
			menu_flag_temp=menu_flag;
		}
		
		if(menu_flag_temp==1){return 0;}
		else if(menu_flag_temp==2){MenuToFunction();StopWatch();}
		else if(menu_flag_temp==3){MenuToFunction();LED();}
	  else if(menu_flag_temp==4){MenuToFunction();MPU6050();}
		else if(menu_flag_temp==5){MenuToFunction();Game();}
		else if(menu_flag_temp==6){MenuToFunction();Emoji();}
		else if(menu_flag_temp==7){MenuToFunction();Gradienter();}
			

			if(menu_flag==1)//此时光标要在退回键
			{
				if(DirectFlag==1)Set_Selection(move_flag,1,0);//1表示Menu_Graph中的秒表，0表示退回键
				else if(DirectFlag==2)Set_Selection(move_flag,0,0);
			}
			
			else
			{
				if(DirectFlag==1)Set_Selection(move_flag,menu_flag,menu_flag-1);
				else if(DirectFlag==2)Set_Selection(move_flag,menu_flag-2,menu_flag-1);
			}
	}
}

/*----------------------------------秒表-------------------------------------*/

/*秒表功能界面UI函数
 *hour,min,sec分别为秒表功能中时分秒的变量
 *
 *
 */
uint8_t hour,min,sec;
void Show_StopWatch_UI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
	OLED_Printf(32,20,OLED_8X16,"%02d:%02d:%02d",hour,min,sec);
	OLED_ShowString(8,44,"开始",OLED_8X16);
	OLED_ShowString(48,44,"停止",OLED_8X16);
	OLED_ShowString(88,44,"清除",OLED_8X16);
}

/*秒表计时函数
 *该函数在中断函数中调用，通过中断获取数据
 *hour,min,sec分别为秒表功能中时分秒的变量
 *start_timing_flag为开始计时标志位：1：开始，0：停止
 */
uint8_t start_timing_flag;
void StopWatch_Tick(void)
{
	static uint16_t Count;
	Count++;
	if(Count>=1000)//定时器设置为1ms进行一次，1000为一秒
	{
		Count=0;
			if(start_timing_flag==1)
		{
			sec++;
			if(sec>=60)
			{
				sec=0;
				min++;
				if(min>=60)
				{
					min=0;
					hour++;
					if(hour>99)hour=0;
				}
			}
		}
	}
	
}

/*功能函数：秒表的工作函数
 *通过三个按键进行选择、控制秒表的工作
 *注：与前面的不同，在光标到达“开始”时，秒表直接开始工作，不需要按key3进行确认
 *“停止”和“清零”也不需要key3进行确认
 *
 *
 */
uint8_t stopwatch_flag=1;
int StopWatch(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		uint8_t stopwatch_flag_temp=0;
		if(KeyNum==1)//上一项
		{
			stopwatch_flag--;
			if(stopwatch_flag<=0)stopwatch_flag=4;
		}
		else if(KeyNum==2)//下一项
		{
			stopwatch_flag++;
			if(stopwatch_flag>=5)stopwatch_flag=1;
		}
		else if(KeyNum==3)//确认
		{
			OLED_Clear();
			OLED_Update();
			stopwatch_flag_temp=stopwatch_flag;
		}
		
		if(stopwatch_flag_temp==1){return 0;}
		
		
		switch(stopwatch_flag)
		{
			case 1:
				Show_StopWatch_UI();
				OLED_ReverseArea(0,0,16,16);//光标停在“返回”图标
				OLED_Update();
				break;
			
			case 2:
				Show_StopWatch_UI();
				start_timing_flag=1;
				OLED_ReverseArea(8,44,32,16);//光标停在“开始”
				OLED_Update();
				break;
			
			case 3:
				Show_StopWatch_UI();
				start_timing_flag=0;
				OLED_ReverseArea(48,44,32,16);//光标停在“停止”
				OLED_Update();
				break;
			
			case 4:
				Show_StopWatch_UI();
				start_timing_flag=0;
				hour=min=sec=0;
				OLED_ReverseArea(88,44,32,16);//光标停在“清除”
				OLED_Update();
				break;
		}
	}
}

/*----------------------------------手电筒-------------------------------------*/

/*手电筒UI函数
 *
 *
 */
void Show_LED_UI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
	OLED_ShowString(20,20,"OFF",OLED_12X24);
	OLED_ShowString(72,20,"ON",OLED_12X24);
}

/*LED功能内通过按键对LED亮灭的控制与光标的移动显示函数
 *
 *
 *
 */
uint8_t led_flag=1;
int LED(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		uint8_t led_flag_temp=0;
		if(KeyNum==1)//上一项
		{
			led_flag--;
			if(led_flag<=0)led_flag=3;
		}
		else if(KeyNum==2)//下一项
		{
			led_flag++;
			if(led_flag>=4)led_flag=1;
		}
		else if(KeyNum==3)//确认
		{
			OLED_Clear();
			OLED_Update();
			led_flag_temp=led_flag;
		}
		
		if(led_flag_temp==1){return 0;}
		
		
		switch(led_flag)
		{
			case 1:
				Show_LED_UI();
				OLED_ReverseArea(0,0,16,16);
				OLED_Update();
				break;
			
			case 2:
				Show_LED_UI();
				LED_OFF();
				OLED_ReverseArea(20,20,36,24);
				OLED_Update();
				break;
			
			case 3:
				Show_LED_UI();
				LED_ON();
				OLED_ReverseArea(72,20,24,24);
				OLED_Update();
				break;
			
		
		}
	}
}
/*----------------------------------MPU6050-------------------------------------*/

/*MPU6050姿态计算函数
 *通过陀螺仪和加速度计进行欧拉角计算
 *并通过互补滤波进行数据校正
 *
 *
 *
 */
int16_t ax,ay,az,gx,gy,gz;//MPU6050测得的三轴加速度和角速度
float roll_g,pitch_g,yaw_g;//陀螺仪解算的欧拉角
float roll_a,pitch_a;//加速度计解算的欧拉角
float Roll,Pitch,Yaw;//互补滤波后的欧拉角
float a=0.7;//互补滤波器系数
float Delta_t=0.005;//采样周期
double pi=3.1415927;//圆周率

void MPU6050_Calculation(void)
{
	Delay_ms(5);
	MPU6050_GetData(&ax,&ay,&az,&gx,&gy,&gz);
	
	//陀螺仪算欧拉角
	roll_g=Roll+(float)gx*Delta_t;//(float)强制转换为浮点数
	pitch_g=Pitch+(float)gy*Delta_t;
	yaw_g=Yaw+(float)gz*Delta_t;
	
	//加速度计算欧拉角
	pitch_a=atan2((-1)*ax,az)*180/pi;
	roll_a=atan2(ay,az)*180/pi;
	
	//互补滤波
	Roll=a*roll_g+(1-a)*roll_a;
	Pitch=a*pitch_g+(1-a)*pitch_a;
	Yaw=a*yaw_g;
	
}

/*MPU6050UI界面函数
 *
 *
 */
void Show_MPU6050_UI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
	OLED_Printf(0,16,OLED_8X16,"Roll: %.2f",Roll);//2f指取小数点后两位
	OLED_Printf(0,32,OLED_8X16,"Pitch:%.2f",Pitch);
	OLED_Printf(0,48,OLED_8X16,"Yaw:  %.2f",Yaw);
}

//MPU6050主函数
//若按下key3，退出MPU6050(void)函数，返回上一级菜单
//否则，进行MPU6050计算和数据显示
int MPU6050(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		if(KeyNum==3)
		{
			OLED_Clear();
			OLED_Update();
			return 0;
		}
		//
		OLED_Clear();
		MPU6050_Calculation();
		Show_MPU6050_UI();
		OLED_ReverseArea(0,0,16,16);
		OLED_Update();
	}
}

/*----------------------------------游戏-------------------------------------*/

//显示游戏界面的UI
void Show_Game_UI(void)
{
	OLED_ShowImage(0,0,16,16,Return);
	OLED_ShowString(0,16,"谷歌小恐龙",OLED_8X16);
}

//游戏界面内光标移动显示和按键控制的判断函数
uint8_t game_flag=1;
int Game(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		uint8_t game_flag_temp=0;
		if(KeyNum==1)//上一项
		{
			game_flag--;
			if(game_flag<=0)game_flag=2;
		}
		else if(KeyNum==2)//下一项
		{
			game_flag++;
			if(game_flag>=3)game_flag=1;
		}
		else if(KeyNum==3)//确认
		{
			OLED_Clear();
			OLED_Update();
			game_flag_temp=game_flag;
		}
		
		if(game_flag_temp==1){return 0;}
		else if(game_flag_temp==2){DinoGame_Pos_Init();DinoGame_Animation();}
		
		switch(game_flag)
		{
			case 1:
				Show_Game_UI();
				OLED_ReverseArea(0,0,16,16);
				OLED_Update();
				break;
			
			case 2:
				Show_Game_UI();
				LED_OFF();
				OLED_ReverseArea(0,16,80,16);
				OLED_Update();
				break;
			
			
		
		}
	}
}

/*----------------------------------动态表情包-------------------------------------*/

//动态表情包的设计函数
void Show_Emoji_UI(void)
{
	//闭眼和眉毛向下移动
	for(uint8_t i=0;i<3;i++)
	{
		OLED_Clear();
		OLED_ShowImage(30,10+i,16,16,Eyebrow[0]);//左眉毛
		OLED_ShowImage(82,10+i,16,16,Eyebrow[1]);//右眉毛
		OLED_DrawEllipse(40,32,6,6-i,1);//左眼
		OLED_DrawEllipse(88,32,6,6-i,1);//右眼
		OLED_ShowImage(54,40,20,20,Mouth);
		OLED_Update();
		Delay_ms(100);
	}
	
	//睁眼和眉毛向上移动
	for(uint8_t i=0;i<3;i++)
	{
		OLED_Clear();
		OLED_ShowImage(30,12-i,16,16,Eyebrow[0]);//左眉毛
		OLED_ShowImage(82,12-i,16,16,Eyebrow[1]);//右眉毛
		OLED_DrawEllipse(40,32,6,4+i,1);//左眼
		OLED_DrawEllipse(88,32,6,4+i,1);//右眼
		OLED_ShowImage(54,40,20,20,Mouth);
		OLED_Update();
		Delay_ms(100);
	}
	
	Delay_ms(500);
	
}

//动态表情包通过三个按键控制的控制函数
int Emoji(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		if(KeyNum==3)
		{
			OLED_Clear();
			OLED_Update();
			return 0;
		}
		
		Show_Emoji_UI();
		
	}
}
/*----------------------------------水平仪-------------------------------------*/

void Show_Gradienter_UI(void)
{
	MPU6050_Calculation();
	OLED_DrawCircle(64,32,30,0);//画圆
	OLED_DrawCircle(64-Roll,32+Pitch,4,1);//
}

int Gradienter(void)
{
	while(1)
	{
		KeyNum=Key_GetNum();
		if(KeyNum==3)
		{
			OLED_Clear();
			OLED_Update();
			return 0;
		}
		OLED_Clear();
		Show_Gradienter_UI();
		OLED_Update();
	}
}



