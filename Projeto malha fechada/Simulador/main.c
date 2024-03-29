#include "main.h"

//pinos LCD
#ifndef lcd_enable
   #define lcd_enable   pin_e1   // pino enable do LCD
   #define lcd_rs       pin_e2   // pino rs do LCD
   //#define lcd_rw     pin_e2   // pino rw do LCD
   #define lcd_d4       pin_d4   // pino de dados d4 do LCD
   #define lcd_d5       pin_d5   // pino de dados d5 do LCD
   #define lcd_d6       pin_d6   // pino de dados d6 do LCD
   #define lcd_d7       pin_d7   // pino de dados d7 do LCD
#endif

#include "mod_lcd.c"

//Ganhos
#define kp 0.5 //Ganho proporcional
#define ki 0.5 //Ganho integral
#define kd 1 //Ganho derivativo

//Fun��es
void disturbio(); //Disturbio (Cooler)
void temperatura(); //Temperatura do sensor (LM35)
void transReferencia(); //Referencia do potenciometro
void controle(); //Calculo

float erro; //erro
float somaErro = 0; //Soma os erros
float tempDigital; //Grarda a temperatura real
float ultimaTemp = 0; //Ultima medida de temperatura
float referencia; //Pega a entrada/referencia do potenciometro 1 (Tranformado)
float saida; //Resultado do calculo PID

float pwm;
int16 pot1, pot2; //Potenciometro 1 (Referencia) e 2 (disturbio) (Porta A0 e A1)
int16 tempAnalog; //Temperatura do conversor analogico

void main()
{
   setup_adc_ports(ALL_ANALOG);
   setup_adc(ADC_CLOCK_DIV_32);
   
   setup_psp(PSP_DISABLED);
   setup_spi(SPI_SS_DISABLED);
   
   setup_timer_0(RTCC_INTERNAL|RTCC_DIV_1);
   setup_timer_1(T1_DISABLED);
   setup_timer_2(T2_DIV_BY_16,155,1);
   
   setup_ccp1(CCP_PWM);
   setup_ccp2(CCP_PWM);
   set_pwm1_duty(0);
   set_pwm2_duty(0);
   
   setup_comparator(NC_NC_NC_NC);
   setup_vref(FALSE);
   
   lcd_ini();//Inicia o display
   delay_ms(50);
   
   while(true){
      transReferencia();
      disturbio();
      temperatura();
      controle();
  
      fprintf(Serial, "%f %f %f %f %f\n",referencia, tempDigital, erro, saida, pwm);
      printf(lcd_escreve,"\fT:%f R:%f\nSaida=%ld   ",tempDigital,referencia,(int16)saida); //Limpa o display e imprime
      delay_ms(1000);
   }
}

//Pega a entrada/Referencia, a temperatura desejada
void transReferencia(){
   set_adc_channel(0);//AN0
   delay_us(10);
   pot1 = read_adc();
   
   //    ADC  = temp
   //    1023 = 80
   //    pot1 = y
   //
   //    y = pot1*0.078201368;
   
   referencia = 0.078201368*pot1;
}

//Pega o disturbio no Potenciometro 2
void disturbio(){
   //Protenciometro 2
   set_adc_channel(1);//AN1
   delay_us(10);
   pot2 = read_adc();
   
   //    ADC  = PWM
   //    1023 = 610
   //    pot2 =  y
   //
   //    y = pot2*0.596285435;
   
   pwm = pot2*0.596285435;
   
   set_pwm1_duty((int16)pwm);
}

//Pega a temperatura do LM35
void temperatura(){
   set_adc_channel(3);//AN3
   delay_us(10);
   tempAnalog = read_adc();
   
   // tempAnalog * 5
   // -------------- -> Converter esse valor para tens�o el�trica
   //       1023
   
   //Divide por 0.010 pq � 10 mV por grau celsius

   //  (tempAnalog * 5)
   // --------------------  =  tempAnalog *0.48887
   //    1023 * 0.010
   
   tempDigital = tempAnalog*0.4888758553;
}

//Controle

void controle(){
   //proporcional
   erro = referencia - tempDigital; //Erro
   somaErro += erro; //Soma o erro
   saida = (kp * erro) + (ki * somaErro) + (kd * (ultimaTemp - tempDigital)); //Calculo
   ultimaTemp = tempDigital; //Guarda a ultima temperatura medida
   
   if(saida<0){
      set_pwm2_duty(0);
   }else if(saida > 610){
      set_pwm2_duty(610);
   }else{
      set_pwm2_duty((int16)saida);
   }
}
