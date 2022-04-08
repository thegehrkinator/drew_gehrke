#include <Wire.h> //Needed for I2C
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include <MicroNMEA.h>

#include <Adafruit_BNO08x.h>
#include <Adafruit_I2CDevice.h>


#define I2C_ADDR_IMU 0x4A
#define I2C_ADDR_GPS 0x42
#define BNO08X_RESET -1

Adafruit_BNO08x bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;

SFE_UBLOX_GNSS myGPS;
char nmeaBuffer[100];
MicroNMEA nmea(nmeaBuffer, sizeof(nmeaBuffer));
boolean imu_conn;
boolean gps_conn;

//Enable the Accelerometer and calibrated Gyroscope on the BNO085
void setReports() {
  if (!bno08x.enableReport(SH2_ACCELEROMETER)) {
    Serial.println("Could not enable accelerometer");
  }
  delay(10);
  if (!bno08x.enableReport(SH2_GYROSCOPE_CALIBRATED)) {
    Serial.println("Could not enable accelerometer");
  }
  delay(10);
}


//Check for new data on the NEO M9N GPS Module. Updates latitude and longitude variables.
void checkGPSData() {
  myGPS.begin();
  
  myGPS.checkUblox(); //See if new data is available. Process bytes as they come in.

  if(nmea.isValid() == true)
  {
    long latitude_mdeg = nmea.getLatitude();
    long longitude_mdeg = nmea.getLongitude();

    Serial.print("Latitude (deg): ");
    Serial.println(latitude_mdeg / 1000000., 6);
    Serial.print("Longitude (deg): ");
    Serial.println(longitude_mdeg / 1000000., 6);
    
    nmea.clear(); // Clear the MicroNMEA storage to make sure we are getting fresh data
  }
  else {
    Serial.println("GPS: Waiting for new data");
  }
}


//Check for new data on the BNO085 IMU module. Updates the Gyrocope and Accelerometer variables.
void checkIMUData() {
  if (!bno08x.enableReport(SH2_ACCELEROMETER)) {
    Serial.println("Could not enable accelerometer");
  }

  if (!bno08x.getSensorEvent(&sensorValue)) {
      return;
  } 
  else {
    //Output the data for the Accelerometer if there was a change in previous data
    switch (sensorValue.sensorId) {

    case SH2_ACCELEROMETER:
      Serial.print("Accelerometer - x: ");
      Serial.print(sensorValue.un.accelerometer.x);
      Serial.print(" y: ");
      Serial.print(sensorValue.un.accelerometer.y);
      Serial.print(" z: ");
      Serial.println(sensorValue.un.accelerometer.z);
      break;
    }
  }

  if (!bno08x.enableReport(SH2_GYROSCOPE_CALIBRATED)) {
    Serial.println("Could not enable accelerometer");
  }

  if (!bno08x.getSensorEvent(&sensorValue)) {
      return;
  }
  else {
    //Output the data for the Gyroscope if there was a change in previous data
    switch (sensorValue.sensorId) {
     case SH2_GYROSCOPE_CALIBRATED:
      Serial.print("Gyro - x: ");
      Serial.print(sensorValue.un.gyroscope.x);
      Serial.print(" y: ");
      Serial.print(sensorValue.un.gyroscope.y);
      Serial.print(" z: ");
      Serial.println(sensorValue.un.gyroscope.z);
      break;
    }
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin();
  if(!myGPS.begin()) {
      Serial.println(F("GPS module not detected"));
      gps_conn = false;
  }
  else {
    myGPS.setI2COutput(COM_TYPE_UBX | COM_TYPE_NMEA); //Set the I2C port to output both NMEA and UBX messages
    myGPS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT); //Save (only) the communications port settings to flash and BBR

    myGPS.setProcessNMEAMask(SFE_UBLOX_FILTER_NMEA_ALL); // Make sure the library is passing all NMEA messages to processNMEA
    gps_conn = true;
  }
  
  if (!bno08x.begin_I2C()) {
      Serial.println("Failed to find BNO085 chip");
      imu_conn = false;
  }
  else {imu_conn = true;}
  pinMode(34, INPUT_PULLUP);
}


void loop() {
  //Check for new data from sensors
  while(1) {
    if (digitalRead(34)) {
      Serial.println("Collision Detected!");
    }
    
    if (gps_conn) {
      checkGPSData();
    }
    
    if (imu_conn) {
      checkIMUData();
    }
    
    delay(500); //Allow the I2C bus to update
  }
}

void SFE_UBLOX_GNSS::processNMEA(char incoming)
{
  //Take the incoming char from the u-blox I2C port and pass it on to the MicroNMEA lib
  //for sentence cracking
  nmea.process(incoming);
}
