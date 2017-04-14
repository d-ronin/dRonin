# IMU Orientation Convention

dRonin defines vehicle axes as X forward, Y right, Z down. Bottom sensor orientation is +180 degrees about vehicle X-axis, and is applied first. Sensor rotations are about the vehicle Z-axis, e.g. top 90 degrees is rotated 90 degrees clockwise looking top-down. Note: top 0 degrees is defined with the pin 1 marker closest to the front left corner of the board.

## Invensense IMUs
All known Invensense IMUs share the same gyro/accelerometer convention (with pin 1 at front-left corner): X right, Y forward, Z up. The AKxxxx magnetometer inside MPU9xx0 matches dRonin vehicle convention (X forward, Y right, Z down).

| Orientation            | Gyro X | Gyro Y | Gyro Z | Mag X | Mag Y | Mag Z |
|------------------------|--------|--------|--------|-------|-------|-------|
| PIOS_MPU_TOP_0DEG      | Y      | X      | -Z     | X     | Y     | Z     |
| PIOS_MPU_TOP_90DEG     | -X     | Y      | -Z     | -Y    | X     | Z     |
| PIOS_MPU_TOP_180DEG    | -Y     | -X     | -Z     | -X    | -Y    | Z     |
| PIOS_MPU_TOP_270DEG    | X      | -Y     | -Z     | Y     | -X    | Z     |
| PIOS_MPU_BOTTOM_0DEG   | Y      | -X     | Z      | X     | -Y    | -Z    |
| PIOS_MPU_BOTTOM_90DEG  | X      | Y      | Z      | Y     | X     | -Z    |
| PIOS_MPU_BOTTOM_180DEG | -Y     | X      | Z      | -X    | Y     | -Z    |
| PIOS_MPU_BOTTOM_270DEG | -X     | -Y     | Z      | -Y    | -X    | -Z    |

In the following diagrams, the large axes are dRonin vehicle axes, and the small axes Invensense gyro/accelerometer axes. X is red, Y is green, and Z is blue.

| Orientation            | Gyro/Accel | Magnetometer |
|------------------------|------------|--------------|
| PIOS_MPU_TOP_0DEG      | ![Invensense gyro top 0 degrees](images/mpu_top_0deg.png) | ![Invensense mag top 0 degrees](images/mpu_mag_top_0deg.png) |
| PIOS_MPU_TOP_90DEG     | ![Invensense gyro top 90 degrees](images/mpu_top_90deg.png) | ![Invensense mag top 90 degrees](images/mpu_mag_top_90deg.png) |
| PIOS_MPU_TOP_180DEG    | ![Invensense gyro top 180 degrees](images/mpu_top_180deg.png) | ![Invensense mag top 180 degrees](images/mpu_mag_top_180deg.png) |
| PIOS_MPU_TOP_270DEG    | ![Invensense gyro top 270 degrees](images/mpu_top_270deg.png) | ![Invensense mag top 270 degrees](images/mpu_mag_top_270deg.png) |
| PIOS_MPU_BOTTOM_0DEG   | ![Invensense gyro bottom 0 degrees](images/mpu_bottom_0deg.png) | ![Invensense mag bottom 0 degrees](images/mpu_mag_bottom_0deg.png) |
| PIOS_MPU_BOTTOM_90DEG  | ![Invensense gyro bottom 90 degrees](images/mpu_bottom_90deg.png) | ![Invensense mag bottom 90 degrees](images/mpu_mag_bottom_90deg.png) |
| PIOS_MPU_BOTTOM_180DEG | ![Invensense gyro bottom 180 degrees](images/mpu_bottom_180deg.png) | ![Invensense mag bottom 180 degrees](images/mpu_mag_bottom_180deg.png) |
| PIOS_MPU_BOTTOM_270DEG | ![Invensense gyro bottom 270 degrees](images/mpu_bottom_270deg.png) | ![Invensense mag bottom 270 degrees](images/mpu_mag_bottom_270deg.png) |

## Bosch BMI160
BMI160 gyro/accelerometer convention (with pin 1 at front-left corner): X forward, Y left, Z up.

| Orientation               | Gyro X | Gyro Y | Gyro Z |
|---------------------------|--------|--------|--------|
| PIOS_BMI160_TOP_0DEG      | X      | -Y     | -Z     |
| PIOS_BMI160_TOP_90DEG     | Y      | X      | -Z     |
| PIOS_BMI160_TOP_180DEG    | -X     | Y      | -Z     |
| PIOS_BMI160_TOP_270DEG    | -Y     | -X     | -Z     |
| PIOS_BMI160_BOTTOM_0DEG   | X      | Y      | Z      |
| PIOS_BMI160_BOTTOM_90DEG  | -Y     | X      | Z      |
| PIOS_BMI160_BOTTOM_180DEG | -X     | -Y     | Z      |
| PIOS_BMI160_BOTTOM_270DEG | Y      | -X     | Z      |

In the following diagrams, the large axes are dRonin vehicle axes, and the small axes BMI160 gyro/accelerometer axes. X is red, Y is green, and Z is blue.

| Orientation               | Gyro/Accel                                                         |
|---------------------------|--------------------------------------------------------------------|
| PIOS_BMI160_TOP_0DEG      | ![BMI160 gyro top 0 degrees](images/bmi160_top_0deg.png)           |
| PIOS_BMI160_TOP_90DEG     | ![BMI160 gyro top 90 degrees](images/bmi160_top_90deg.png)         |
| PIOS_BMI160_TOP_180DEG    | ![BMI160 gyro top 180 degrees](images/bmi160_top_180deg.png)       |
| PIOS_BMI160_TOP_270DEG    | ![BMI160 gyro top 270 degrees](images/bmi160_top_270deg.png)       |
| PIOS_BMI160_BOTTOM_0DEG   | ![BMI160 gyro bottom 0 degrees](images/bmi160_bottom_0deg.png)     |
| PIOS_BMI160_BOTTOM_90DEG  | ![BMI160 gyro bottom 90 degrees](images/bmi160_bottom_90deg.png)   |
| PIOS_BMI160_BOTTOM_180DEG | ![BMI160 gyro bottom 180 degrees](images/bmi160_bottom_180deg.png) |
| PIOS_BMI160_BOTTOM_270DEG | ![BMI160 gyro bottom 270 degrees](images/bmi160_bottom_270deg.png) |

## ST LIS3MDL
LIS3MDL magnetometer convention (with pin 1 at front-left corner): X left, Y rear, Z up.

| Orientation            | Mag X  | Mag Y  | Mag Z  |
|------------------------|--------|--------|--------|
| PIOS_LIS_TOP_0DEG      | -Y     | -X     | -Z     |
| PIOS_LIS_TOP_90DEG     | -Y     | X      | -Z     |
| PIOS_LIS_TOP_180DEG    | X      | Y      | -Z     |
| PIOS_LIS_TOP_270DEG    | Y      | -X     | -Z     |
| PIOS_LIS_BOTTOM_0DEG   | -Y     | X      | Z      |
| PIOS_LIS_BOTTOM_90DEG  | -X     | -Y     | Z      |
| PIOS_LIS_BOTTOM_180DEG | Y      | -X     | Z      |
| PIOS_LIS_BOTTOM_270DEG | X      | Y      | Z      |
