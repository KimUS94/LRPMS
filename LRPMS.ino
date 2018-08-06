#include <math.h>
#include <Wire.h>
//MPU-6050은 각 축에 대해 -16383 ~ +16383 까지의 값을 출력
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
SoftwareSerial GPSserial(3, 2);
Adafruit_GPS GPS(&GPSserial);
SoftwareSerial BTSerial(5, 4);

#define GPSECHO false
boolean usingInterrupt = false;

#define MPU6050_ACCEL_XOUT_H 0x3B // R
#define MPU6050_PWR_MGMT_1 0x6B // R/W
#define MPU6050_PWR_MGMT_2 0x6C // R/W
#define MPU6050_WHO_AM_I 0x75 // R
#define MPU6050_I2C_ADDRESS 0x68

/* Kalman filter */
struct GyroKalman {
  float x_angle, x_bias;
  float P_00, P_01, P_10, P_11;
  float Q_angle, Q_gyro;
  float R_angle;
};

struct GyroKalman angX;
struct GyroKalman angY;
struct GyroKalman angZ;

static const float R_angle = 0.3;     //.3 default
static const float Q_angle = 0.01;  //0.01 (Kalman)
static const float Q_gyro = 0.04; //0.04 (Kalman)

const int lowX = -2150;
const int highX = 2210;
const int lowY = -2150;
const int highY = 2210;
const int lowZ = -2150;
const int highZ = 2550;

/* time */
unsigned long prevSensoredTime = 0;
unsigned long curSensoredTime = 0;

typedef union accel_t_gyro_union {
  struct
  {
    uint8_t x_accel_h;
    uint8_t x_accel_l;
    uint8_t y_accel_h;
    uint8_t y_accel_l;
    uint8_t z_accel_h;
    uint8_t z_accel_l;
    uint8_t t_h;
    uint8_t t_l;
    uint8_t x_gyro_h;
    uint8_t x_gyro_l;
    uint8_t y_gyro_h;
    uint8_t y_gyro_l;
    uint8_t z_gyro_h;
    uint8_t z_gyro_l;
  } reg;

  struct
  {
    int x_accel;
    int y_accel;
    int z_accel;
    int temperature;
    int x_gyro;
    int y_gyro;
    int z_gyro;
  } value;
};

int xInit[5] = {0, 0, 0, 0, 0};
int yInit[5] = {0, 0, 0, 0, 0};
int zInit[5] = {0, 0, 0, 0, 0};
int initIndex = 0;
int initSize = 5;
int xCal = 0;
int yCal = 0;
int zCal = 1800;

void setup() {
  int error;
  uint8_t c;

  initGyroKalman(&angX, Q_angle, Q_gyro, R_angle);
  initGyroKalman(&angY, Q_angle, Q_gyro, R_angle);
  initGyroKalman(&angZ, Q_angle, Q_gyro, R_angle);

  //Serial.begin(9600);
  BTSerial.begin(9600);
  //Serial.println("Adafruit GPS library basic test!");

  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 1 Hz update rate

  Wire.begin();

  error = MPU6050_read (MPU6050_WHO_AM_I, &c, 1);
  error = MPU6050_read (MPU6050_PWR_MGMT_2, &c, 1);
  MPU6050_write_reg (MPU6050_PWR_MGMT_1, 0);
}
uint32_t timer = millis();

