/****************************************************/
/*	プロジェクト名:	PG4			*/
/*	ファイル名：	main.c				*/
/*	内容：			メインプログラム*/
/*	日付：			2020/01/30 	*/
/*	コンパイラ:		NC30WA (Ver.5.10 entry)	*/
/****************************************************/

/* インクルードファイル */
#include	"sfr62p.h"			/* OAKS16用定義ファイル	*/
#include	"register.h"			/* 制御レジスタ設定ヘッダファイル*/
#include	"encoder.h"			/* エンコーダドライバヘッダファイル*/
#include	"ad.h"				/* A/Dドライバヘッダファイル*/
#include	"dynamic_disp.h"		/* 7セグメントLED制御ヘッダファイル*/


/* プロ		トタイプ宣言 */
void main(void);				/* メイン関数*/
void Variable_init(void);			/* 変数初期化処理*/
void Interrupt_5ms(void);			/* 割込み関数*/
#pragma	INTERRUPT	Interrupt_5ms


void Vehicle_speed_setting(void);		/* 設定車速処理*/
void Calc_speedmeter_target_angle(void);	/* 到達目標角度算出処理*/
void Calc_speedmeter_indicate_agnle(void);	/* 指示角度算出処理*/
void Motor_contorl(void);			/* モータ制御処理*/
void LED_contorol(void);			/* LED制御処理*/
void Display_control(void);			/* 表示制御処理*/
void System_counter_contorol(void);		/* システムカウンタ制御処理*/


/* マクロ定義 */
#define	SYSTEM_COUNTER_MAX		0xFFFFFFFFuL		/* システムカウンタ(1s単位) 最大値*/
#define	SYSTEM_COUNTER_SUB_TM		200			/* システムカウンタ(1s未満) 5ms×200=1s*/

#define	LED2_BLINK_TIMER			(200 - 1)	/* LED2 点滅周期 400×5ms = 2000ms*/
#define	LED2_BLINK_ON_TM			100		/* LED2 点灯判定時間	*/


/* 変数の宣言 */

unsigned long  System_counter;					/* システムカウンタ(1s単位)*/
unsigned char  System_counter_sub;				/* システムカウンタ(1s未満)*/
unsigned short Led2_blink_timer;				/* LED2 点滅周期のカウンタ*/

#define LIMIT_TIMER 5
signed short speed;//設定車速
signed int target_angle;//到達目標角度
signed int indicate_angle;//指示角度
signed int limit_timer;//25ms毎に指示角度算出処理を行うため
signed int step;//ステップ数	



int a, b, c, d;
int value;//ディスプレイ表示値

/*----------------------------------------------------------*/
/* 処理     :メイン処理					*/
/* 機能		:各種初期設定					*/
/*           5ms割込処理の起動					*/
/*----------------------------------------------------------*/
void main(void)
{
	Port_init();					/* ポート初期化*/
	Ad_init();					/* AD入力初期化*/
	Encoder_init();					/* エンコーダ初期化*/
	Dynamic_7seg_disp_init();			/* ダイナミック点灯初期化*/

	Variable_init();				/* 変数初期化*/

	Set_TimerA0();					/* タイマA0設定*/

	_asm("\tFSET	I");				/* 割り込み許可*/

	while (1) {
		;					/* メインループ (処理なし)*/
	}
}


/*----------------------------------------------------------*/
/* 処理     :変数初期化処理					*/
/* 機能		:プログラム実行前に変数を初期化する	 */
/*----------------------------------------------------------*/
void Variable_init(void)
{
	System_counter = 0;				/* システムカウンタ (1s単位) 初期化*/
	System_counter_sub = 0;				/* システムカウンタ (1s未満) 初期化*/

	Led2_blink_timer = LED2_BLINK_TIMER;		/* LED2 点滅周期のカウンタ初期化*/

	a = CHAR_CODE_BLANK;
	b = 0;
	c = 0;
	d = 0;

	value = 0;
}


/*----------------------------------------------------------*/
/* 処理     :5ms割込処理					*/
/* 機能		:5ms毎に処理を行う				*/
/*----------------------------------------------------------*/
void Interrupt_5ms(void)
{
	System_counter_contorol();			/*       システムカウンタ制御処理*/

	Encoder_main();					/* [Lib] エンコーダドライバ処理*/
	Ad_main();					/* [Lib] A/D変換ドライバ処理*/

	Vehicle_speed_setting();			/*       設定車速処理	*/
	Calc_speedmeter_target_angle();			/*       到達目標角度算出処理　*/
	Calc_speedmeter_indicate_agnle();		/*       指示角度算出処理　*/
	Motor_contorl();				/*       モータ制御処理*/
	LED_contorol();					/*       LED制御処理*/
	Display_control();				/*       表示制御処理	*/

	Dynamic_7seg_disp();				/* [Lib] 7セグメントLED制御処理	*/
}



/*----------------------------------------------------------*/
/* 処理     :設定車速処理				*/
/* 機能		:自動運転の設定車速の切り替えを行う	 */
/*----------------------------------------------------------*/

void Vehicle_speed_setting(void)
{	
	
	if (p1_5 == 0 && p1_6 == 1) {//SW1 ONかつSW1 OFFのとき表示値へ
		speed = value * 10;
	}
	else if ((p1_5 == 1 && p1_6 == 1) || p1_6 == 0 ) {
	//SW1 OFFかつSW2 OFFのときまたはSW2 ONのとき0へ
		speed = 0;
	}
}


/*----------------------------------------------------------*/
/* 処理     :到達目標角度算出処理				*/
/* 機能		:設定車速に対応したスピードメータの	*/
/*           到達目標角度を算出する			*/
/*----------------------------------------------------------*/
void Calc_speedmeter_target_angle(void)
{
	target_angle = speed / 5;
}


