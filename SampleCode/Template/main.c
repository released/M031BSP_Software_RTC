/******************************************************************************
 * @file     main.c
 * @version  V1.00
 * @brief    A project template for M031 MCU.
 *
 * Copyright (C) 2017 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "NuMicro.h"

/*----------------------------------------------------*/

/*----------------------------------------------------*/
typedef enum{
	flag_1ms = 0 ,
	flag_5ms ,
	flag_10ms ,
	flag_50ms ,	
	flag_100ms ,
	
	flag_RTC ,
	
	flag_DEFAULT	
}Flag_Index;

uint8_t BitFlag = 0;
#define BitFlag_ON(flag)							(BitFlag|=flag)
#define BitFlag_OFF(flag)							(BitFlag&=~flag)
#define BitFlag_READ(flag)							((BitFlag&flag)?1:0)
#define ReadBit(bit)								(uint8_t)(1<<bit)

#define is_flag_set(idx)							(BitFlag_READ(ReadBit(idx)))
#define set_flag(idx,en)							( (en == 1) ? (BitFlag_ON(ReadBit(idx))) : (BitFlag_OFF(ReadBit(idx))))

/*----------------------------------------------------*/
typedef enum{
	TIMER_1MS = 1 ,
	TIMER_5MS = 5 ,
	TIMER_10MS = 10 ,	
	TIMER_50MS = 50 ,
	TIMER_100MS = 100 ,
	TIMER_1S = 1000 ,
	
	TIMER_DEFAULT	
}TIMER_Define;

uint16_t conter_1ms = 0;

/*----------------------------------------------------*/

unsigned int g_year=2015;
unsigned char g_month=2;
unsigned char g_day=28;
unsigned char g_day_old;
unsigned char g_weekly=2;

unsigned char hour=23;
unsigned char min=59;
unsigned char  sec=30;

/*----------------------------------------------------*/

int isLeapYear(int year) 
{
	return (year%400==0) || ((year%4==0) && (year%100!=0));
}

void SoftwareRTC(void)
{
	if (!is_flag_set(flag_RTC))	//(flag_RTC==0)
		return;
	sec++;
	if(sec==60)
	{
		sec=0;			
		min++;			
	}
	if(min==60)
	{
		min=0;
		hour++;			
	}
	if(hour==24)
	{
		hour=0;
		g_day++;
		g_weekly++;
	}
	printf("hr.:%d :min: %d: sec: %d\n\r",hour,min,sec);
	set_flag(flag_RTC,DISABLE);//flag_RTC=0;
}

void SoftwareYMD(void)
{	
	int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	days[1] += isLeapYear(g_year);
	if(g_day==g_day_old)
		return;
	g_day_old=g_day;
	if(g_day>days[g_month-1])
	{
		g_day=1;
		g_month++;

	}

	if(g_weekly>7)//7 is sum day
		g_weekly=1;

	if(g_month>12)
	{
		g_month=1;
		g_year++;
	}
	printf("Y:%d, M:%d, D:%d, Weekly:%d\n\r",g_year,g_month,g_day,g_weekly);
}

void RTC_Process(void)
{
	SoftwareRTC();
	SoftwareYMD();
}


void loop_1ms(void)
{
	static uint16_t CNT = 1;
//	static uint16_t LOG = 0;

	if (is_flag_set(flag_1ms))
	{		
		set_flag(flag_1ms,DISABLE);
		
		if (CNT++ == (TIMER_1S/TIMER_1MS))
		{
			CNT = 1;
//			printf("%s : %4d\r\n",__FUNCTION__,LOG++);

			PB14 ^= 1;
		}		
	}
}

void loop(void)
{
	loop_1ms();
}

