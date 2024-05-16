#include <Arduino.h>
#include <Adafruit_MCP3008.h>
#include <Encoder.h>
#include <WiFi.h>
#include <WiFiClient.h>


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

const int freq = 2500;
const int ledChannel = 0;
const int resolution = 10;

int adc1_buf[8];
int adc2_buf[8];

float mid = 6.5;
int base_pid = 350; // 255, 512 too fast, start lower and incrementally increase over time, this is speed

uint8_t lineArray[13];
float previousPosition = 6;

float error;
float last_error;
float total_error;

float Kp = 8;   // 2 8
float Kd = 300; // 100 125? Change to better go around corners
float Ki = 0;

int move_once = 0;
int block_order = 0;
int transition_done = 0;
int sound_direction = 0; // 0 = robot's right, 1 = robot's left
int first_time = 0;
int second_time = 0;

const char* ssid = "RobotWifi";
const char* password = "passwordpassword";
const char* host = "192.168.4.2";
const uint16_t port = 8080;
const char* message = "leader:checkin";


void M1_backward(int pwm_value)
{
    ledcWrite(M1_IN_1_CHANNEL, pwm_value);
    ledcWrite(M1_IN_2_CHANNEL, 0);
}

void M1_forward(int pwm_value)
{
    ledcWrite(M1_IN_1_CHANNEL, 0);
    ledcWrite(M1_IN_2_CHANNEL, pwm_value);
}

void M1_stop()
{
    ledcWrite(M1_IN_1_CHANNEL, PWM_VALUE);
    ledcWrite(M1_IN_2_CHANNEL, PWM_VALUE);
}

void M2_backward(int pwm_value)
{
    ledcWrite(M2_IN_1_CHANNEL, pwm_value);
    ledcWrite(M2_IN_2_CHANNEL, 0);
}

void M2_forward(int pwm_value)
{
    ledcWrite(M2_IN_1_CHANNEL, 0);
    ledcWrite(M2_IN_2_CHANNEL, pwm_value);
}

void M2_stop()
{
    ledcWrite(M2_IN_1_CHANNEL, PWM_VALUE);
    ledcWrite(M2_IN_2_CHANNEL, PWM_VALUE);
}

void readADC()
{
    for (int i = 0; i < 8; i++)
    {
        adc1_buf[i] = adc1.readADC(i);
        adc2_buf[i] = adc2.readADC(i);
    }
}

void digitalConvert()
{
    for (int i = 0; i < 7; i++)
    {
        if (adc1_buf[i] > 700)
        {
            lineArray[2 * i] = 1;
        }
        else
        {
            lineArray[2 * i] = 0;
        }
        // Serial.print(lineArray[2*i]); Serial.print("\t");
        // Serial.print(adc1_buf[i]); Serial.print("\t");

        if (i < 6)
        {
            if (adc2_buf[i] > 700)
            {
                lineArray[2 * i + 1] = 1;
            }
            else
            {
                lineArray[2 * i + 1] = 0;
            }
            // Serial.print(lineArray[2*i+1]); Serial.print("\t");
            // Serial.print(adc2_buf[i]); Serial.print("\t");
        }
    }
    Serial.print("\n");
}

float getPosition(float previousPosition)
{

    float pos = 0;
    uint8_t white_count = 0;
    for (int i = 0; i < 13; i++)
    {
        if (lineArray[i] == 0)
        {
            pos += i;
            white_count += 1;
        }
    }

    // Serial.print("white: "); Serial.print(white_count); Serial.print("\t");
    // Serial.print("pos: "); Serial.print(pos); Serial.print("\t");
    if (white_count == 0)
    {
        return previousPosition;
    }
    return pos / white_count;
}

