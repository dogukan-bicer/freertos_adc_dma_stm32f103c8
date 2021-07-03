#include "stm32f10x.h"
#include "stm32f10x_rcc.h"


#include "FreeRTOS/include/FreeRTOS.h"
#include "FreeRTOS/include/task.h"
uint32_t analogveri = 0;

void DMA_Sifirla(void);
void dma_ayarlari(void);
void timer_ayarlari(void);
void sistem_ayar(void);
void vTaskAnalogOkuma(void *p);
void vTaskButonKontrol(void *p);
void vTaskYesilLed(void *p);

int main(void)
{
	sistem_ayar();
	xTaskCreate(vTaskAnalogOkuma, (const char*) "Analog oku", 128, NULL, 8, NULL);
	xTaskCreate(vTaskButonKontrol, (const char*) "on off led", 128, NULL, 7, NULL);
	xTaskCreate(vTaskYesilLed, (const char*) "yesil led blink", 128, NULL, 6, NULL);
  vTaskStartScheduler();
	return 0;
}

void DMA_Sifirla(void)
{
	DMA1_Channel5->CCR=0;
	DMA1_Channel5->CNDTR=0;
	DMA1_Channel5->CPAR=0;
	DMA1_Channel5->CMAR=0;
	DMA1->IFCR=0;
	DMA1->ISR=0;
}

void dma_ayarlari(void) {

// çevre birim adresi
	DMA1_Channel5->CPAR =0x40012C00+0x4C; //0x40012C00 : Timer 1'in adresi 0x4c timer 1 DMAR'register'inin offset'i 
	DMA1_Channel5->CMAR =(u32)&analogveri;//veriyi timer1 e gönder
	DMA1_Channel5->CCR |= (1<<4); //veri transfer yönü  :  1 : bellekten  oku
	DMA1_Channel5->CCR |= (3<<12); // KANAL öncelik seiyesi  : 11 : çok yüksek
	DMA1_Channel5->CCR |= (1<<11); // bellek kelime boyutu   : 10 : 32 bit
	DMA1_Channel5->CCR |= (1<<9);  // çevre kelime boyutu    : 10 : 32 bit
	DMA1_Channel5->CCR |= (1<<5);  // circular mod           :  1 : aktif
	DMA1_Channel5->CCR |= (1<<4);			//Transfer hata kesmesi aktif.
  DMA1_Channel5->CNDTR = 1;// kaç adet veri transfer edilecek?
	TIM1->DCR  |= (1<<8) | 15; //timerdaki veri
	TIM1->DIER |= (1<<8); // UPDATE TETIKLEME
  TIM1->EGR  |= 6 ;//DMA tetikleme
  DMA1_Channel5->CCR |= 1;//DMA aktif
  TIM1->CR1 |= 1;	//timer aktif
}



void sistem_ayar()
{
	RCC->APB2ENR |= (1<<4); //// PORT A AKTIF
 	RCC->APB2ENR |= (1<<2); //// PORT C AKTIF
	GPIOA->CRL &= 0xFFFF00FF; /// A3 ve A2 pini resetlendi
	GPIOA->CRL |= 0x00008000; /// A3 giris push pull	A2 analog giris
	GPIOC->CRH &= 0xFF0FFFFF; /// C12 pini resetlendi
	GPIOC->CRH |= 0x00300000; /// C12 pini ÇIKIS	
	GPIOA->CRH &= 0xFFF0FF0F; /// A9 ve A12 pini resetlendi
	GPIOA->CRH |= 0x00030030; /// A9 ve A12 pini CIKIS
	timer_ayarlari();
	 GPIOA->CRH &=~(16<<8);
	 GPIOA->CRH |=(11<<8);//a10 pini analog giris 0b1011
	
	RCC->APB2ENR |= 0x201;
	ADC1->CR2 = 0;    //adc kapali
	ADC1->SQR3 = 2;   //adc pa2 den okuma yapicak
	ADC1->CR2 |= 1;  //sürekli mod
	ADC1->CR2 |= 2; //adc kalibrasyon
	DMA_Sifirla(); //dma registerlarini sifirla
  dma_ayarlari();
	
GPIOA->ODR |= 0x1000;
GPIOA->ODR |= 0x0200;
GPIOA->ODR |= 0x2000;
}

void timer_ayarlari(void){
	
	RCC->APB2ENR |= (1<<11); //// Enabling PORT A 
	 RCC->APB2ENR |= (1<<11); //timer 1 clock aktif
	 RCC->AHBENR |=1; // DMA 1 aktif hale getirildi (enable)
	 TIM1->CCMR2 |=(6<<4); //kanal 3 pwm mode 1 icin ayarlandi
	 TIM1->ARR =3600; //maksimum duty cycle degeri
	 TIM1->PSC =0;
   TIM1->CCER |=(1<<8); //KANAL 3 IÇIN cikis aktif hale getirildi
	 TIM1->BDTR |=(1<<15); //ana giris aktif I

}

void vTaskAnalogOkuma(void *p)
{
	for (;;)
	{
		ADC1->SQR1 &= ~(0xf << 20);//adc kanal dizisi uzunlugu
    ADC1->SQR1 |= (0x1 << 20);			
    ADC1->SMPR2 &= ~(0x7);
    ADC1->SMPR2 |= (0x1);	//7.5 döngü
    //12mhz: 7.5+12.5 =20 hz ->3.3us(mikro Saniye) 
    ADC1->CR2 &= ~(1 << 11); // right alignment		
    ADC1->CR2 |= 2|1; //adc açik ve surekli
    ADC1->CR2 |= (1<<2);// kalibrasyon	
    while(ADC1->CR2 & (1<<2));
    ADC1->CR2 |=1; //adc açik	
	  GPIOA->CRH &=~(1<<6);//PA10 çikis
    GPIOA->CRH |=(1<<7)|(3<<4);
    analogveri = (ADC1->DR/1.13);		
		vTaskDelay(10/portTICK_RATE_MS);
	}
}

void vTaskButonKontrol(void *p)
{
	for (;;)
	{
		if(GPIOA->IDR & (1<<3)) //a3
		{ 
			GPIOA->ODR &= ~0x1000;	
		}
		else{
		  GPIOA->ODR |= 0x1000;
		}
		vTaskDelay(10/portTICK_RATE_MS);
	}
}

void vTaskYesilLed(void *p)
{
	for (;;)
	{
		GPIOC->ODR ^= 0x2000;
		vTaskDelay(300/portTICK_RATE_MS);
	}
}