void loop() {
  int error;
  double dT;
  accel_t_gyro_union accel_t_gyro;
  curSensoredTime = millis();
  error = MPU6050_read (MPU6050_ACCEL_XOUT_H, (uint8_t *) &accel_t_gyro, sizeof(accel_t_gyro));

  uint8_t swap;
#define SWAP(x,y) swap = x; x = y; y = swap
  SWAP (accel_t_gyro.reg.x_accel_h, accel_t_gyro.reg.x_accel_l);
  SWAP (accel_t_gyro.reg.y_accel_h, accel_t_gyro.reg.y_accel_l);
  SWAP (accel_t_gyro.reg.z_accel_h, accel_t_gyro.reg.z_accel_l);
  SWAP (accel_t_gyro.reg.t_h, accel_t_gyro.reg.t_l);
  SWAP (accel_t_gyro.reg.x_gyro_h, accel_t_gyro.reg.x_gyro_l);
  SWAP (accel_t_gyro.reg.y_gyro_h, accel_t_gyro.reg.y_gyro_l);
  SWAP (accel_t_gyro.reg.z_gyro_h, accel_t_gyro.reg.z_gyro_l);

  if (prevSensoredTime > 0) {
    int gx1 = 0, gy1 = 0, gz1 = 0;
    float gx2 = 0, gy2 = 0, gz2 = 0;
    int loopTime = curSensoredTime - prevSensoredTime;

    gx2 = angleInDegrees(lowX, highX, accel_t_gyro.value.x_gyro);
    gy2 = angleInDegrees(lowY, highY, accel_t_gyro.value.y_gyro);
    gz2 = angleInDegrees(lowZ, highZ, accel_t_gyro.value.z_gyro);

    predict(&angX, gx2, loopTime);
    predict(&angY, gy2, loopTime);
    predict(&angZ, gz2, loopTime);

    gx1 = update(&angX, accel_t_gyro.value.x_accel) / 10;
    gy1 = update(&angY, accel_t_gyro.value.y_accel) / 10;
    gz1 = update(&angZ, accel_t_gyro.value.z_accel) / 10;

    if (initIndex < initSize)
    {
      xInit[initIndex] = gx1;
      yInit[initIndex] = gy1;
      zInit[initIndex] = gz1;
      if (initIndex == initSize - 1)
      {
        int sumX = 0; int sumY = 0; int sumZ = 0;
        for (int k = 1; k <= initSize; k++)
        {
          sumX += xInit[k];
          sumY += yInit[k];
          sumZ += zInit[k];
        }

        xCal -= sumX / (initSize - 1);
        yCal -= sumY / (initSize - 1);
        zCal = (sumZ / (initSize - 1) - zCal);
      }
      initIndex++;
    }
    else
    {
      gx1 += xCal;
      gy1 += yCal;
    }



    BTSerial.listen();
    BTSerial.print("m"); BTSerial.print(9); //BTSerial.print('/');
    BTSerial.print("x"); BTSerial.print(gx1, DEC); //BTSerial.print('/');
    BTSerial.print("y"); BTSerial.print(gy1, DEC); //BTSerial.print('/');
    BTSerial.print("z"); BTSerial.print(gz1, DEC); //BTSerial.print('/');
  }
  unsigned char c;
  Wire.requestFrom(0xA0 >> 1, 1);    // 슬레이브 장치가 1바이트 요청
  c = Wire.read();   // 읽은 값(심박수) c에 저장(byte)
  BTSerial.print("h"); BTSerial.print(c, DEC);   //BTSerial.print('/');

  GPSserial.listen();
  char gps = GPS.read();
  if (GPS.newNMEAreceived())
  {
    GPS.lastNMEA();
    if (!GPS.parse(GPS.lastNMEA()))
      return;
  }
  if (timer > millis())  timer = millis();
  if (millis() - timer > 1000)
  {
    BTSerial.print("t");  BTSerial.print(GPS.hour, DEC); BTSerial.print(':'); BTSerial.print(GPS.minute, DEC); BTSerial.print(':'); BTSerial.print(GPS.seconds, DEC); //BTSerial.print('/');
    BTSerial.print("d");  BTSerial.print("20"); BTSerial.print(GPS.year, DEC); BTSerial.print(','); BTSerial.print(GPS.month, DEC); BTSerial.print(','); BTSerial.print(GPS.day, DEC); //BTSerial.print('/');
    if (GPS.fix)
    {
      BTSerial.print("a");
      BTSerial.print(GPS.latitude, 4);
      BTSerial.print(GPS.lat);
      BTSerial.print("o");
      BTSerial.print(GPS.longitude, 4);
      BTSerial.print(GPS.lon);
    }
    else
    {
      BTSerial.print("a");
      BTSerial.print(000.); 
      BTSerial.print(0000); 
      BTSerial.print("o");
      BTSerial.print(000.);
      BTSerial.print(0000); 
    }
  }
  prevSensoredTime = curSensoredTime;
  delay(2000);
}

int MPU6050_read(int start, uint8_t *buffer, int size)
{
  int i, n, error;
  Wire.beginTransmission(MPU6050_I2C_ADDRESS);
  n = Wire.write(start);

  if (n != 1)
    return (-10);

  n = Wire.endTransmission(false);
  if (n != 0)
    return (n);

  Wire.requestFrom(MPU6050_I2C_ADDRESS, size, true);
  i = 0;
  while (Wire.available() && i < size)
  {
    buffer[i++] = Wire.read();
  }
  if ( i != size)
    return (-11);
  return (0);
}

int MPU6050_write(int start, const uint8_t *pData, int size)
{
  int n, error;

  Wire.beginTransmission(MPU6050_I2C_ADDRESS);

  n = Wire.write(start);
  if (n != 1)
    return (-20);

  n = Wire.write(pData, size);
  if (n != size)
    return (-21);

  error = Wire.endTransmission(true);
  if (error != 0)
    return (error);
  return (0);
}

int MPU6050_write_reg(int reg, uint8_t data)
{
  int error;
  error = MPU6050_write(reg, &data, 1);
  return (error);
}


float angleInDegrees(int lo, int hi, int measured)
{
  float x = (hi - lo) / 180.0;
  return (float)measured / x;
}

void initGyroKalman(struct GyroKalman * kalman, const float Q_angle, const float Q_gyro, const float R_angle)
{
  kalman->Q_angle = Q_angle;
  kalman->Q_gyro = Q_gyro;
  kalman->R_angle = R_angle;

  kalman->P_00 = 0;
  kalman->P_01 = 0;
  kalman->P_10 = 0;
  kalman->P_11 = 0;
}

void predict(struct GyroKalman * kalman, float dotAngle, float dt)
{
  kalman->x_angle += dt * (dotAngle - kalman->x_bias);
  kalman->P_00 += -1 * dt * (kalman->P_10 + kalman->P_01) + dt * dt * kalman->P_11 + kalman->Q_angle;
  kalman->P_01 += -1 * dt * kalman->P_11;
  kalman->P_10 += -1 * dt * kalman->P_11;
  kalman->P_11 += kalman->Q_gyro;
}

float update(struct GyroKalman * kalman, float angle_m) {
  const float y = angle_m - kalman->x_angle;
  const float S = kalman->P_00 + kalman->R_angle;
  const float K_0 = kalman->P_00 / S;
  const float K_1 = kalman->P_10 / S;
  kalman->x_angle += K_0 * y;
  kalman->x_bias += K_1 * y;
  kalman->P_00 -= K_0 * kalman->P_00;
  kalman->P_01 -= K_0 * kalman->P_01;
  kalman->P_10 -= K_1 * kalman->P_00;
  kalman->P_11 -= K_1 * kalman->P_01;
  return kalman->x_angle;
}