void move_straight(Encoder &enc1, Encoder &enc2, int dist)
{
    enc1.readAndReset();
    enc2.readAndReset();
    base_pid = 300; // 450
    Kp = 3;         // 3
    Kd = 0;         // 0
    Ki = 0;
    long enc1_value = enc1.read();
    long enc2_value = enc2.read();
    long dist_travelled = (enc1_value / 154.32) * 1.26 * PI;

    while ((dist_travelled < dist) && (move_once == 0))
    { // should be 6 :/
        // travel forwards
        error = enc1_value + enc2_value;
        total_error += error;
        int pid_value = Kp * error + Kd * (error - last_error) + Ki * total_error;
        int right_motor = base_pid + pid_value;
        int left_motor = base_pid - pid_value;
        M1_forward(left_motor);
        M2_forward(right_motor);
        enc1_value = enc1.read();
        enc2_value = enc2.read();
        dist_travelled = (enc1_value / 154.32) * 1.26 * PI;
        last_error = error;
    }
    move_once = 0;
    M1_stop();
    M2_stop();
    delay(2000);
}

void turn_left90(Encoder &enc1, Encoder &enc2)
{
    enc1.readAndReset();
    enc2.readAndReset();
    base_pid = 300; // 450
    Kp = 1;         // 3
    Kd = 10;        // 0
    Ki = 0;
    long enc1_value = enc1.read();
    long enc2_value = enc2.read(); // right motor
    while (enc2_value > -200)
    { // 3 3/8 inches -240 - try sensor fusion
        error = enc2_value - enc1_value;
        total_error += error;
        int pid_value = Kp * error + Kd * (error - last_error) + Ki * total_error;
        int right_motor = base_pid + pid_value;
        int left_motor = base_pid - pid_value;

        // Serial.print("left_enc: \t");Serial.print(enc1_value);Serial.print("\t right_enc: \t");Serial.println(enc2_value);
        // Serial.print("Enc2 Degrees: \t"); Serial.print((enc2_value/154.32)); Serial.print("\t");
        // Serial.print("left: \t"); Serial.print(left_motor);Serial.print("right: \t"); Serial.println(right_motor);
        M1_backward(left_motor);
        M2_forward(right_motor);
        enc1_value = enc1.read();
        enc2_value = enc2.read();
        last_error = error;
    }
    M1_stop();
    M2_stop();
    delay(2000);
}

void turn_right90(Encoder &enc1, Encoder &enc2)
{
    enc1.readAndReset();
    enc2.readAndReset();
    base_pid = 300; // 450
    Kp = 1;         // 3
    Kd = 10;        // 0
    Ki = 0;
    long enc1_value = enc1.read();
    long enc2_value = enc2.read(); // right motor
    while (enc1_value < 240)
    {                                    // 3 3/8 inches 240 - try sensor fusion, 280 maybe
        error = enc2_value - enc1_value; // may have to subtract instead of add these
        total_error += error;
        int pid_value = Kp * error + Kd * (error - last_error) + Ki * total_error;
        int right_motor = base_pid - pid_value;
        int left_motor = base_pid + pid_value;

        // Serial.print("left_enc: \t");Serial.print(enc1_value);Serial.print("\t right_enc: \t");Serial.println(enc2_value);
        // Serial.print("Enc2 Degrees: \t"); Serial.print((enc2_value/154.32)); Serial.print("\t");
        // Serial.print("left: \t"); Serial.print(left_motor);Serial.print("right: \t"); Serial.println(right_motor);
        M1_forward(left_motor);
        M2_backward(right_motor);
        enc1_value = enc1.read();
        enc2_value = enc2.read();
        last_error = error;
    }
    M1_stop();
    M2_stop();
    delay(2000);
}

void turn_180(Encoder &enc1, Encoder &enc2)
{
    enc1.readAndReset();
    enc2.readAndReset();
    base_pid = 300; // 450
    Kp = 1;         // 3
    Kd = 10;        // 0
    Ki = 0;
    long enc1_value = enc1.read();
    long enc2_value = enc2.read(); // right motor
    while (enc1_value < 480)
    {                                    // 180 degrees by just doubling amount for 90?
        error = enc2_value - enc1_value; // may have to subtract instead of add these
        total_error += error;
        int pid_value = Kp * error + Kd * (error - last_error) + Ki * total_error;
        int right_motor = base_pid + pid_value;
        int left_motor = base_pid - pid_value;
        M1_forward(left_motor);
        M2_backward(right_motor);
        enc1_value = enc1.read();
        enc2_value = enc2.read();
        last_error = error;
    }
    M1_stop();
    M2_stop();
    delay(2000);
}

