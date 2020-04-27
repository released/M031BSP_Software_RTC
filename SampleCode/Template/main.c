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
#include "EEPROM_Emulate.h"
/*----------------------------------------------------*/

/*----------------------------------------------------*/
typedef enum{
	flag_1ms = 0 ,
	flag_5ms ,
	flag_10ms ,
	flag_50ms ,	
	flag_100ms ,
	
	flag_RTC ,
	flag_Record_Data ,
	
	flag_DEFAULT	
}Flag_Index;

uint8_t BitFlag = 0;
#define BitFlag_ON(flag)							(BitFlag|=flag)
#define BitFlag_OFF(flag)							(BitFlag&=~flag)
#define BitFlag_READ(flag)							((BitFlag&flag)?1:0)
#define ReadBit(bit)								(uint8_t)(1<<bit)

#define is_flag_set(idx)							(BitFlag_READ(ReadBit(idx)))
#define set_flag(idx,en)							( (en == 1) ? (BitFlag_ON(ReadBit(idx))) : (BitFlag_OFF(ReadBit(idx))))

#define HIBYTE(v1)              					((uint8_t)((v1)>>8))                      //v1 is UINT16
#define LOBYTE(v1)             	 				((uint8_t)((v1)&0xFF))

/*----------------------------------------------------*/
#define DATA_FLASH_OFFSET  						(0x3C00)
#define DATA_FLASH_AMOUNT						(16)
#define DATA_FLASH_PAGE  						(2)

#define DATA_FALSH_IDX_START					(1)
#define DATA_FALSH_IDX_RTC_YEAR					(DATA_FALSH_IDX_START)					// 2 BYTE
#define DATA_FALSH_IDX_RTC_MONTH				(DATA_FALSH_IDX_RTC_YEAR+2)
#define DATA_FALSH_IDX_RTC_DAY					(DATA_FALSH_IDX_RTC_MONTH+1)
#define DATA_FALSH_IDX_RTC_WEEKLY				(DATA_FALSH_IDX_RTC_DAY+1)

#define DATA_FALSH_IDX_RTC_HOUR				(DATA_FALSH_IDX_RTC_WEEKLY+1)
#define DATA_FALSH_IDX_RTC_MIN					(DATA_FALSH_IDX_RTC_HOUR+1)
#define DATA_FALSH_IDX_RTC_SEC					(DATA_FALSH_IDX_RTC_MIN+1)

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
#define	RTC_DEFAULT_YEAR		(2020)
#define	RTC_DEFAULT_MONTH		(4)
#define	RTC_DEFAULT_DAY		(27)
#define	RTC_DEFAULT_WEEKLY		(1)

#define	RTC_DEFAULT_HOUR		(13)
#define	RTC_DEFAULT_MIN		(59)
#define	RTC_DEFAULT_SEC			(30)

unsigned int g_year = RTC_DEFAULT_YEAR;
unsigned char g_month =RTC_DEFAULT_MONTH;
unsigned char g_day =RTC_DEFAULT_DAY;
unsigned char g_day_old;
unsigned char g_weekly = RTC_DEFAULT_WEEKLY;	//MONDAY : 1

unsigned char hour = RTC_DEFAULT_HOUR;
unsigned char min = RTC_DEFAULT_MIN;
unsigned char  sec = RTC_DEFAULT_SEC;

/*----------------------------------------------------*/
int IsDebugFifoEmpty(void);

void GetDateTimeFromFlash(void)
{
	uint8_t		rtc_temp = 0;
	uint8_t 	rtc_msb = 0;

	Read_Data(DATA_FALSH_IDX_RTC_YEAR, &rtc_temp);
	Read_Data(DATA_FALSH_IDX_RTC_YEAR+1, &rtc_msb);
	g_year = rtc_msb << 8 | rtc_temp;
	if ((rtc_msb == 0xFF) || (rtc_temp == 0xFF))
	{
		g_year = RTC_DEFAULT_YEAR;
	}

	Read_Data(DATA_FALSH_IDX_RTC_MONTH, &rtc_temp);
	g_month = rtc_temp;
	if (g_month == 0xFF)
	{
		g_month = RTC_DEFAULT_MONTH;
	}

	Read_Data(DATA_FALSH_IDX_RTC_DAY, &rtc_temp);
	g_day = rtc_temp;
	if (g_day == 0xFF)
	{
		g_day = RTC_DEFAULT_DAY;
	}	

	Read_Data(DATA_FALSH_IDX_RTC_WEEKLY, &rtc_temp);
	g_weekly = rtc_temp;
	if (g_weekly == 0xFF)
	{
		g_weekly = RTC_DEFAULT_WEEKLY;
	}


	Read_Data(DATA_FALSH_IDX_RTC_HOUR, &rtc_temp);
	hour = rtc_temp;
	if (hour == 0xFF)
	{
		hour = RTC_DEFAULT_HOUR;
	}
	
	Read_Data(DATA_FALSH_IDX_RTC_MIN, &rtc_temp);
	min = rtc_temp;
	if (min == 0xFF)
	{
		min = RTC_DEFAULT_MIN;
	}

	Read_Data(DATA_FALSH_IDX_RTC_SEC, &rtc_temp);
	sec = rtc_temp;	
	if (sec == 0xFF)
	{
		sec = RTC_DEFAULT_SEC;
	}
	
}

