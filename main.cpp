#include <Arduino.h>
#include <Adafruit_MCP3008.h>
#include <Encoder.h>

const unsigned int M1_ENC_A = 39;
const unsigned int M1_ENC_B = 38;
const unsigned int M2_ENC_A = 37;
const unsigned int M2_ENC_B = 36;

Adafruit_MCP3008 adc1;
Adafruit_MCP3008 adc2;

const unsigned int ADC_1_CS = 2;
const unsigned int ADC_2_CS = 17;

const unsigned int M1_IN_1 = 13;
const unsigned int M1_IN_2 = 12;
const unsigned int M2_IN_1 = 25;
const unsigned int M2_IN_2 = 14;

const unsigned int M1_IN_1_CHANNEL = 8;
const unsigned int M1_IN_2_CHANNEL = 9;
const unsigned int M2_IN_1_CHANNEL = 10;
const unsigned int M2_IN_2_CHANNEL = 11;

const unsigned int M1_I_SENSE = 35;
const unsigned int M2_I_SENSE = 34;

// const float M_I_COUNTS_TO_A = (3.3 / 1024.0) / 0.120;

const unsigned int PWM_VALUE = 1024; // Max PWM given 10 bit resolution

const int freq = 5000;
const int ledChannel = 0;
const int resolution = 10;

int adc1_buf[8];
int adc2_buf[8];

float mid = 6.5;
int base_pid = 300; //255, 512 too fast, start lower and incrementally increase over time, this is speed

uint8_t lineArray[13]; 
float previousPosition = 6;


float error;
float last_error;
float total_error;

float Kp = 12; //2 8
float Kd = 300; //100 125? Change to better go around corners
float Ki = 0;

int move_once = 0;
int block_order = 0;

void M1_backward(int pwm_value) {
  ledcWrite(M1_IN_1_CHANNEL, pwm_value);
  ledcWrite(M1_IN_2_CHANNEL, 0);
}

void M1_forward(int pwm_value) {
  ledcWrite(M1_IN_1_CHANNEL, 0);
  ledcWrite(M1_IN_2_CHANNEL, pwm_value);
}

void M1_stop() {
  ledcWrite(M1_IN_1_CHANNEL, PWM_VALUE);
  ledcWrite(M1_IN_2_CHANNEL, PWM_VALUE);
}

void M2_backward(int pwm_value) {
  ledcWrite(M2_IN_1_CHANNEL, pwm_value);
  ledcWrite(M2_IN_2_CHANNEL, 0);
}

void M2_forward(int pwm_value) {
  ledcWrite(M2_IN_1_CHANNEL, 0);
  ledcWrite(M2_IN_2_CHANNEL, pwm_value);
}

void M2_stop() {
  ledcWrite(M2_IN_1_CHANNEL, PWM_VALUE);
  ledcWrite(M2_IN_2_CHANNEL, PWM_VALUE);
}


void readADC() {
  for (int i = 0; i < 8; i++) {
    adc1_buf[i] = adc1.readADC(i);
    adc2_buf[i] = adc2.readADC(i);
  }
}

void digitalConvert(){
  for (int i = 0; i < 7; i++) {
    if (adc1_buf[i]>700) {
      lineArray[2*i] = 1; 
    } else {
      lineArray[2*i] = 0;
    }
    Serial.print(lineArray[2*i]); Serial.print("\t");
    //Serial.print(adc1_buf[i]); Serial.print("\t");

    if (i<6) {
      if (adc2_buf[i]>700){
        lineArray[2*i+1] = 1;
      } else {
        lineArray[2*i+1] = 0;
      }
      Serial.print(lineArray[2*i+1]); Serial.print("\t");
      //Serial.print(adc2_buf[i]); Serial.print("\t");
    }
  }
  Serial.print("\n");
}

float getPosition(float previousPosition) {
  
  float pos = 0;
  uint8_t white_count = 0;
  for (int i = 0; i < 13; i++) {
    if (lineArray[i] == 0) {
      pos += i;
      white_count+=1;
    } 
  }

  // Serial.print("white: "); Serial.print(white_count); Serial.print("\t");
  // Serial.print("pos: "); Serial.print(pos); Serial.print("\t");
  if (white_count == 0) {
    return previousPosition;
  }
  return pos/white_count;
}
void setup() {
  // Stop the right motor by setting pin 14 low
  // this pin floats high or is pulled
  // high during the bootloader phase for some reason

  pinMode(14, OUTPUT);
  digitalWrite(14, LOW);
  delay(100);

  Serial.begin(115200);

  ledcSetup(M1_IN_1_CHANNEL, freq, resolution);
  ledcSetup(M1_IN_2_CHANNEL, freq, resolution);
  ledcSetup(M2_IN_1_CHANNEL, freq, resolution);
  ledcSetup(M2_IN_2_CHANNEL, freq, resolution);

  ledcAttachPin(M1_IN_1, M1_IN_1_CHANNEL);
  ledcAttachPin(M1_IN_2, M1_IN_2_CHANNEL);
  ledcAttachPin(M2_IN_1, M2_IN_1_CHANNEL);
  ledcAttachPin(M2_IN_2, M2_IN_2_CHANNEL);

  adc1.begin(ADC_1_CS);  
  adc2.begin(ADC_2_CS);

  pinMode(M1_I_SENSE, INPUT);
  pinMode(M2_I_SENSE, INPUT);

  delay(5000);

}

