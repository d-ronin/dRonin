# IMU Orientation Convention

dRonin defines vehicle axes as X forward, Y right, Z down. Bottom sensor orientation is +180 degrees about vehicle X-axis, and is applied first. Sensor rotations are about the vehicle Z-axis, e.g. top 90 degrees is rotated 90 degrees clockwise looking top-down.

## Invensense IMUs
All known Invensense IMUs share the same gyro/accelerometer convention: X right, Y forward, Z up. The AKxxxx magnetometer inside MPU9xx0 matches dRonin vehicle convention (X forward, Y right, Z down).

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
### PIOS_MPU_TOP_0DEG
![Invensense top 0 degrees](images/mpu_top_0deg.png)

### PIOS_MPU_TOP_90DEG
![Invensense top 90 degrees](images/mpu_top_90deg.png)

### PIOS_MPU_TOP_180DEG
![Invensense top 180 degrees](images/mpu_top_180deg.png)

### PIOS_MPU_TOP_270DEG
![Invensense top 270 degrees](images/mpu_top_270deg.png)

### PIOS_MPU_BOTTOM_0DEG
![Invensense bottom 0 degrees](images/mpu_bottom_0deg.png)

### PIOS_MPU_BOTTOM_90DEG
![Invensense bottom 90 degrees](images/mpu_bottom_90deg.png)

### PIOS_MPU_BOTTOM_180DEG
![Invensense bottom 180 degrees](images/mpu_bottom_180deg.png)

### PIOS_MPU_BOTTOM_270DEG
![Invensense bottom 270 degrees](images/mpu_bottom_270deg.png)
