#include <math.h>
#include <Servo.h>

Servo motor1; // right
Servo motor2; // left

int servo_pin_right_wheel = 10;
int servo_pin_left_wheel = 11;

float prev_time = 0;

bool hit_l = false;
bool hit_r = false;

double a_max = 0.2;
double a_max_emergency = 1.0;

unsigned long task_start_time = 0;
unsigned long task_duration = 0;
bool task_active = false;

float x_const = 0.3;
float d_value = 0;
float d_minus = 0;

float k = -4.0;

double s_wished = 0.10;

enum AimPhase
{
    AIM_REVERSE,
    AIM_TURN,
    AIM_STRAIGHTEN
};

AimPhase aimPhase = AIM_REVERSE;

enum Mode
{
    DRIVE,
    STOP,
    REVERSE,
    AIM_LEFT,
    AIM_RIGHT
};

Mode mode = DRIVE;

unsigned long pingTime()
{
    pinMode(13, OUTPUT);
    digitalWrite(13, LOW);
    delayMicroseconds(2);
    digitalWrite(13, HIGH);
    delayMicroseconds(5);
    digitalWrite(13, LOW);
    pinMode(13, INPUT);
    unsigned long duration = pulseIn(13, HIGH);
    return duration;
}

float distance_to_wall()
{
    unsigned long ping_time = pingTime();
    float s = 343 / 2 * ping_time * pow(10, -6);
    return s;
}

float filter(float value, float new_value, float x)
{
    return (1.0 - x) * value + x * new_value;
}

void setup()
{
    motor1.attach(servo_pin_right_wheel);
    motor2.attach(servo_pin_left_wheel);
    Serial.begin(9600);
    pinMode(7, INPUT);
    pinMode(5, INPUT);
}

float calculate_velocity(float error_distance)
{
    float v_wished = k * error_distance;
    return v_wished;
}

void loop()
{
    // digitalRead = 1 om ingen hit och digitalRead = 0 om hit
    // Skriver variabeln så att hit_l,r = 1 om hit
    // Skriver variabeln så att hit_l,r = 0 om safe
    // 1 borde evaluera som truthy
    // 0 borde evaluera som falsy

    float distance = distance_to_wall();
    float d_plus = filter(d_minus, distance, 0.3);

    float error_distance = s_wished - d_plus;

    float v_wished = constrain(calculate_velocity(error_distance), -0.15, 0.15);
    // float v_wished = calculate_velocity(error_distance);

    // Serial.println("distance  " + String(distance));
    Serial.println("error     " + String(error_distance));
    // Serial.println("minus     " + String(d_minus));

    hit_l = 1 - digitalRead(5);
    hit_r = 1 - digitalRead(7);

    float seconds_passed = micros() / 1000000.0;
    float delta_time = seconds_passed - prev_time;
    prev_time = seconds_passed;
    if (delta_time <= 0)
    {
        return;
    }

    if (!task_active)
    {
        if (hit_l && hit_r && mode != REVERSE)
        {
            drive(0.0, 0.0, delta_time, a_max_emergency);
            mode = REVERSE;
            start_task_timer(3.0);
        }
        else if (hit_l && mode != AIM_RIGHT)
        {
            drive(0.0, 0.0, delta_time, a_max_emergency);
            mode = AIM_RIGHT;
            start_task_timer(1.5);
        }
        else if (hit_r && mode != AIM_LEFT)
        {
            drive(0.0, 0.0, delta_time, a_max_emergency);
            mode = AIM_LEFT;
            start_task_timer(1.5);
        }
        else
        {
            mode = DRIVE;
        }
    }

    switch (mode)
    {
    case DRIVE:
        drive(v_wished, v_wished, delta_time, a_max);
        break;

    case REVERSE:
        drive(-0.15, -0.15, delta_time, a_max);
        break;

    case AIM_LEFT:
        // svänger först vänster och rätar sedan ut
        // slutposition till vänster om vart man började

        switch (aimPhase)
        {
        case AIM_REVERSE:
            drive(-0.15, -0.15, delta_time, a_max);
            if (task_timer_done())
            {
                aimPhase = AIM_TURN;
                start_task_timer(2.5);
                Serial.println("hej");
            }
            break;

        case AIM_TURN:
            drive(0.1, 0.01, delta_time, a_max);
            if (task_timer_done())
            {
                aimPhase = AIM_STRAIGHTEN;
                start_task_timer(1.3);
            }
            break;

        case AIM_STRAIGHTEN:
            drive(0.01, 0.1, delta_time, a_max);
            if (task_timer_done())
            {
                aimPhase = AIM_REVERSE;
                mode = DRIVE;
            }
            break;
        }
        Serial.println("siktar vänster, step: " + String(aimPhase));
        break;

    case AIM_RIGHT:
        // svänger först höger och rätar sedan ut
        // slutposition till vänster om vart man började

        switch (aimPhase)
        {
        case AIM_REVERSE:
            drive(-0.15, -0.15, delta_time, a_max);
            if (task_timer_done())
            {
                aimPhase = AIM_TURN;
                start_task_timer(2.3);
                Serial.println("hej");
            }
            break;

        case AIM_TURN:
            drive(0.01, 0.1, delta_time, a_max);
            if (task_timer_done())
            {
                aimPhase = AIM_STRAIGHTEN;
                start_task_timer(1.5); // 1.7s?
            }
            break;

        case AIM_STRAIGHTEN:
            drive(0.1, 0.01, delta_time, a_max);
            if (task_timer_done())
            {
                aimPhase = AIM_REVERSE;
                mode = DRIVE;
            }
            break;
        }
        Serial.println("siktar höger, step: " + String(aimPhase));
        break;

    default:
        // STOP
        drive(0.0, 0.0, delta_time, a_max_emergency);
    }
    d_minus = d_plus;
}

void start_task_timer(float seconds)
{
    task_start_time = micros();
    task_duration = seconds * 1000000.0;
    task_active = true;
}

bool task_timer_done()
{
    if (!task_active)
        return false;
    if (micros() - task_start_time >= task_duration)
    {
        task_active = false;
        return true;
    }
    return false;
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
