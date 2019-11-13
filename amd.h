#pragma once

enum ControlMode {
	MANUAL = 1,
	AUTOMATIC = 2,
};

int amdReadTemp(const char *path, int *temp);
int amdSetControlMode(const char *path, enum ControlMode mode);
int amdSetFanPWM(const char *path, int pwm);