void white_line_follow(int spd, int kp_new, int kd_new)
{ // unsure if this one actually works or not.
    // For straight continuous line: speed = 350, kp = 8, kd = 300, ki = 0 or kp=8 to 12, kd=300
    // For dotted line: speed = 300, kp = 12, kd = 400, ki = 0
    base_pid = spd;
    Kp = kp_new;
    Kd = kd_new;
    Ki = 0;
    int t_start = micros();
    readADC();
    int t_end = micros();

    digitalConvert();

    float pos = getPosition(previousPosition);
    previousPosition = pos;

    error = pos - mid;
    total_error += error;

    int pid_value = Kp * error + Kd * (error - last_error) + Ki * total_error;
    int right_motor = base_pid + pid_value;
    int left_motor = base_pid - pid_value;

    M1_forward(left_motor);
    M2_forward(right_motor);

    // Serial.print("time: \t"); Serial.print(t_end - t_start); Serial.print("\t");
    // Serial.print("pos: \t"); Serial.print(pos);Serial.print("right: \t"); Serial.print(right_motor);
    //  Serial.print("left: \t"); Serial.print(left_motor);Serial.print("right: \t"); Serial.println(right_motor);
    //  Serial.println();
    last_error = error;
}

void right_turn_till_line(Encoder &enc1, Encoder &enc2)
{
    enc1.readAndReset();
    enc2.readAndReset();
    base_pid = 300; // 450
    Kp = 1;         // 3
    Kd = 10;        // 0
    Ki = 0;
    long enc1_value = enc1.read();
    long enc2_value = enc2.read(); // right motor
    int time_to_turn = 0;
    while (true)
    { // 3 3/8 inches 240 - try sensor fusion, check position of line
        readADC();
        digitalConvert();
        error = enc2_value - enc1_value;
        total_error += error;
        int pid_value = Kp * error + Kd * (error - last_error) + Ki * total_error;
        int right_motor = base_pid - pid_value;
        int left_motor = base_pid + pid_value;
        M1_forward(left_motor);
        M2_backward(right_motor);
        if ((lineArray[0] == 0) || (lineArray[1] == 0))
        {
            time_to_turn++;
        }
        if ((time_to_turn > 0) && ((lineArray[5] == 0) || (lineArray[6] == 0) || (lineArray[7] == 0)))
        { //(lineArray[4]==0)||
            break;
        }
        enc1_value = enc1.read();
        enc2_value = enc2.read();
        last_error = error;
    }
    M1_stop();
    M2_stop();
    delay(2000);
}

void left_turn_till_line(Encoder &enc1, Encoder &enc2)
{
    enc1.readAndReset();
    enc2.readAndReset();
    base_pid = 300; // 450
    Kp = 1;         // 3
    Kd = 10;        // 0
    Ki = 0;
    long enc1_value = enc1.read();
    long enc2_value = enc2.read(); // right motor
    int time_to_turn = 0;
    while (true)
    { // 3 3/8 inches -240 - try sensor fusion
        readADC();
        digitalConvert();
        error = enc2_value - enc1_value;
        total_error += error;
        int pid_value = Kp * error + Kd * (error - last_error) + Ki * total_error;
        int right_motor = base_pid + pid_value;
        int left_motor = base_pid - pid_value;
        M1_backward(left_motor);
        M2_forward(right_motor);
        if ((lineArray[12] == 0) || (lineArray[11] == 0))
        {
            time_to_turn++;
        }
        if ((time_to_turn > 0) && ((lineArray[4] == 0) || (lineArray[5] == 0) || (lineArray[6] == 0) || (lineArray[7] == 0)))
        {
            break;
        }
        enc1_value = enc1.read();
        enc2_value = enc2.read();
        last_error = error;
    }
    M1_stop();
    M2_stop();
    delay(2000);
}

