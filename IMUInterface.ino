#include <I2Cdev.h>
#include "MPU6050_6Axis_MotionApps20.h"
#include "Wire.h"

MPU6050 mpu;
// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================

volatile bool mpuInterrupt = false;     // indicates whether MPU interrupt pin has gone high
void dmpDataReady() {
	mpuInterrupt = true;
}

void setup(){
	initMPU();


}

void loop()
{
	calculateYPR();
	//display ypr
	Serial.println("");
	Serial.print("ypr\t");
	for (int i = 0; i < 3; i++) {
		Serial.print(ypr[i] * 180 / M_PI);
		if (i != 2) {
			Serial.print("\t");
		}
	}

}

void initMPU() {
	Wire.begin();
	//Initilise mpu and then verify
	Serial.begin(9600);
	Serial.println(F("Initializing I2C devices..."));
	mpu.initialize();
	Serial.println(F("Testing device connections..."));
	Serial.println(mpu.testConnection() ? F("MPU6050 connection successful") : F("MPU6050 connection failed"));

	//Initilizing dmp then turning on dmp
	devStatus = mpu.dmpInitialize();
	if (devStatus == 0) {
		Serial.println("Enabling DMP...");
		mpu.setDMPEnabled(true);
		attachInterrupt(0, dmpDataReady, RISING);
		mpuIntStatus = mpu.getIntStatus();
		dmpReady = true;
		packetSize = mpu.dmpGetFIFOPacketSize();
		Serial.println("Enabling DMP Done");
	}
	

}

void calculateYPR() {
	mpuInterrupt = false;
	mpuIntStatus = mpu.getIntStatus();
	fifoCount = mpu.getFIFOCount();

	//checking if FIFO buffer is overflowed on register 0x3A bit 4 FIFO_OFLOW_INT
	if ((mpuIntStatus & 0x10) || fifoCount >= 1024) {
		mpu.resetFIFO();
	}
	//check if DMP interrupt is set on register 0x3A bit 1 DMP_INT
	else if (mpuIntStatus & 0x02) {
		//wait for correct abailable data length
		while (fifoCount < packetSize) {
			fifoCount = mpu.getFIFOCount();
		}

		//read a packet from FIFO buffer
		mpu.getFIFOBytes(fifoBuffer, packetSize);

		// track FIFO count here in case there is > 1 packet available
		// (this lets us immediately read more without waiting for an interrupt)
		fifoCount -= packetSize;

		//calcualting YPR and storing into ypr
		mpu.dmpGetQuaternion(&q, fifoBuffer);
		mpu.dmpGetGravity(&gravity, &q);
		mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

	}
}