/*
INSTRUCTIONS FOR USE
Take the cuff pressure to 200, Data is shown in serial monitor. When it reaches 200, steady your arm and press the USER button on the microcontoller
Make sure your arm doesn't move when the pressure is between 180 and 40 This is when data is recorded and operated on.

When the pressure goes below 40 you can press the user button again and your Systolic and diastolic values will be present in the serial monitor
*/






#include <mbed.h>
#include <USBSerial.h>
#include <chrono>
#include <cstdlib>


USBSerial serial;

InterruptIn buttonInterrupt(USER_BUTTON);     //used for button interrupt
volatile uint32_t myTick = 0;
volatile uint32_t myTick2 = 0;

// pb8 i2c scl
// pb9 i2c sda/MOSI
I2C i2c(I2C_SDA, I2C_SCL);

const int addr7bit = 0x18;      // 7 bit I2C address
const int addr8bit = 0x18 << 1; 
const int addrwrite = 0x30;
const int addrread = 0x31;
char data[4];
char status[1];
u_int32_t pressure_data[4];
int btnval=0;
int datacounter=0;
int sdc=2;
u_int32_t fa[10000];
int pulse=0;
uint32_t systolic=0;
uint32_t diastolic=0;
int reader=0;
uint32_t smooth[10000];
uint32_t parr[7]={0,0,0,0,0,0,0};


void btndebounce(void){           //eliminate button debounce
  volatile static uint32_t lastTick = 0;

  // check if 300ms has passed since the last real press
  if (myTick - lastTick > 300) {

    btnval= !btnval;

    // save the time of the real press
    lastTick = myTick;
  }
}

// system tick ISR
void myTicker(void) {
  myTick++;
}

void myTicker2(void){
  myTick2++;
}

// int pulseCalc(){      //calculating pulse

//       Pulse = 5*

//   return pulse;
// }

int main()
{   int j=0;
    for(int i=0;i<10000;i++){
      fa[i]=0;
      smooth[i]=0;
    }
    u_int32_t cmd[3];

      // Setup the system tick
      Ticker t2;
  Ticker t;
  t.attach_us(&myTicker, 1000);

  // set the interrupt to fire on a rising edge
  buttonInterrupt.rise(&btndebounce);

    while (1) {

        

        cmd[0] = 0xAA;
        cmd[1] = 0x00;
        cmd[2] = 0x00;

        i2c.write(addrwrite, (const char*)cmd, 3);

        wait_ms(5);

       
        i2c.read(addrread, status, 1);
        wait_ms(5);
        //serial.printf("%d\n",status[0]);
         i2c.read(addrread, data, 4);
           pressure_data[0] = data[0];
           pressure_data[1] =  ( u_int32_t)data[1];
           pressure_data[2] =  ( u_int32_t)data[2];
           pressure_data[3] =  ( u_int32_t)data[3];

        
          u_int32_t output =  u_int32_t(pressure_data[1]<<16|pressure_data[2]<<8|pressure_data[3]);
        //   serial.printf("%d\n",output);

        u_int32_t output_min = 419430;
        u_int32_t output_max = 3774873 ;
        u_int32_t p_max = 300;
        u_int32_t p_min = 0;

        u_int32_t  pressure = ((output - output_min)*(p_max -p_min))/(output_max-output_min) + p_min;   //Pressure data 
        //serial.printf("%d, ",pressure);
        if (pressure>180){
          reader=1;               //pressure spikes are only measured when user takes the intital pressure to >180, This ensures acccurate readings
        }
        if (pressure<40){
          reader=0;             //Similarly, No readings are recorded when pressure drops below 40
        }
        
        if(btnval==1){          //readings recorded only after user button is pressed
          
            if(datacounter<10000){
                  fa[datacounter]=pressure; 
                  
                  //serial.printf("fa %d,Reader %d, ",fa[datacounter],reader);
                  

              if(datacounter>10 && datacounter<9990){       //This is used to reduce noise in data
                  int temp = (fa[datacounter-2]+fa[datacounter-3]+fa[datacounter-4]+fa[datacounter-1]+fa[datacounter])/5;
                if(smooth[sdc-1]!=temp){
                smooth[sdc]=temp;
                serial.printf("%d, ",smooth[sdc]);
                sdc++;
                }



              if(systolic==0 && reader ==1 && smooth[sdc-1]==smooth[sdc-3] && smooth[sdc-2]>smooth[sdc-4]){   //This condition identifies pressure peak caused by heart (AKA, Systolic pressure)
                  systolic = smooth[sdc-1];
                  serial.printf("Systolic %d, ",systolic);
              }

              if(smooth[sdc-1]==smooth[sdc-3] && reader ==1 && smooth[sdc-2]>smooth[sdc-4]){    //This is the last pressure peak (AKA, Diastolic)
                  diastolic = smooth[sdc-1];
                  if(j==0){
                    t2.attach_us(&myTicker2, 1000);
                    j++;
                  }
                  if(diastolic !=0&& j<6 &&j!=0){
                    parr[j]=diastolic;
                    j++;
                  }
                  else if(j==6){
                    t2.detach();
                    pulse = 5*60000/myTick2;     //counting duration of 5 pulses in myTick2 milliseconds. -> 
                    j++;

                  }
                  //serial.printf("diastolic %d, ",diastolic);
                  
              }





              }
              //serial.printf("Your Sys and Dias are %d and %d",systolic, diastolic);
              datacounter++;



            }
            else{
                  serial.printf("\nData cap reached, Restart device\n");      //Since data is saved in an array, it can only hold so much, This throws an exception when array is overloaded.
            }
          //
        }
        else{
          serial.printf("Press User Button to start reading\n\n");
          serial.printf("%d, ",pressure);
          serial.printf("Systolic %d, ",systolic);
          serial.printf("diastolic %d, ",diastolic);
          
          serial.printf("Pulse %d, ",pulse);
          wait_ms(1000);
          
              //These print commands make it easier for the user to interact with the device
        }
          
    }
}