#define F_CPU 16000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void Uart_Init(void);
unsigned char Uart_Receive(void);
void Uart_Transmit(unsigned char);
void UART1_Transnum(int);

volatile int g_mod=0;
volatile int adc[8];
volatile int adc_max[8]={0,};
volatile int adc_min[8]={1024,1024,1024,1024,1024,1024,1024,1024};
volatile int i=0;
volatile double normalization[8];
volatile int value[8]={-8,-4,-2,-1,1,2,4,8};
volatile int sense_value=0;
volatile int rightocr=0, leftocr=0;
volatile int last_right=0, last_lest=0;
volatile int cnt1=0, cnt=0;
volatile int line_count=0;
volatile int line_dis=0;
volatile int mod=0;
volatile int stop=0;
ISR(INT0_vect)
{
	g_mod=0;
	 
}
ISR(INT1_vect)
{
	g_mod=1;
	
}

ISR(TIMER0_OVF_vect)
{
	TCNT0=100;
	for(i=0;i<8;i++)
	{
		ADMUX=(0x40)|i;
		ADCSRA |=(1<<ADSC);
		while(!(ADCSRA&(1<<ADIF)));
		adc[i]=ADC;
	}
	
	if(g_mod==0){
		for(i=0;i<8;i++)
		{
			if(adc[i]>=adc_max[i]) adc_max[i]=adc[i];
			else if(adc[i]<=adc_min[i]) adc_min[i]=adc[i];
			UART1_Transnum(adc[i]);
			Uart_Transmit(' ');
		}
		for(i=0;i<8;i++)
		{
			UART1_Transnum( adc_max[i]);
			Uart_Transmit(' ');
			UART1_Transnum( adc_min[i]);
			Uart_Transmit(' ');
		}
		Uart_Transmit(0x0d);
		for(i=0;i<8;i++)
		{
			if(adc[i]<800)  PORTA&=~(1<<i);
			else  PORTA|=(1<<i);
		}
	}
	else if(g_mod==1){
		PORTE=0x0A;
		sense_value=0;
		
		for(i=0;i<8;i++)
		{
			normalization[i]=(double)(adc[i]-adc_min[i])/(adc_max[i]-adc_min[i])*100;
			
			if(normalization[i]<55)  PORTA&=~(1<<i);
			else  PORTA|=(1<<i);
			
			UART1_Transnum((int)normalization[i]);
			Uart_Transmit(' ');
			
			sense_value+=normalization[i]*value[i];
		}
		UART1_Transnum(sense_value);
		Uart_Transmit(0x0d);
		
		if(line_count>=3)
		{
			OCR1A=0;
			OCR1B=0;
			stop=1;
		}
		else if(normalization[0]>55&&normalization[1]>55&&normalization[2]>55&&normalization[3]>55&&normalization[4]>55&&normalization[5]>55&&normalization[6]>55&&normalization[7]>55)
		{
			if(last_right>=last_lest)
			{
				OCR1A=last_right;
				OCR1B=0;
			}
			else
			{
				OCR1A=0;
				OCR1B=last_lest;
			}
		}
		else if((normalization[0]<55||normalization[1]<55)&&normalization[3]>55&&normalization[4]>55&&(normalization[6]<55||normalization[7]<55))
		{
			OCR1A=799;
			OCR1B=200;
		}
		else
		{
			rightocr=600+(sense_value/3), 
			leftocr=600-(sense_value/3);
			if(rightocr>=799) rightocr=799;
			else if(rightocr<=0) rightocr=0;
			if(leftocr>=799) leftocr=799;
			else if(leftocr<=0) leftocr=0;
			OCR1A=rightocr;
			OCR1B=leftocr;
			last_right=OCR1A, 
			last_lest=OCR1B;
		}
		if(normalization[0]<55&&normalization[1]<55&&normalization[2]<55&&normalization[3]<55&&normalization[4]<55&&normalization[5]<55&&normalization[6]<55&&normalization[7]<55)
		{
			line_dis=1;
			if(line_count==0)
			{
				cnt1=0;
			}
			
		}
		
		if(line_dis==1&&normalization[0]>55&&normalization[1]>55&&normalization[2]>55&&normalization[5]>55&&normalization[6]>55&&normalization[7]>55)
		{
			line_count++;
			line_dis=0;
		}
		
		if(cnt1==200)
		{
			line_count=0;
			cnt1=0;
		}
		cnt1++;
		
		
		
		if(line_count==1&&stop==1)
		{
			g_mod=2;
			cnt=0;
		}
		
	}
	else if(g_mod==2){
		line_count=0;
		PORTE=0x09;
		OCR1A=700;
		OCR1B=700;
		if(cnt==130) 
		{
			if(normalization[7]>55)
			{
				g_mod=1;
				stop=0;
			}
		}
		mod=1;
	}
	cnt++;
}



int main()
{
	DDRA=0xff;
	DDRF=0x00;
	DDRB = 0xff;
	DDRD = 0x00;
	DDRE=0x0f;
	
	PORTA=0xff;
	PORTE=0x0A;
	
	TCCR0=(0<<WGM01)|(0<<WGM00)|(0<<COM01)|(0<<COM00)|(1<<CS02)|(1<<CS01)|(1<<CS00);
	TIMSK=(1<<TOIE0);
	
	TCCR1A =(1<<COM1A1)|(0<<COM1A0)|(1<<COM1B1)|(0<<COM1B0)|(1<<WGM11);
	TCCR1B =(1<<WGM13)|(1<<WGM12)|(0<<CS02)|(0<<CS01)|(1<<CS00);
	EICRA = (1<<ISC01)|(1<<ISC11);
	EIMSK = (1<<INT1)|(1<<INT0);
	
	ADMUX=0x40;
	ADCSRA=0x87;
	ICR1=799;
	OCR1A=0;
	OCR1B=0;
	
	Uart_Init();
	TCNT0=100;
	sei();
	
	while(1){
		
		if(line_count>=3&&mod==1)
		{
			OCR1A=0;
			OCR1B=0;
			break;
		}
	}
}

void Uart_Init(void){
	DDRD = (1<<DDD3);
	UCSR1A = 0x00;
	UCSR1B = (1<<TXEN1) | (1<<RXCIE1);
	UCSR1C = (1<<UCSZ11) | (1<<UCSZ10);

	UBRR1H = 0;
	UBRR1L = 8;
}

void Uart_Transmit(unsigned char data){
	while(!(UCSR1A & (1<<UDRE1)));
	UDR1 = data;
}

void UART1_Transnum(int num){
	if(num <0)
	{
		Uart_Transmit('-');
		num = -num;
	}
	Uart_Transmit(((num%10000)/1000)+48);
	Uart_Transmit(((num%1000)/100)+48);
	Uart_Transmit(((num%100)/10)+48);
	Uart_Transmit(((num%10)/1)+48);
}