/*----------------------------------------------------------*/
/* 処理     :指示角度算出処理					*/
/* 機能		:到達目標角度に追従して指示角度を移動させる	*/
/*----------------------------------------------------------*/
void Calc_speedmeter_indicate_agnle(void)
{	
	
	if(p1_6 == 0){//SW2 ON
		if (limit_timer < LIMIT_TIMER) {//5*5msまでカウントする(デフォルトでは5ms毎に処理を行うため)
			limit_timer++;
		}
		else {//25msになったら以下を行う
			if (target_angle > indicate_angle) {
				indicate_angle += 1;
			}
			else if (target_angle < indicate_angle) {
				indicate_angle -= 1;
			}
			limit_timer = 0;
		}
	}else{//SW2 OFF
		if(limit_timer < LIMIT_TIMER){
			limit_timer++;
		}else{//25msになったら以下を行う
			if(target_angle > indicate_angle) {
				indicate_angle += 1;
			}
			else if (target_angle < indicate_angle) {
				indicate_angle -= 1;
			}
			limit_timer = 0;
		}
	}
}


/*----------------------------------------------------------*/
/* 処理     :モータ制御処理				*/
/* 機能		:指示角度に対応したステッピングモータ制御出を	*/
/*           行う						*/
/*----------------------------------------------------------*/
void Motor_contorl(void)
{
	step = indicate_angle % 4;

	if (step == 0) {
		p7_5 = 0;
		p7_7 = 0;
	}
	else if (step == 1) {
		p7_5 = 0;
		p7_7 = 1;
	}
	else if (step == 2) {
		p7_5 = 1;
		p7_7 = 1;
	}
	else if (step == 3) {
		p7_5 = 1;
		p7_7 = 0;
	}
}


/*----------------------------------------------------------*/
/* 処理     :LED制御処理					*/
/* 機能		:LEDの出力を行う				*/
/*----------------------------------------------------------*/
void LED_contorol(void)
{
	if (p1_5 == 1) {					/* SW1 OFFの場合*/
		Led2_blink_timer = LED2_BLINK_TIMER;		/* 点滅周期を周期最大値に設定する*/
								/* →SW1 ON時は点滅周期先頭から開始する*/
		p0_1 = 1;					/* LED2 消灯*/

	}
	else {							/* SW1 ONの場合	*/
		if (Led2_blink_timer < LED2_BLINK_TIMER) {	/* 400×5ms = 2000ms までカウントする	*/
			Led2_blink_timer++;
		}
		else {						/* 点滅周期に到達した場合	*/
			Led2_blink_timer = 0;			/* 点滅周期のカウントを最初から行う*/
		}

		if (Led2_blink_timer < LED2_BLINK_ON_TM) {	/* 点滅周期の前半*/
			p0_1 = 0;				/* LED2 を点灯する	*/
		}
		else {						/* 点滅周期の後半*/
			p0_1 = 1;				/* LED2 を消灯する	*/
		}
	}
}


/*----------------------------------------------------------*/
/* 処理     :表示制御処理					*/
/* 機能		:7セグメントLEDの表示内容を決定し、		*/
/*           7セグメントLED制御処理に表示内容を渡す		*/
/*----------------------------------------------------------*/
void Display_control(void)
{
	T_Dynamic_disp disp_buff[4];

	signed char  clk = 0;//クリック回数
	clk = Get_encoder_counter();

	if (a != CHAR_CODE_CTR_BAR) {
		value = 100 * b + 10 * c + d;
	}
	else {
		value = -(100 * b + 10 * c + d);
	}

	value += clk;

	if (value > 0 && value < 240) {

		a = CHAR_CODE_BLANK;
		b = value / 100;
		c = (value % 100) / 10;
		d = (value % 100) % 10;

	}
	else if (value >= 240) {

		a = CHAR_CODE_BLANK;
		b = 2;
		c = 4;
		d = 0;

	}
	
	/*------------------------------*/
	/*	表示内容を決定する	*/
	/*------------------------------*/
	disp_buff[0].figure = a;				/* 1000位桁 = 表示*/
	disp_buff[0].dp = 0;					/*   小数点 = 消灯*/

	disp_buff[1].figure = b;				/*  100位桁 = 消灯*/
	disp_buff[1].dp = 0;					/*   小数点 = 消灯*/

	disp_buff[2].figure = c;				/*   10位桁 = 消灯*/
	disp_buff[2].dp = 0;					/*   小数点 = 点灯*/

	disp_buff[3].figure = d;				/*    1位桁 = 消灯*/
	disp_buff[3].dp = 0;

	/*------------------------------*/
	/* 決定した表示内容を			*/
	/* 7セグメントLED制御に渡す		*/
	/*------------------------------*/
	Set_dynamic_7seg_disp_buff(&disp_buff[0]);			/* 7セグメント制御に表示値を設定*/
}


/*----------------------------------------------------------*/
/* 処理     :システムカウンタ制御処理				*/
/* 機能		:起動からの経過時間を測定する			*/
/*----------------------------------------------------------*/
void System_counter_contorol(void)
{
	if (System_counter_sub < SYSTEM_COUNTER_SUB_TM) {	/* システムカウンタ(1s未満)カウント	*/
		System_counter_sub++;
	}

	if (System_counter_sub >= SYSTEM_COUNTER_SUB_TM) {	/* システムカウンタ(1s未満) 1s測定		*/
		System_counter_sub = 0;

		if (System_counter < SYSTEM_COUNTER_MAX) {	/* システムカウンタ(1s単位) 測定		*/
			System_counter++;
		}
		else {						/* 最大値到達後は	*/
			System_counter = SYSTEM_COUNTER_MAX;	/* 最大値で固定する	*/
		}
	}
}