void timer_counter(void)
{
	conter_1ms ++;
	
	set_flag(flag_1ms,ENABLE);
	
	if(!(conter_1ms %TIMER_5MS)){
		set_flag(flag_5ms,ENABLE);}
	
	if(!(conter_1ms %TIMER_10MS)){
		set_flag(flag_10ms,ENABLE);}
	
	if(!(conter_1ms %TIMER_50MS)){
		set_flag(flag_50ms,ENABLE);}
	
	if(!(conter_1ms %TIMER_100MS)){
		set_flag(flag_100ms,ENABLE);}

	if(conter_1ms >= 65500){
		conter_1ms = 0;}
}

void GPIO_Init (void)
{
    GPIO_SetMode(PB, BIT14, GPIO_MODE_OUTPUT);
}

void TMR3_IRQHandler(void)
{
//	static uint32_t LOG = 0;
//	static uint16_t CNT = 0;
	
    if(TIMER_GetIntFlag(TIMER3) == 1)
    {
        TIMER_ClearIntFlag(TIMER3);
		
		timer_counter();		
		loop();
    }
}

void TIMER3_Init(void)
{
    TIMER_Open(TIMER3, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER3);
    NVIC_EnableIRQ(TMR3_IRQn);	
    TIMER_Start(TIMER3);
}

void TMR2_IRQHandler(void)
{
    if(TIMER_GetIntFlag(TIMER2) == 1)
    {
        TIMER_ClearIntFlag(TIMER2);
		set_flag(flag_RTC,ENABLE);		
    }
}

void TIMER2_Init(void)
{
    TIMER_Open(TIMER2, TIMER_PERIODIC_MODE, 1);
    TIMER_EnableInt(TIMER2);
    NVIC_EnableIRQ(TMR2_IRQn);	
    TIMER_Start(TIMER2);
}


void UART02_IRQHandler(void)
{

	if ((UART_GET_INT_FLAG(UART0,UART_INTSTS_RDAINT_Msk)))
	{
        /* UART receive data available flag */
 
	}
    else if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RXTOINT_Msk)) 
    {
 
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);

	/* Set UART receive time-out */
	UART_SetTimeoutCnt(UART0, 20);

	/* Set UART FIFO RX interrupt trigger level to 4-bytes*/
    UART0->FIFO = ((UART0->FIFO & (~UART_FIFO_RFITL_Msk)) | UART_FIFO_RFITL_4BYTES);

	/* Enable UART Interrupt - */
	UART_ENABLE_INT(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);
	
	NVIC_EnableIRQ(UART02_IRQn);	

	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());


	UART_WAIT_TX_EMPTY(UART0);
}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Enable HIRC clock (Internal RC 48MHz) */
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
//    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
	
    /* Wait for HIRC clock ready */
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);
//    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);
	
    /* Select HCLK clock source as HIRC and HCLK source divider as 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLKSEL_HIRC, CLK_CLKDIV0_HCLK(1));

    /* Enable UART0 clock */
    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_PCLK0, CLK_CLKDIV0_UART0(1));

    CLK_EnableModuleClock(TMR2_MODULE);
    CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL1_TMR2SEL_PCLK1, 0);
	
    CLK_EnableModuleClock(TMR3_MODULE);
    CLK_SetModuleClock(TMR3_MODULE, CLK_CLKSEL1_TMR3SEL_PCLK1, 0);


    /* Update System Core Clock */
    SystemCoreClockUpdate();

	/*----------------------------------------------------*/
    /* Set PB multi-function pins for UART0 RXD=PB.12 and TXD=PB.13 */
    SYS->GPB_MFPH = (SYS->GPB_MFPH & ~(SYS_GPB_MFPH_PB12MFP_Msk | SYS_GPB_MFPH_PB13MFP_Msk))    |       \
                    (SYS_GPB_MFPH_PB12MFP_UART0_RXD | SYS_GPB_MFPH_PB13MFP_UART0_TXD);


    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M031 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

    UART0_Init();

	GPIO_Init();

	TIMER2_Init();

	TIMER3_Init();
	
    /* Got no where to go, just loop forever */
    while(1)
    {

		
		RTC_Process();
		
    }
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