void maze_straight(Encoder &enc1, Encoder &enc2)
{
    enc1.readAndReset();
    enc2.readAndReset();
    base_pid = 300; // 450
    Kp = 8;
    Kd = 300;
    Ki = 0;
    long enc1_value = enc1.read();
    long enc2_value = enc2.read();
    long dist_travelled = (enc1_value / 154.32) * 1.26 * PI;
    int empty_space = 0;
    int turn_white = 0;

    while ((dist_travelled < 8) && (move_once == 0))
    {
        // travel forwards
        readADC();
        digitalConvert();
        float pos = getPosition(previousPosition);
        previousPosition = pos;

        error = pos - mid;
        total_error += error;

        int pid_value = Kp * error + Kd * (error - last_error) + Ki * total_error;
        int right_motor = base_pid + pid_value;
        int left_motor = base_pid - pid_value;

        M1_forward(left_motor);
        M2_forward(right_motor);
        enc1_value = enc1.read();
        enc2_value = enc2.read();
        dist_travelled = (abs(enc1_value) / 154.32) * 1.26 * PI;
        last_error = error;
        if ((lineArray[5] == 1) && (lineArray[6] == 1) && (lineArray[7] == 1))
        {
            M1_stop();
            M2_stop();
            delay(1000);
            empty_space = 1;
            break;
        }
        turn_white = 0;
        for (int i = 0; i < 10; i++)
        {
            if (lineArray[i] == 0)
            {
                turn_white += 1;
            }
        }
        if (turn_white >= 5)
        {
            M1_stop();
            M2_stop();
            delay(1000);
            break;
        }
    }
    if (empty_space == 1)
    {
        move_straight(enc1, enc2, 5);
    }
    else if (turn_white >= 5)
    {
        move_straight(enc1, enc2, 3);
        right_turn_till_line(enc1, enc2);
    }
    else
    {
        M1_stop();
        M2_stop();
        delay(2000);
    }
    move_once = 0;
}

std::string extractMessage(const std::string &input)
{
    std::string prefix = "leader:";
    size_t pos = input.find(prefix);
    if (pos != std::string::npos)
    {
        size_t commaPos = input.find(',', pos);
        if (commaPos != std::string::npos)
        {
            return input.substr(pos + prefix.length(), commaPos - (pos + prefix.length()));
        }
    }
    return "";
}

void setup()
{

    // Stop the right motor by setting pin 14 low
    pinMode(14, OUTPUT);
    digitalWrite(14, LOW);
    delay(100);

    // Serial Processing
    Serial.begin(9600);

    // IO Setups
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

    // Wifi Setups
    bool host_1 = true;
    Serial.begin(9600);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    Serial.println("WiFi AP started");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    delay(5000);

    bool found_host = false;

    // Find IP Address
    while (!found_host)
    {
        if (host_1)
        {
            WiFiClient client;
            Serial.println("Searching for Host on 192.168.4.2");
            if (client.connect("192.168.4.2", port))
            {
                Serial.println("Found central control station");
                client.println(message);
                found_host = true;
                client.stop();
            }
            else
            {
                Serial.println("Not found on 192.168.4.2");
                host_1 = !host_1;
            }
            delay(100);
        }
        else
        {
            WiFiClient client;
            Serial.println("Searching for Host on 192.168.4.4");
            if (client.connect("192.168.4.4", port))
            {
                Serial.println("Found central control station");
                client.println(message);
                found_host = true;
                host = "192.168.4.4";
                client.stop();
            }
            else
            {
                Serial.println("Not found on 192.168.4.2");
                host_1 = !host_1;
            }
            delay(100);
        }
    }
    Serial.end();
}