void loop() {


  //if(block_order == 0){
    // //Travel 6 inches straight speed = 450, kp = 3, kd = 0, ki = 0
  //   Encoder enc1(M1_ENC_A, M1_ENC_B);
  //   Encoder enc2(M2_ENC_A, M2_ENC_B);
  //   long enc1_value = enc1.read();
  //   long enc2_value = enc2.read();
  //   long dist_travelled = (enc1_value/154.32)*1.26*PI;

  //   while((dist_travelled < 18) && (move_once == 0)){ //should be 6 :/
  //       //travel forwards
  //       error = enc1_value + enc2_value;
  //       total_error += error;
  //       int pid_value = Kp*error + Kd*(error-last_error) + Ki*total_error;
  //       int right_motor = base_pid + pid_value;
  //       int left_motor = base_pid - pid_value;
  //       M1_forward(left_motor);
  //       M2_forward(right_motor);
  //       enc1_value = enc1.read();
  //       enc2_value = enc2.read();
  //       dist_travelled = (enc1_value/154.32)*1.26*PI;
  //       last_error = error;
  //   }
  //   move_once = 0;
  //   M1_stop();
  //   M2_stop();
  //   delay(2000);
  //   block_order = 1;
  // }else if(block_order == 1){
  //   //Turn left 90 degrees: speed = 450, kp = 3, kd = 0, ki = 0
  //   Encoder enc1(M1_ENC_A, M1_ENC_B);
  //   Encoder enc2(M2_ENC_A, M2_ENC_B); //key at AVW 1338
  //   long enc1_value = enc1.read();
  //   long enc2_value = enc2.read(); //right motor
  //   while(enc2_value > -212){//3 3/8 inches 240 - try sensor fusion
  //     error = enc2_value - enc1_value; //may have to subtract instead of add these
  //     total_error += error;
  //     int pid_value = Kp*error + Kd*(error-last_error) + Ki*total_error;
  //     int right_motor = base_pid + pid_value;
  //     int left_motor = base_pid - pid_value;


  //     // Serial.print("left_enc: \t");Serial.print(enc1_value);Serial.print("\t right_enc: \t");Serial.println(enc2_value);
  //     // Serial.print("Enc2 Degrees: \t"); Serial.print((enc2_value/154.32)); Serial.print("\t");
  //     // Serial.print("left: \t"); Serial.print(left_motor);Serial.print("right: \t"); Serial.println(right_motor); 
  //     M1_backward(left_motor);
  //     M2_forward(right_motor);
  //     enc1_value = enc1.read();
  //     enc2_value = enc2.read();
  //     last_error = error;
  //   }
  //   M1_stop();
  //   M2_stop();
  //   delay(2000);
  //   block_order = 0;
  // }

  // //Turn right 90 degrees: speed = 450, kp = 3, kd = 0, ki = 0
  // Encoder enc1(M1_ENC_A, M1_ENC_B);
  // Encoder enc2(M2_ENC_A, M2_ENC_B); //key at AVW 1338
  // long enc1_value = enc1.read();
  // long enc2_value = enc2.read(); //right motor
  // // Serial.print("left_enc: \t");Serial.print(enc1_value);
  // while(enc1_value < 212){//3 3/8 inches 240 - try sensor fusion
  //   error = enc2_value - enc1_value; //may have to subtract instead of add these
  //   total_error += error;
  //   int pid_value = Kp*error + Kd*(error-last_error) + Ki*total_error;
  //   int right_motor = base_pid + pid_value;
  //   int left_motor = base_pid - pid_value;


  //   // Serial.print("left_enc: \t");Serial.print(enc1_value);Serial.print("\t right_enc: \t");Serial.println(enc2_value);
  //   // Serial.print("Enc2 Degrees: \t"); Serial.print((enc2_value/154.32)); Serial.print("\t");
  //   Serial.print("left: \t"); Serial.print(left_motor);Serial.print("right: \t"); Serial.println(right_motor); 
  //   M1_forward(left_motor);
  //   M2_backward(right_motor);
  //   enc1_value = enc1.read();
  //   enc2_value = enc2.read();
  //   last_error = error;
  // }
  // M1_stop();
  // M2_stop();
  // delay(2000);


  // // For straight continuous line: speed = 350, kp = 2, kd = 100, ki = 0
  // // For dotted line: speed = 300, kp = 12, kd = 300, ki = 0
  int t_start = micros();
  readADC();
  int t_end = micros();

  digitalConvert();

  float pos = getPosition(previousPosition);
  previousPosition = pos;

  error = pos - mid;
  total_error += error;

  int pid_value = Kp*error + Kd*(error-last_error) + Ki*total_error;
  int right_motor = base_pid + pid_value;
  int left_motor = base_pid - pid_value;

  M1_forward(left_motor);
  M2_forward(right_motor);

  //Serial.print("time: \t"); Serial.print(t_end - t_start); Serial.print("\t");
  //Serial.print("pos: \t"); Serial.print(pos);Serial.print("right: \t"); Serial.print(right_motor); 
  // Serial.print("left: \t"); Serial.print(left_motor);Serial.print("right: \t"); Serial.println(right_motor); 
  // Serial.println();


  last_error = error;

  int all_white = 0;
	for (int i = 0; i < 13; i++) {
    		if (lineArray[i] == 0) {
      		 all_white+=1;
    		} 
  	}
	if(all_white == 13) {
	   M1_stop();
     M2_stop();
     delay(2000);
	}

  //delay(100);

}
