#include <Servo.h>

Servo motor1; // right
Servo motor2; // left
int servo_pin_right_wheel = 10;
int servo_pin_left_wheel = 11;
float prev_time = 0;
bool crash = false;

void setup()
{
    delay(1000);
    motor1.attach(servo_pin_right_wheel);
    motor2.attach(servo_pin_left_wheel);
    Serial.begin(9600);
    pinMode(7, INPUT);
    pinMode(5, INPUT);
}

void loop()
{
    Serial.println(crash);

    float seconds_passed = micros() / 1000000.0;
    float delta_time = seconds_passed - prev_time;

    prev_time = seconds_passed;
    if (delta_time <= 0)
    {
        return;
    }

    if (crash == false)
    {
        drive(0.15, 0.15, delta_time);
    }
    else if (crash == true)
    {
        int time_done_reverse = seconds_passed + 1.5 * 1000000;
        if (seconds_passed < time_done_reverse)
        {
            reverse(0.05, 0.05, delta_time);
            crash = false;
        }
        else
        {
            Serial.println("niklas");
            crash = false;
        }
    }
}

int right_speed_to_arduino_units(double meter_per_second)
{
    int r_val = -697.22 * -meter_per_second + 1498.2;
    return r_val;
}

int left_speed_to_arduino_units(double meter_per_second)
{
    int l_val = 690.7 * -meter_per_second + 1501.8;
    return l_val;
}

double prev_left_velocity = 0;
double prev_right_velocity = 0;
double a_max = 0.2;
double a_max_emergency = 1.0;

void drive(double left_speed_target, double right_speed_target, double delta_time)
{

    double left_acc_target = (left_speed_target - prev_left_velocity) / delta_time;
    double left_a_allowed = constrain(left_acc_target, -a_max, a_max);
    double left_speed_allowed = prev_left_velocity + left_a_allowed * delta_time;
    prev_left_velocity = left_speed_allowed; // in m/s

    double right_acc_target = (right_speed_target - prev_right_velocity) / delta_time;
    double right_a_allowed = constrain(right_acc_target, -a_max, a_max);
    double right_speed_allowed = prev_right_velocity + right_a_allowed * delta_time;
    prev_right_velocity = right_speed_allowed;

    int r_microseconds = right_speed_to_arduino_units(right_speed_allowed);
    int l_microseconds = left_speed_to_arduino_units(left_speed_allowed);

    int collision = check_collision();

    if (collision == 0)
    {
        stop_drive();
        crash = true;
    }
    else
    {
        motor1.writeMicroseconds(r_microseconds);
        motor2.writeMicroseconds(l_microseconds);
    }
}

void reverse(double left_speed_target, double right_speed_target, float delta_time)
{
    drive(-left_speed_target, -right_speed_target, delta_time);
}

void stop_drive()
{
    motor1.writeMicroseconds(1498.2);
    motor2.writeMicroseconds(1501.8);
}

int check_collision()
{
    int input_l = digitalRead(5);
    int input_r = digitalRead(7);

    int sensor_values[] = {input_l, input_r};
    int temp[] = {9, 5};

    if (input_l == 1 and input_r == 1)
    {
        return 1; // allt är lugnt
    }
    else if (input_l == 0 and input_r == 0)
    {
        return 0; // krock båda
    }
    else if (input_l == 0)
    {
        return 0; // krock vänster
    }
    else if (input_r == 0)
    {

        return 0; // krock höger
    }
}
