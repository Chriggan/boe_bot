#include <Servo.h>

Servo motor1; // right
Servo motor2; // left
int servo_pin_right_wheel = 10;
int servo_pin_left_wheel = 11;
float prev_time = 0;
bool crash = false;
int reverse_until = 0;
int reverse_direction;
int angle = 0;

void setup()
{
    motor1.attach(servo_pin_right_wheel);
    motor2.attach(servo_pin_left_wheel);
    Serial.begin(9600);
    pinMode(7, INPUT);
    pinMode(5, INPUT);
}

double a_max = 0.2;
double a_max_emergency = 1.0;

void loop()
{

    int input_l = digitalRead(5);
    int input_r = digitalRead(7);

    float seconds_passed = micros() / 1000000.0;
    float delta_time = seconds_passed - prev_time;

    prev_time = seconds_passed;
    if (delta_time <= 0)
    {
        return;
    }

    int drive_collision = check_collision();

    // Serial.println(drive_collision);

    if (crash == false)
    {

        drive(0.15, 0.15, delta_time, a_max);
        if (drive_collision == 0 or drive_collision == 2)
        {
            reverse_direction = check_collision();
            drive(0, 0, delta_time, a_max_emergency);
            crash = true;
            if (reverse_direction == 2 and abs(angle) != 90)
            {
                reverse_until = prev_time + 3.5;
                angle -= 45;
            }
            else if (reverse_direction == 0 and abs(angle) != 90)
            {
                reverse_until = prev_time + 3.5;
                angle += 45;
            }
            else if (angle == -90)
            {
                reverse_until = prev_time + 7.0;
                angle += 90;
            }
            else if (angle == 90)
            {
                reverse_until = prev_time + 7.0;
                angle -= 90;
            }
        }
        else if (drive_collision == 1)
        {
            crash = false;
        }
    }
    else if (crash == true)
    {
        // Bestämmer reverse_direction momentant

        // Serial.println("reverse direction.    " + String(reverse_direction));

        Serial.println("   ---   " + String(angle) + "   ---   ");
        if (reverse_direction == 2)
        { // krock höger, snurrar vänster hjul snabbare
            if (angle == -90)
            {
                reverse(0.1, 0.01, delta_time, a_max);
            }
            else
            {
                reverse(0.01, 0.1, delta_time, a_max);
            }
        }
        else
        { // vänster eller båda
            if (angle == 90)
            {
                reverse(0.1, 0.01, delta_time, a_max);
            }
            else
            {
                reverse(0.01, 0.1, delta_time, a_max);
            }
        }
        if (seconds_passed > reverse_until)
        {

            drive(0, 0, delta_time, a_max_emergency);

            Serial.println(reverse_direction);
            reverse_direction = 1;
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

void drive(double left_speed_target, double right_speed_target, double delta_time, float acceleration)
{

    double left_acc_target = (left_speed_target - prev_left_velocity) / delta_time;
    double left_a_allowed = constrain(left_acc_target, -acceleration, acceleration);
    double left_speed_allowed = prev_left_velocity + left_a_allowed * delta_time;
    prev_left_velocity = left_speed_allowed; // in m/s

    double right_acc_target = (right_speed_target - prev_right_velocity) / delta_time;
    double right_a_allowed = constrain(right_acc_target, -acceleration, acceleration);
    double right_speed_allowed = prev_right_velocity + right_a_allowed * delta_time;
    prev_right_velocity = right_speed_allowed;

    int r_microseconds = right_speed_to_arduino_units(right_speed_allowed);
    int l_microseconds = left_speed_to_arduino_units(left_speed_allowed);

    motor1.writeMicroseconds(r_microseconds);
    motor2.writeMicroseconds(l_microseconds);
}

void reverse(double left_speed_target, double right_speed_target, float delta_time, float acceleration)
{
    drive(-left_speed_target, -right_speed_target, delta_time, acceleration);
}

int check_collision()
{
    int input_l = digitalRead(5);
    int input_r = digitalRead(7);

    // Serial.println(String(input_l) + "   -   " + String(input_r));

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
        return 2; // krock höger
    }
}
