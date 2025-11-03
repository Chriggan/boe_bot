#include <Servo.h>

Servo motor1; // right
Servo motor2; // left
int servo_pin_right_wheel = 10;
int servo_pin_left_wheel = 11;
float prev_time = 0;

void setup()
{
    delay(1000);
    motor1.attach(servo_pin_right_wheel);
    motor2.attach(servo_pin_left_wheel);
    Serial.begin(9600);
}

void loop()
{
    float seconds_passed = micros() / 1000000.0;
    float delta_time = seconds_passed - prev_time;
    prev_time = seconds_passed;
    if (delta_time <= 0)
    {
        return;
    }
    if (seconds_passed < 3.0)
    {
        Serial.println("First");
        calculate_and_send_signals(0.15, 0.15, delta_time);
    }

    else if (seconds_passed < 6.0)
    {
        calculate_and_send_signals(0.05, 0.1, delta_time);
    }

    else if (seconds_passed < 12.0)
    {
        calculate_and_send_signals(0.1, 0.05, delta_time);
    }

    else if (seconds_passed < 15.0)
    {
        calculate_and_send_signals(0.05, 0.1, delta_time);
    }

    else if (seconds_passed < 18.0)
    {
        calculate_and_send_signals(0.05, 0.05, delta_time);
    }

    else if (seconds_passed < 20.0)
    {
        calculate_and_send_signals(0, 0, delta_time);
    }

    else
    {
        return;
    }
}

int right_speed_to_arduino_units(double meter_per_second)
{
    in t r_val = -697.22 * meter_per_second + 1498.2;
    return r_val;
}

int left_speed_to_arduino_units(double meter_per_second)
{
    int l_val = 690.7 * meter_per_second + 1501.8;
    return l_val;
}

double prev_left_velocity = 0;
double prev_right_velocity = 0;
double a_max = 0.1;

void calculate_and_send_signals(double left_speed_target, double right_speed_target, double delta_time)
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

    motor1.writeMicroseconds(r_microseconds);
    motor2.writeMicroseconds(l_microseconds);
}