void loop()
{ // key at AVW 1338, take video of it running in case it doesn't work day of 17th

  /*
    bool checkin_complete = false;
    Serial.begin(9600);

    while (!checkin_complete)
    {
        WiFiClient client;
        if (client.connect(host, port))
        {
            client.println(message);
            String response = client.readStringUntil('|');
            Serial.println(response);
            Serial.println("-------------- ^^ Something Recieved ^^ -----------");
            Serial.println("Connected to central control station");
            client.stop();
            Serial.println("\nMessage sent to central control station:");
            Serial.println(message);
            std::string recd_message = extractMessage(response.c_str());
            Serial.println("Recieved / Extracted Message: ");
            Serial.println(recd_message.c_str());
            if (recd_message == "begin")
            {
                message = "leader:active";
                checkin_complete = true;
            }
            else if (recd_message == "reset")
            {
                message = "leader:checkin";
                return;
            }
        }
        else
        {
            Serial.println("Connection to central control station failed");
        }
        delay(2000);
    }
    Serial.end();

    */
    Encoder enc1(M1_ENC_A, M1_ENC_B);
    Encoder enc2(M2_ENC_A, M2_ENC_B);
   
    move_straight(enc1, enc2, 10);

    // DO THE STUFF UNTIL SOUND



    /*
      //Line of the Republic
      move_straight(enc1, enc2, 12);
      while(true){
        white_line_follow(300, 8, 300);
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
          break;
        }
      }
      move_straight(enc1, enc2, 12);
      turn_right90(enc1, enc2);
      move_straight(enc1, enc2, 12);
      delay(2000);


      //Maze - 3 of each color block
      while(true){
        white_line_follow(300, 8, 300);
        int turn_white = 0;
        for(int i = 0; i < 10; i++){
          if (lineArray[i] == 0) {
              turn_white+=1;
          }
        }
        if(turn_white >= 5){
          M1_stop();
          M2_stop();
          delay(1000);
          break;
        }
      }
      move_straight(enc1,enc2,3);
      right_turn_till_line(enc1,enc2);

      while(true){
        white_line_follow(350, 8, 300);
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
          break;
        }
      }
      move_straight(enc1, enc2, 12);
      turn_left90(enc1, enc2);
      move_straight(enc1, enc2, 12);
      delay(2000);


      //segmented line, make sure to change K values and speed
      while(true){
        white_line_follow(220, 12, 500);
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
          break;
        }
      }


      move_straight(enc1, enc2, 36);//24
      //turn_left90(enc1, enc2);
      enc1.readAndReset();
      enc2.readAndReset();
      base_pid = 300; //450
      Kp = 1; //3
      Kd = 10; //0
      Ki = 0;
      long enc1_value = enc1.read();
      long enc2_value = enc2.read(); //right motor
      while(enc2_value > -230){//3 3/8 inches -240 - try sensor fusion
        error = enc2_value - enc1_value;
        total_error += error;
        int pid_value = Kp*error + Kd*(error-last_error) + Ki*total_error;
        int right_motor = base_pid + pid_value;
        int left_motor = base_pid - pid_value;
        M1_backward(left_motor);
        M2_forward(right_motor);
        enc1_value = enc1.read();
        enc2_value = enc2.read();
        last_error = error;
      }
      M1_stop();
      M2_stop();
      delay(2000);
      //move_straight(enc1,enc2,12);

      enc1.readAndReset();
      enc2.readAndReset();
      base_pid = 250; //450
      Kp = 1; //3
      Kd = 10; //0
      Ki = 0;
      enc1_value = enc1.read();
      enc2_value = enc2.read();
      long dist_travelled = (enc1_value/154.32)*1.26*PI;

      while(dist_travelled < 140){
          //travel forwards
          readADC();
          digitalConvert();
          error = enc1_value + enc2_value;
          total_error += error;
          int pid_value = Kp*error + Kd*(error-last_error) + Ki*total_error;
          int right_motor = base_pid + pid_value;
          int left_motor = base_pid - pid_value;
          M1_forward(left_motor);
          M2_forward(right_motor);
          int all_white = 0;
          for (int i = 0; i < 13; i++) {
            if (lineArray[i] == 0) {
              all_white+=1;
            }
          }
          if(all_white >= 5) { //try to reach end square without appearing to stop
            M1_stop();
            M2_stop();
            delay(1000);
            break;
          }
          enc1_value = enc1.read();
          enc2_value = enc2.read();
          dist_travelled = (enc1_value/154.32)*1.26*PI;
          last_error = error;
      }
      M1_stop();
      M2_stop();
      delay(1000);
      enc1.readAndReset();
      enc2.readAndReset();
      base_pid = 300; //450
      Kp = 1; //3
      Kd = 10; //0
      Ki = 0;
      enc1_value = enc1.read();
      enc2_value = enc2.read(); //right motor
      while(enc2_value > -210){//3 3/8 inches -240 - try sensor fusion
        error = enc2_value - enc1_value;
        total_error += error;
        int pid_value = Kp*error + Kd*(error-last_error) + Ki*total_error;
        int right_motor = base_pid + pid_value;
        int left_motor = base_pid - pid_value;
        M1_backward(left_motor);
        M2_forward(right_motor);
        enc1_value = enc1.read();
        enc2_value = enc2.read();
        last_error = error;
      }
      M1_stop();
      M2_stop();
      delay(2000);
      move_straight(enc1, enc2, 10);
      turn_right90(enc1,enc2);
      move_straight(enc1, enc2, 24);
      //turn_right90(enc1,enc2);
      enc1.readAndReset();
      enc2.readAndReset();
      base_pid = 300; //450
      Kp = 1; //3
      Kd = 10; //0
      Ki = 0;
      enc1_value = enc1.read();
      enc2_value = enc2.read(); //right motor
      while(enc1_value < 220){//3 3/8 inches 240 - try sensor fusion, 280 maybe
        error = enc2_value - enc1_value; //may have to subtract instead of add these
        total_error += error;
        int pid_value = Kp*error + Kd*(error-last_error) + Ki*total_error;
        int right_motor = base_pid - pid_value;
        int left_motor = base_pid + pid_value;

        // Serial.print("left_enc: \t");Serial.print(enc1_value);Serial.print("\t right_enc: \t");Serial.println(enc2_value);
        // Serial.print("Enc2 Degrees: \t"); Serial.print((enc2_value/154.32)); Serial.print("\t");
        // Serial.print("left: \t"); Serial.print(left_motor);Serial.print("right: \t"); Serial.println(right_motor);
        M1_forward(left_motor);
        M2_backward(right_motor);
        enc1_value = enc1.read();
        enc2_value = enc2.read();
        last_error = error;
      }
      M1_stop();
      M2_stop();
      delay(2000);
      move_straight(enc1, enc2, 6);
      delay(2000); */



      //Sound detection
      /*
      while(true){
        white_line_follow(300, 8, 300);
        int turn_white = 0;
        for(int i = 0; i < 10; i++){
          if (lineArray[i] == 0) {
              turn_white+=1;
          }
        }
        if(turn_white >= 5){
          M1_stop();
          M2_stop();
          delay(1000);
          break;
        }
      }
      move_straight(enc1,enc2,3);
      right_turn_till_line(enc1,enc2);
      while(true){
        white_line_follow(300, 8, 300);
        int all_white = 0;
        for (int i = 0; i < 13; i++) {
          if (lineArray[i] == 0) {
            all_white+=1;
          }
        }
        if(all_white == 13) {
          M1_stop();
          M2_stop();
          delay(1000);
          break;
        }
      }
      move_straight(enc1,enc2,3);
      //Turns here are wack, try turning until white detected in middle?
      //turn_right90(enc1,enc2); //wait for sound on right
      right_turn_till_line(enc1,enc2);

     // HERE

         // Audio Wait for Right
    bool audio_right_complete = false;
    message = "leader:audio_right";
    Serial.begin(9600);

    while (!audio_right_complete)
    {
        WiFiClient client;
        if (client.connect(host, port))
        {
            client.println(message);
            String response = client.readStringUntil('|');
            Serial.println(response);
            Serial.println("-------------- ^^ Message Recieved ^^ -----------");
            client.stop();
            Serial.println(message);
            Serial.println("-------------- ^^ Message Sent ^^ -----------");
            std::string recd_message = extractMessage(response.c_str());
            Serial.println("Message extracted: ");
            Serial.println(recd_message.c_str());
            if (recd_message == "side_right_complete")
            {
                message = "leader:moving_sides";
                audio_right_complete = true;
            }
            else if (recd_message == "reset")
            {
                message = "leader:checkin";
                return;
            }
        }
        else
        {
            Serial.println("Connection to central control station failed");
        }
        delay(2000);
    }
    Serial.end();


      //delay(2000);
      // turn_left90(enc1,enc2);
      // turn_left90(enc1,enc2); //wait for sound on left
      left_turn_till_line(enc1,enc2);

       // Audio Wait for Left
    bool audio_left_complete = false;
    message = "leader:audio_left";
    std::string direction = "";
    Serial.begin(9600);

    while (!audio_left_complete)
    {
        WiFiClient client;
        if (client.connect(host, port))
        {
            client.println(message);
            String response = client.readStringUntil('|');
            Serial.println(response);
            Serial.println("-------------- ^^ Message Recieved ^^ -----------");
            client.stop();
            Serial.println(message);
            Serial.println("-------------- ^^ Message Sent ^^ -----------");
            std::string recd_message = extractMessage(response.c_str());
            Serial.println("Message extracted: ");
            Serial.println(recd_message.c_str());
            if (recd_message == "direction_left")
            {
                message = "leader:audio_proceed";
                audio_left_complete = true;
                sound_direction = 1;
            }
            else if (recd_message == "direction_right"){
                message = "leader:audio_proceed";
                audio_left_complete = true;
                sound_direction = 0;
            }
            else if (recd_message == "reset")
            {
                message = "leader:checkin";
                return;
            }
        }
        else
        {
            Serial.println("Connection to central control station failed");
        }
        delay(2000);
    }
    Serial.end();


      if(sound_direction == 0){
        // turn_right90(enc1,enc2);
        // turn_right90(enc1,enc2);
        right_turn_till_line(enc1,enc2);
      }
      while(true){
        white_line_follow(300, 8, 300);
        int turn_white = 0;
        if(sound_direction == 1){
          for(int i = 0; i < 10; i++){
            if (lineArray[i] == 0) {
                turn_white+=1;
            }
          }
          if(turn_white >= 5){
            M1_stop();
            M2_stop();
            delay(1000);
            break;
          }
        }else{
          for(int i = 4; i < 13; i++){
            if (lineArray[i] == 0) {
                turn_white+=1;
            }
          }
          if(turn_white >= 5){
            M1_stop();
            M2_stop();
            delay(1000);
            break;
          }
        }
      }
      if(sound_direction == 0){
        move_straight(enc1,enc2,3);
        left_turn_till_line(enc1,enc2);
      }else{
        move_straight(enc1,enc2,3);
        right_turn_till_line(enc1,enc2);
      }
      while(true){
        white_line_follow(300, 8, 300);
        int turn_white = 0;
        if(sound_direction == 1){
          for(int i = 0; i < 10; i++){
            if (lineArray[i] == 0) {
                turn_white+=1;
            }
          }
          if(turn_white >= 5){
            M1_stop();
            M2_stop();
            delay(1000);
            break;
          }
        }else{
          for(int i = 4; i < 13; i++){
            if (lineArray[i] == 0) {
                turn_white+=1;
            }
          }
          if(turn_white >= 5){
            M1_stop();
            M2_stop();
            delay(1000);
            break;
          }
        }
      }
      if(sound_direction == 0){
        move_straight(enc1,enc2,3);
        left_turn_till_line(enc1,enc2);
      }else{
        move_straight(enc1,enc2,3);
        right_turn_till_line(enc1,enc2);
      }
      while(true){
        white_line_follow(300, 8, 300);
        int turn_white = 0;
        if(sound_direction == 0){
          for(int i = 0; i < 10; i++){
            if (lineArray[i] == 0) {
                turn_white+=1;
            }
          }
          if(turn_white >= 5){
            M1_stop();
            M2_stop();
            delay(1000);
            break;
          }
        }else{
          for(int i = 4; i < 13; i++){
            if (lineArray[i] == 0) {
                turn_white+=1;
            }
          }
          if(turn_white >= 5){
            M1_stop();
            M2_stop();
            delay(1000);
            break;
          }
        }
      }
      if(sound_direction == 1){
        move_straight(enc1,enc2,3);
        left_turn_till_line(enc1,enc2);
      }else{
        move_straight(enc1,enc2,3);
        right_turn_till_line(enc1,enc2);
      }
      while(true){
        white_line_follow(300, 8, 300);
        int all_white = 0;
        for (int i = 0; i < 13; i++) {
          if (lineArray[i] == 0) {
            all_white+=1;
          }
        }
        if(all_white == 13) {
          M1_stop();
          M2_stop();
          delay(1000);
          break;
        }
      }
      move_straight(enc1, enc2, 12);
      turn_left90(enc1, enc2);
      move_straight(enc1, enc2, 12);



    */



      /*

      //Small white line
      while(true){
        white_line_follow(300, 8, 300);
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
          break;
        }
      }*/
    /*
    move_straight(enc1, enc2, 12);
    turn_left90(enc1, enc2);
    move_straight(enc1, enc2, 6);



    //Straight shot to end
    while(true){
      white_line_follow(230, 12, 300);
      int all_black = 0;
      for (int i = 0; i < 13; i++) {
        if (lineArray[i] == 1) {
          all_black+=1;
        }
      }
      if(all_black == 13) {
        M1_stop();
        M2_stop();
        delay(1000);
        break;
      }
    }

    enc1.readAndReset();
    enc2.readAndReset();
    base_pid = 250; //450
    Kp = 1; //3
    Kd = 10; //0
    Ki = 0;
    long enc1_value = enc1.read();
    long enc2_value = enc2.read();
    // long dist_travelled = (enc1_value/154.32)*1.26*PI;

    while(true){
        //travel forwards
        readADC();
        digitalConvert();
        error = enc1_value + enc2_value;
        total_error += error;
        int pid_value = Kp*error + Kd*(error-last_error) + Ki*total_error;
        int right_motor = base_pid + pid_value;
        int left_motor = base_pid - pid_value;
        M1_forward(left_motor);
        M2_forward(right_motor);
        int all_white = 0;
        for (int i = 0; i < 13; i++) {
          if (lineArray[i] == 0) {
            all_white+=1;
          }
        }
        if(all_white == 13) { //try to reach end square without appearing to stop
          M1_stop();
          M2_stop();
          delay(1000);
          move_straight(enc1, enc2, 12);
          delay(1000);
          break;
        }
        enc1_value = enc1.read();
        enc2_value = enc2.read();
        // dist_travelled = (enc1_value/154.32)*1.26*PI;
        last_error = error;
    }
    delay(2000);
    */
    /*
    // For straight continuous line: speed = 350, kp = 2, kd = 100, ki = 0
    // For dotted line: speed = 300, kp = 12, kd = 300, ki = 0
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
  */
    /*
      Encoder enc1(M1_ENC_A, M1_ENC_B);
      Encoder enc2(M2_ENC_A, M2_ENC_B);
      move_straight(enc1, enc2, 18);
      base_pid = 350;
      Kp = 8;
      Kd = 300;
      while (true){
        //white_line_follow(350, 8, 300);
        white_line_follow(300, 12, 400);
        int all_white = 0;
        for(int i = 0; i < 10; i++){
          if (lineArray[i] == 0) {
              all_white+=1;
          }
        }
        if(all_white > 6){
          M1_stop();
          M2_stop();
          delay(2000);
          break;
        }
      }*/
    // for (int i = 0; i < 13; i++) {
    //   		if (lineArray[i] == 0) {
    //     		 all_white+=1;
    //   		}
    // 	}
    // if(all_white == 13) {
    //    M1_stop();
    //    M2_stop();
    //    delay(2000);
    // }

    // delay(100);
}