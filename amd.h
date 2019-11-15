#pragma once

enum ControlMode {
	MANUAL = 1,
	AUTOMATIC = 2,
};

int amdGetTemp(const char *path, int *temp);
int amdSetControlMode(const char *path, enum ControlMode mode);
int amdGetControlMode(const char *path, enum ControlMode *mode);
int amdSetFanPWM(const char *path, int pwm);
int amdGetFanPWM(const char *path, int *pwm);
int amdGetBusyPercent(const char *path, int *busy);
int amdGetPowerAvg(const char *path, int *p);
int amdGetFanMinRPM(const char *path, int *rpm);
int amdGetFanMaxRPM(const char *path, int *rpm);
int amdGetVendorProduct(const char *path, int *vendor, int *device);
int amdGetSubsystemIDs(const char *path, int *vendor, int *device);