void SaveDateTimeToFlash(void)
{
	if (is_flag_set(flag_Record_Data))
	{
		set_flag(flag_Record_Data , DISABLE);

		Write_Data(DATA_FALSH_IDX_RTC_YEAR , LOBYTE(g_year));
		Write_Data(DATA_FALSH_IDX_RTC_YEAR+1 , HIBYTE(g_year));

		Write_Data(DATA_FALSH_IDX_RTC_MONTH , g_month);
		Write_Data(DATA_FALSH_IDX_RTC_DAY , g_day);
		Write_Data(DATA_FALSH_IDX_RTC_WEEKLY , g_weekly);
		
		Write_Data(DATA_FALSH_IDX_RTC_HOUR , hour);
		Write_Data(DATA_FALSH_IDX_RTC_MIN , min);
		Write_Data(DATA_FALSH_IDX_RTC_SEC , sec);	
		
		/* Disable FMC ISP function */
		FMC_Close();

		/* Lock protected registers */
		SYS_LockReg();

		printf(" \r\n%s ! \r\n\r\n" , __FUNCTION__);
	}
}

int set_data_flash_base(uint32_t u32DFBA)
{
    uint32_t   au32Config[2];
	
    /* Read User Configuration 0 & 1 */
    if (FMC_ReadConfig(au32Config, 2) < 0)
    {
        printf("\nRead User Config failed!\n");
        return -1;
    }

    /* Check if Data Flash is enabled (CONFIG0[0]) and is expected address (CONFIG1) */
    if ((!(au32Config[0] & 0x1)) && (au32Config[1] == u32DFBA))
        return 0;

    FMC_ENABLE_CFG_UPDATE();

    au32Config[0] &= ~0x1;         /* CONFIG0[0] = 0 (Enabled) / 1 (Disabled) */
    au32Config[1] = u32DFBA;

    /* Update User Configuration settings. */
    if (FMC_WriteConfig(au32Config, 2) < 0)
        return -1;

    FMC_ReadConfig(au32Config, 2);

    /* Check if Data Flash is enabled (CONFIG0[0]) and is expected address (CONFIG1) */
    if (((au32Config[0] & 0x01) == 1) || (au32Config[1] != u32DFBA))
    {
        printf("Error: Program Config Failed!\n");
        /* Disable FMC ISP function */
        FMC_Close();
        SYS_LockReg();
        return -1;
    }


    printf("\nSet Data Flash base as 0x%x.\n", u32DFBA);

    /* To check if all the debug messages are finished */
    while(!IsDebugFifoEmpty());

    /* Perform chip reset to make new User Config take effect */
    SYS->IPRST0 = SYS_IPRST0_CHIPRST_Msk;
    return 0;
}

void Emulate_EEPROM(void)
{
    SYS_UnlockReg();

    /* Enable FMC ISP function */
    FMC_Open();

    if (set_data_flash_base(DATA_FLASH_OFFSET) < 0)
    {
        printf("Failed to set Data Flash base address!\r\n");
    }

	/* Test Init_EEPROM() */
	Init_EEPROM(DATA_FLASH_AMOUNT, DATA_FLASH_PAGE);
	Search_Valid_Page();	
}

int isLeapYear(int year) 
{
	return (year%400==0) || ((year%4==0) && (year%100!=0));
}

void SoftwareRTC(void)
{
	if (!is_flag_set(flag_RTC))	//(flag_RTC==0)
		return;
	sec++;

	if ((sec%10) == 0 )	//record data per 10 sec	
	{
		set_flag(flag_Record_Data , ENABLE);	
	}
	
	if(sec==60)
	{
//		set_flag(flag_Record_Data , ENABLE);	//record data per 60 sec
	
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

	Emulate_EEPROM();
	GetDateTimeFromFlash();
	
    /* Got no where to go, just loop forever */
    while(1)
    {		
		RTC_Process();
		SaveDateTimeToFlash();
    }
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
