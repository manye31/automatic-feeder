/*
 * uc1.c
 *
 * Created: 7/22/2024 4:19:40 PM
 * Author : liamm
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//Definitions
#define FREQ_CLK 1000000
#define lcd_port PORTD // We are using port D for the LCD pins (0 = register select, 1 = enable, 4-7 = data)
#define lcd_EN 1       // enable pin on LCD
#define lcd_RS 0       // register select on LCD
#include "hx711.h"	   // Include folder for amplifier

	//Dummy Variables 
	char printraw[100],printweight[100]; // Dummy variables to print to LCD from weight sensor for accuracy
	char pot_reading = 0; // value read from analog sensor (0-255 since we'll only read the upper 8 bits of the 10-bit number)

	//Hx711 Setup 
	int8_t gain = HX711_GAINCHANNELA128;
	int32_t offset = 8331400;//8433560;
	double calibrationweight = 318;
	int32_t calib_offset = 8331650;//8440750;
	double scale = 10;//23; //(calib_offset-offset)/calibrationweight;
	
	//Actual Variables
	double amount_of_food;
	int hour,min_ten,min_one,hour_int,merid,food_time,hour_food_time,merid_food_time,min_ten_food_time,min_one_food_time;
	int food_flag=0,k;
	
	


//Subroutines to include
void wait(volatile unsigned int);
void LCD_write(unsigned char cmd);
void LCD_init(void);
void LCD_command(char);
void LCD_data(char);
void LCD_print(char *str);
void LCD_gotoxy(unsigned char x, unsigned char y);
void testing_scale(void);
double question_1(void);
int send_amount_of_food(double);
double food_options(void);
void print_food(double);
int clock_set_mer(void);
int clock_set_min_one(void);
int clock_set_min_ten(void);
int clock_set_hour(void);
void print_hour(int);
void print_min_ten(int);
void print_min_one(int);
void print_mer(int);
int int_of_feed(void);
void print_hour_int(int);
void print_time(int,int,int,int);
void clear_LCD(void);
void print_food_time(int,int,int,int);
void uc2(void);
int switch_merid(int);

int main(void)
{
	//Initialize Ports
	DDRD = 0xF3;						//Set PD0, PD1, and PD4-7 to output for the LCD
	DDRC=  0b00010001;					//Set PC0 as potentiometer output
										//Set PC4 as output to stop feeding
	DDRB=  0b00111000;					//Set PB3-5 as outputs of how much food we are filling the bowls with
	ADMUX= 0b00100101;					//Connected to ADC5 so we are use the MUX Bits,then we use internal V_ref,use left justified location
	PRR=   0b00000000;					//Set power reduction bit to power on the ADC
	ADCSRA=0b10000111;					//Enable ADC and set scaler to 128
	EICRA=0b00001110;					//Sets INT1 and INT0 on rising edge
	EIMSK=0b00000011;					//Enables INT0 and INT1

	PORTB= 0b00111000;
	PINC=  0b00000000;
	PORTD= 0b00000000;
	
	//Initialize functions
	int test=0;							//If testing the scale set this to 1, if not 0
	LCD_init();							//initialize the LCD
	sei();								//Set interrupt command 
	hx711_init(gain, scale, offset);	//Initialize Amplifier
	
	
	if(test==1){
		testing_scale();
				}
				
				
	//Question 1////////////////////////////////////////////////
			amount_of_food=question_1();
			PORTB=send_amount_of_food(amount_of_food);	

	//Question 2 ///////////////////////////
			LCD_gotoxy(1, 2);
			LCD_print("Set Time:            ");
			LCD_gotoxy(1, 1);
			LCD_print("00:00                     ");
			while ((PINC == (PINC | 0b00001000))){
			hour=clock_set_hour();
			}
			wait(500);
			while ((PINC == (PINC | 0b00001000))){
			min_ten=clock_set_min_ten();
			}
			wait(500);
			while ((PINC == (PINC | 0b00001000))){
			min_one=clock_set_min_one();
			}
			wait(500);
			while ((PINC == (PINC | 0b00001000))){
			merid=clock_set_mer();
			}
			
			
		//Question 3 ///////////////////////////
			clear_LCD();	
			LCD_gotoxy(1, 1);
			LCD_print("Interval of Feeding?           ");
			LCD_gotoxy(1, 2);
			while ((PINC == (PINC | 0b00001000))){       //Wait until select switch is pressed
			hour_int=int_of_feed();
			}
			
		//Clock Main loop
			clear_LCD();
			while(1){
		//Setting the food time
				if (food_flag==0){
					if (hour_int==6){
							hour_food_time=hour+hour_int;
							min_one_food_time=min_one;
							min_ten_food_time=min_ten;
							merid_food_time=merid;
						if (hour_food_time>11){
							if (hour_food_time>12){
							hour_food_time=hour_food_time-12;
							}
							merid=switch_merid(merid);
						}
					}//End of if of 6 hour interval timing
					if (hour_int==12){
						hour_food_time=hour;
						min_one_food_time=min_one;
						min_ten_food_time=min_ten;
						merid_food_time=switch_merid(merid);
					}//End of 12 hour interval timing 
					if (hour_int==24){
						hour_food_time=hour;
						min_one_food_time=min_one-1;
						min_ten_food_time=min_ten;
						merid_food_time=merid;
						if (min_one_food_time==-1){
							min_ten_food_time=min_ten-1;
							min_one_food_time=9;
							if (min_ten_food_time==-1){
							hour_food_time=hour_food_time-1;
							min_ten_food_time=5;
								if (hour_food_time==0){
									hour_food_time=12;
													}
								if (hour_food_time==11){
									merid_food_time=switch_merid(merid_food_time);
								}
													}
										}
					}//End of 24 hour interval timing
					food_flag=1;
					}//End of setting next food time
					
			
	//Wait 60 seconds and then changes the time
	k=0;
	clear_LCD();
	while(k<61){ 
				//Print both the time and food time
				print_time(hour,min_ten,min_one,merid);
				print_food_time(hour_food_time,min_ten_food_time,min_one_food_time,merid_food_time);
				//wait(50);  //this loop takes longer than a second, so it is hard coded the amount of time it takes
				k=k+1;
	}
	
	//Logic for how time changes when it reaches max of minute...
			min_one=min_one+1;
			if (min_one==10)
			{
				min_one=0;
				min_ten=min_ten+1;
					if (min_ten==6){
						min_ten=0;
						hour=hour+1;
							if (hour==13){
								hour=1;
								if (merid==0){
									merid=1;
										     }
								else{
									merid=0;
											}
						}//If hour ends	
						}//If minute tens end			 
						}//If minute ones end
								//Send signal to uC2 that it is food time, wait til it is done
									//Testing Parameters
									//hour_food_time=hour;
									//min_one_food_time=min_one;
									//min_ten_food_time=min_ten;
									//merid_food_time=merid;
									if((hour_food_time==hour) & (min_one_food_time==min_one) & (min_ten_food_time==min_ten) & (merid==merid_food_time)){
										uc2();
										food_flag=0;
									}
						}//Clock time loop end
									
					}//Main End




void LCD_init(void) { // initialize the LCD
	LCD_command(0x02); // set the LCD to 4-bit control mode
	wait(1);
	LCD_command(0x28); // set the LCD to 2 lines, 5X7 dots, 4-bit mode
	wait(1);
	LCD_command(0x01); // clear LCD
	wait(1);
	LCD_command(0x0E); // turn LCD cursor ON
	wait(1);
	LCD_command(0x80); //  set cursor at first line, first position (positions are defined by lower 7 bits and are 1-40 for line 1 and 41-80 for line 2, though
	// positions 17-40 and 57-80 are not visible on a 16-column display
	wait(1);
	return;
} // end LCD_init()
void LCD_gotoxy(unsigned char x, unsigned char y) {  // move the LCD cursor to a given position on the LCD screen
	unsigned char firstCharAdr[] = {0x80, 0xC0, 0x94, 0xD4};
	LCD_command(firstCharAdr[y - 1] + x - 1);
	wait(1);
} //end LCD_gotoxy
void LCD_command(char command_value) { // send a command to the LCD driver -- 1/2 nibble at a time
	lcd_port = command_value & 0xF0;          // Send upper nibble (mask lower nibble because PD4-PD7 pins are used for data)
	lcd_port &= ~(1 << lcd_RS); // lcd_RS = 0 for command
	lcd_port |= (1 << lcd_EN);  // toggle lcd_EN bit
	wait(1);
	lcd_port &= ~(1 << lcd_EN); // toggle bit back off
	wait(10);
	lcd_port = ((command_value << 4) & 0xF0); // Send lower nibble (shift 4-bits and mask for lower nibble)
	lcd_port &= ~(1 << lcd_RS); // lcd_RS = 0 for command
	lcd_port |= (1 << lcd_EN);  // toggle lcd_EN bit
	wait(1);
	lcd_port &= ~(1 << lcd_EN); // toggle bit back off
	wait(10);
} // end LCD_command()
void LCD_data(char data_value) { // send data to the LCD driver -- 1/2 nibble at a time
	lcd_port = data_value & 0xF0; // Send upper nibble (mask lower nibble because PD4-PD7 pins are used for data)
	lcd_port |= (1 << lcd_RS);  // lcd_RS = 1 for data
	lcd_port |= (1 << lcd_EN);  // toggle lcd_EN bit
	wait(1);
	lcd_port &= ~(1 << lcd_EN); // toggle bit back off
	wait(10);
	lcd_port = ((data_value << 4) & 0xF0); // Shift 4-bit and mask
	lcd_port |= (1 << lcd_RS);  // lcd_RS = 1 for data
	lcd_port |= (1 << lcd_EN);  // toggle lcd_EN bit
	wait(1);
	lcd_port &= ~(1 << lcd_EN); // toggle bit back off
	wait(10);
} // end LCD_data()
void LCD_print( char *str) { // send string information to the LCD driver to be printed (the argument stores the address of the string in pointer *str)
	int i = 0;
	while (str[i] != '\0') {     // loop until NULL character in the string
		LCD_data(str[i]); // sending data on LCD byte by byte
		i++;
	}
	return;
} // end LCD_print()
void wait(volatile unsigned int number_of_msec) {
	// This subroutine creates a delay equal to number_of_msec*T, where T is 1 msec
	// It changes depending on the frequency defined by FREQ_CLK
	char register_B_setting;
	char count_limit;
	
	// Some typical clock frequencies:
	switch(FREQ_CLK) {
		case 16000000:
		register_B_setting = 0b00000011; // this will start the timer in Normal mode with prescaler of 64 (CS02 = 0, CS01 = CS00 = 1).
		count_limit = 250; // For prescaler of 64, a count of 250 will require 1 msec
		break;
		case 8000000:
		register_B_setting =  0b00000011; // this will start the timer in Normal mode with prescaler of 64 (CS02 = 0, CS01 = CS00 = 1).
		count_limit = 125; // for prescaler of 64, a count of 125 will require 1 msec
		break;
		case 1000000:
		register_B_setting = 0b00000010; // this will start the timer in Normal mode with prescaler of 8 (CS02 = 0, CS01 = 1, CS00 = 0).
		count_limit = 125; // for prescaler of 8, a count of 125 will require 1 msec
		break;
	}
	
	while (number_of_msec > 0) {
		TCCR0A = 0x00; // clears WGM00 and WGM01 (bits 0 and 1) to ensure Timer/Counter is in normal mode.
		TCNT0 = 0;  // preload value for testing on count = 250
		TCCR0B =  register_B_setting;  // Start TIMER0 with the settings defined above
		while (TCNT0 < count_limit); // exits when count = the required limit for a 1 msec delay
		TCCR0B = 0x00; // Stop TIMER0
		number_of_msec--;
	}
} // end wait()
void testing_scale(void){
	while(1){

		double weight_i = hx711_getweight();		//Get initial weight
		//weight_i=-weight_i;
		//double density=134;							//g/cups
		//double weight_2;
		//weight_2=amount_of_food*density;			//Get weight of amount food
		//int i=1;
		while(1){
			double weight = hx711_getweight();	//read weight
			int32_t read = hx711_read();
			ltoa(read, printraw, 10);
			//weight=-weight;
			weight=weight-weight_i;										//subtract from intial weight
			
			dtostrf(weight, 3, 3, printweight);
			LCD_gotoxy(1, 1);											// Go to the location 1,1 of lcd
			LCD_print(printraw);										// Print the text
			LCD_gotoxy(1, 2);											// Go to the location 1,2 of lcd
			LCD_print(printweight);										// Print the text
		}
	}
	
	
}
double question_1(void){
	double amount_of_food=0;
	int reset_1=1;//,i=0,p=0,flag=0;
	while(reset_1==1){
		reset_1=1;
		clear_LCD();
		LCD_gotoxy(1, 1);
		LCD_print("How much food?");
		LCD_gotoxy(1, 2);
		while ((PINC == (PINC | 0b00001000)))
		{
			amount_of_food=food_options();
			LCD_gotoxy(1, 2);		
			print_food(amount_of_food);
		}
		reset_1=0;
	}
	return amount_of_food;
}
double food_options(void){
	double amount_of_food=0;
	ADCSRA = ADCSRA | 0b01000000; //Start conversion, by setting ADSC bit
	while((ADCSRA & 0b00010000) == 0){}   //while ADIF flag(bit 4) is clear(0) the conversion is not finished yet
	pot_reading = ADCH; //Left justified register is the results in binary value, the last two bits are thrown away
	ADCSRA = ADCSRA | 0b01000000; //Start conversion, by setting ADSC bit
	while((ADCSRA & 0b00010000) == 0){}   //while ADIF flag(bit 4) is clear(0) the conversion is not finished yet
	pot_reading = ADCH; //Left justified register is the results in binary value, the last two bits are thrown away
	if (pot_reading<=32){
		amount_of_food=.25;
		print_food(amount_of_food);
	}
	else if(pot_reading>32 && pot_reading<=64){
		amount_of_food=.50;
		print_food(amount_of_food);
	}
	else if(pot_reading>64 && pot_reading<=96){
		amount_of_food=.75;
		print_food(amount_of_food);
	}
	else if(pot_reading>96 && pot_reading<=128){
		amount_of_food=1;
		print_food(amount_of_food);
	}
	else if(pot_reading>128 && pot_reading<=160){
		amount_of_food=1.5;
		print_food(amount_of_food);
	}
	else if(pot_reading>160 && pot_reading<=192){
		amount_of_food=2;
		print_food(amount_of_food);
	}
	else if(pot_reading>192 && pot_reading<=224){
		amount_of_food=3;
		print_food(amount_of_food);
	}
	else if(pot_reading>224 && pot_reading<=255){
		amount_of_food=4;
		print_food(amount_of_food);
	}
	
	return amount_of_food;
}
int send_amount_of_food(double amount_of_food){
	int dummy=0;
	if (amount_of_food==.25)
	{
		dummy=0;
	}
	if (amount_of_food==.50)
	{
		dummy=8;
	}
	if (amount_of_food==.75)
	{
		dummy=16;
	}
	if (amount_of_food==1)
	{
		dummy=24;
	}
	if (amount_of_food==1.5)
	{
		dummy=40;
	}
	if (amount_of_food==2)
	{
		dummy=56;
	}
	if (amount_of_food==3)
	{
		dummy=48;
	}
	if (amount_of_food==4)
	{
		dummy=32;
	}
	return dummy;
}
void print_food(double food){
	dtostrf(food, 3, 3, printweight);
	LCD_print(printweight);
	LCD_print(" cups       ");

}
int  clock_set_hour(){
	int hour=0;
	ADCSRA = ADCSRA | 0b01000000; //Start conversion, by setting ADSC bit
	while((ADCSRA & 0b00010000) == 0){}   //while ADIF flag(bit 4) is clear(0) the conversion is not finished yet
	pot_reading = ADCH; //Left justified register is the results in binary value, the last two bits are thrown away
	ADCSRA = ADCSRA | 0b01000000; //Start conversion, by setting ADSC bit
	if (pot_reading<=42){
		hour=1;
		print_hour(hour);
	}
	else if(pot_reading>42 && pot_reading<=63){
		hour=2;
		print_hour(hour);
	}
	else if(pot_reading>63 && pot_reading<=84){
		hour=3;
		print_hour(hour);
	}
	else if(pot_reading>84 && pot_reading<=105){
		hour=4;
		print_hour(hour);
	}
	else if(pot_reading>105 && pot_reading<=126){
		hour=5;
		print_hour(hour);
	}
	else if(pot_reading>126 && pot_reading<=147){
		hour=7;
		print_hour(hour);
	}
	else if(pot_reading>147 && pot_reading<=168){
		hour=8;
		print_hour(hour);
	}
	else if(pot_reading>168 && pot_reading<=189){
		hour=9;
		print_hour(hour);
	}
	else if(pot_reading>189 && pot_reading<=210){
		hour=10;
		print_hour(hour);
	}
	else if(pot_reading>210 && pot_reading<=231){
		hour=11;
		print_hour(hour);
	}	
	else if(pot_reading>231 && pot_reading<=255){
		hour=12;
		print_hour(hour);
	}	
	return hour;	
}
int  clock_set_min_ten(){
	int min_ten=0;
	ADCSRA = ADCSRA | 0b01000000; //Start conversion, by setting ADSC bit
	while((ADCSRA & 0b00010000) == 0){}   //while ADIF flag(bit 4) is clear(0) the conversion is not finished yet
	pot_reading = ADCH; //Left justified register is the results in binary value, the last two bits are thrown away
	ADCSRA = ADCSRA | 0b01000000; //Start conversion, by setting ADSC bit
	if (pot_reading<=42){
		min_ten=0;
		print_min_ten(min_ten);
	}
	else if(pot_reading>42 && pot_reading<=84){
		min_ten=1;
		print_min_ten(min_ten);
	}
	else if(pot_reading>84 && pot_reading<=126){
		min_ten=2;
		print_min_ten(min_ten);
	}
	else if(pot_reading>126 && pot_reading<=168){
		min_ten=3;
		print_min_ten(min_ten);
	}
	else if(pot_reading>168 && pot_reading<=210){
		min_ten=4;
		print_min_ten(min_ten);
	}
	else if(pot_reading>210 && pot_reading<=255){
		min_ten=5;
		print_min_ten(min_ten);
	}	
	return min_ten;
}
int  clock_set_min_one(){
	int min_one=0;
	ADCSRA = ADCSRA | 0b01000000; //Start conversion, by setting ADSC bit
	while((ADCSRA & 0b00010000) == 0){}   //while ADIF flag(bit 4) is clear(0) the conversion is not finished yet
	pot_reading = ADCH; //Left justified register is the results in binary value, the last two bits are thrown away
	ADCSRA = ADCSRA | 0b01000000; //Start conversion, by setting ADSC bit
	if (pot_reading<=28){
		min_one=0;
		print_min_one(min_one);
	}
	else if(pot_reading>28 && pot_reading<=40){
		min_one=1;
		print_min_one(min_one);
	}
	else if(pot_reading>40 && pot_reading<=56){
		min_one=2;
		print_min_one(min_one);
	}
	else if(pot_reading>56 && pot_reading<=84){
		min_one=3;
		print_min_one(min_one);
	}
	else if(pot_reading>84 && pot_reading<=112){
		min_one=4;
		print_min_one(min_one);
	}
	else if(pot_reading>112 && pot_reading<=140){
		min_one=5;
		print_min_one(min_one);
	}
	else if(pot_reading>140 && pot_reading<=168){
		min_one=6;
		print_min_one(min_one);
	}
	else if(pot_reading>168 && pot_reading<=196){
		min_one=7;
		print_min_one(min_one);
	}
	else if(pot_reading>196 && pot_reading<=224){
		min_one=8;
		print_min_one(min_one);
	}
	else if(pot_reading>224 && pot_reading<=255){
		min_one=9;
		print_min_one(min_one);
	}
	return min_one;
}
void print_hour(int hour){
	if(hour<10){
		LCD_gotoxy(1, 1);
		LCD_print(" ");
		LCD_gotoxy(2, 1);
		itoa(hour,printraw,10);
		LCD_print(printraw);
	}
	else{
		LCD_gotoxy(1, 1);
		itoa(hour,printraw,10);
		LCD_print(printraw);
	}
}
int  clock_set_mer(void){
	int meridiem=0;
	ADCSRA = ADCSRA | 0b01000000; //Start conversion, by setting ADSC bit
	while((ADCSRA & 0b00010000) == 0){}   //while ADIF flag(bit 4) is clear(0) the conversion is not finished yet
	pot_reading = ADCH; //Left justified register is the results in binary value, the last two bits are thrown away
	if (pot_reading<=128){
		meridiem=0;
	}
	else{
		meridiem=1;
	}
	print_mer(meridiem);
	return meridiem;
}
void print_min_ten(int min_ten){
	LCD_gotoxy(4, 1);
	itoa(min_ten,printraw,10);
	LCD_print(printraw);
}
void print_min_one(int min_one){
	LCD_gotoxy(5, 1);
	itoa(min_one,printraw,10);
	LCD_print(printraw);
}	
void print_mer(int merid){
	if (merid==0){
		LCD_gotoxy(7, 1);
		LCD_print("AM");
	}
	else{
		LCD_gotoxy(7, 1);
		LCD_print("PM");
	}
}
int  int_of_feed(void){
	int hour_int=0;
	ADCSRA = ADCSRA | 0b01000000; //Start conversion, by setting ADSC bit
	while((ADCSRA & 0b00010000) == 0){}   //while ADIF flag(bit 4) is clear(0) the conversion is not finished yet
	pot_reading = ADCH; //Left justified register is the results in binary value, the last two bits are thrown away
	ADCSRA = ADCSRA | 0b01000000; //Start conversion, by setting ADSC bit
	if (pot_reading<=85){
		hour_int=6;
		print_hour_int(hour_int);
	}
	else if(pot_reading>85 && pot_reading<=170){
		hour_int=12;
		print_hour_int(hour_int);
	}
	else if(pot_reading>170 && pot_reading<=255){
		hour_int=24;
		print_hour_int(hour_int);
	}
	return hour_int;
}
void print_hour_int(int hour_int){
if(hour_int<10){
	LCD_gotoxy(1, 2);
	LCD_print(" ");
	LCD_gotoxy(2, 2);
	itoa(hour_int,printraw,10);
	LCD_print(printraw);
}
else{
	LCD_gotoxy(1, 2);
	itoa(hour_int,printraw,10);
	LCD_print(printraw);
}
	LCD_gotoxy(4, 2);
	LCD_print("hours");
}
void print_time(int hour,int min_ten,int min_one,int merid){
	print_hour(hour);
	LCD_gotoxy(3, 1);
	LCD_print(":");
	print_min_ten(min_ten);
	print_min_one(min_one);
	print_mer(merid);	
}
void clear_LCD(void){
	LCD_gotoxy(1, 1);
	LCD_print("                       ");
	LCD_gotoxy(1, 2);
	LCD_print("                     ");
		
}
void print_food_time(int hour,int min_ten,int min_one,int merid){
	LCD_gotoxy(1, 2);
	LCD_print("FoodT:");
	if(hour<10){
		LCD_gotoxy(8, 2);
		LCD_print(" ");
		LCD_gotoxy(9, 2);
		itoa(hour,printraw,10);
		LCD_print(printraw);
	}
	else{
		LCD_gotoxy(8, 2);
		itoa(hour,printraw,10);
		LCD_print(printraw);
	}
		LCD_gotoxy(10, 2);
		LCD_print(":");
	LCD_gotoxy(11, 2);
	itoa(min_ten,printraw,10);
	LCD_print(printraw);
	LCD_gotoxy(12, 2);
	itoa(min_one,printraw,10);
	LCD_print(printraw);
	if (merid==0){
	LCD_gotoxy(14, 2);
	LCD_print("AM");
	}
	else{
	LCD_gotoxy(14, 2);
	LCD_print("PM");
		}
}
int switch_merid(int merid){
	if (merid==0){
		merid=1;
	}
	if (merid==1){
		merid=0;
	}
	return merid;
}
void uc2(void){
	sei();
	PORTC=PORTC | 0b00000001;				//Send signal to UC2 to start feeding
	clear_LCD();
	//wait(2000);		
	LCD_gotoxy(1,1);
	LCD_print("FOOD TIME!       ");
	LCD_gotoxy(1,2);
	LCD_print("In Progress       ");
	while (PINC == (PINC | 0b00000100)){ // Wait for UC2 to send back that its done

	}
	PORTC=PORTC & 0b11111110;			  //Reset signal to send to UC2 to start feeding
	clear_LCD();
}
ISR(INT0_vect){
	////Overide Interupt
	
	//sei();
	clear_LCD();
	LCD_gotoxy(1,1);
	LCD_print("    OVERRIDE");
	uc2();
}
ISR(INT1_vect){
	PORTC=PORTC & 0b11101111;	//Clear bit tells UC2 measurement is done
	clear_LCD();
	LCD_gotoxy(1,1);
	LCD_print("Measuring Food       ");
	double weight_i = hx711_getweight();		//Get initial weight
	double density=134/2;							//g/cups 
	double weight_2;		
	weight_2=amount_of_food*density;			//Get weight of amount food
	int i=1;
	while(i==1){
			double weight = hx711_getweight();	//read weight
			//int32_t read = hx711_read();
			//ltoa(read, printraw, 10);
			weight=weight-weight_i;				//subtract from intial weight
			//weight=-weight;
			LCD_gotoxy(1,2);
			dtostrf(weight, 3, 3, printweight);
			LCD_print(printweight);
			if (weight>weight_2){				//When weight is above food, end loop
				clear_LCD();
				i=0;
				PORTC=PORTC | 0b0000010000;		//Send signal to UC2 that its measuring
			}
	}
	clear_LCD();
}
