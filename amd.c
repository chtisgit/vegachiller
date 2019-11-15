#include "amd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATH_LEN 1024

static int amdReadInt(const char *path_, const char *path_ext, int path_ext_len, int *res)
{
	char path[MAX_PATH_LEN];
	if (strlen(path_) + path_ext_len >= MAX_PATH_LEN - 1)
		return 1;

	strcpy(path, path_);
	strcat(path, path_ext);

	FILE *f = fopen(path, "rb");
	if (f == NULL)
		return 1;

	char tmp[30];

	if (fscanf(f, "%s\n", tmp) != 1)
		return 1;

	*res = (int) strtol(tmp, NULL, 0);

	fclose(f);

	return 0;
}

static int amdWriteInt(const char *path_, const char *path_ext, int path_ext_len, int value)
{
	char path[MAX_PATH_LEN];
	if (strlen(path_) + path_ext_len >= MAX_PATH_LEN - 1)
		return 1;

	strcpy(path, path_);
	strcat(path, path_ext);

	FILE *f = fopen(path, "wb");
	if (f == NULL)
		return 1;

	fprintf(f, "%d\n", value);
	if(fclose(f) != 0)
		return 1;

	return 0;
}

int amdGetTemp(const char *path, int *temp)
{
	static const char file[] = "/hwmon/hwmon0/temp1_input";
	return amdReadInt(path, file, sizeof(file) - 1, temp);
}

int amdSetControlMode(const char *path, enum ControlMode mode)
{
	if (mode != AUTOMATIC && mode != MANUAL) {
		return 1;
	}

	static const char file[] = "/hwmon/hwmon0/pwm1_enable";
	return amdWriteInt(path, file, sizeof(file) - 1, (int)mode);
}

int amdGetControlMode(const char *path, enum ControlMode *mode)
{
	static const char file[] = "/hwmon/hwmon0/pwm1_enable";
	int imode;
	int res = amdReadInt(path, file, sizeof(file) - 1, &imode);
	*mode = (enum ControlMode) imode;
	return res;
}

int amdSetFanPWM(const char *path, int pwm)
{
	if (pwm < 0 || pwm > 255) {
		return 1;
	}

	static const char file[] = "/hwmon/hwmon0/pwm1";
	return amdWriteInt(path, file, sizeof(file) - 1, pwm);
}

int amdGetFanPWM(const char *path, int *pwm)
{
	static const char file[] = "/hwmon/hwmon0/pwm1";
	return amdReadInt(path, file, sizeof(file) - 1, pwm);
}

int amdGetBusyPercent(const char *path, int *busy)
{
	static const char file[] = "/gpu_busy_percent";
	return amdReadInt(path, file, sizeof(file) - 1, busy);
}

int amdGetPowerAvg(const char *path, int *p)
{
	static const char file[] = "/hwmon/hwmon0/power1_average";
	return amdReadInt(path, file, sizeof(file) - 1, p);
}

int amdGetFanMinRPM(const char *path, int *rpm)
{
	static const char file[] = "/hwmon/hwmon0/fan1_min";
	return amdReadInt(path, file, sizeof(file) - 1, rpm);
}

int amdGetFanMaxRPM(const char *path, int *rpm)
{
	static const char file[] = "/hwmon/hwmon0/fan1_max";
	return amdReadInt(path, file, sizeof(file) - 1, rpm);
}

int amdGetVendorProduct(const char *path, int *vendor, int *device)
{
	if (vendor != NULL) {
		static const char file1[] = "/vendor";
		int res = amdReadInt(path, file1, sizeof(file1) - 1, vendor);
		if (res != 0) {
			return res;
		}
	}

	if (device != NULL) {
		static const char file2[] = "/device";
		return amdReadInt(path, file2, sizeof(file2) - 2, device);
	}

	return 0;
}

int amdGetSubsystemIDs(const char *path, int *vendor, int *device)
{
	if (vendor != NULL) {
		static const char file1[] = "/subsystem_vendor";
		int res = amdReadInt(path, file1, sizeof(file1) - 1, vendor);
		if (res != 0) {
			return res;
		}
	}

	if (device != NULL) {
		static const char file2[] = "/subsystem_device";
		return amdReadInt(path, file2, sizeof(file2) - 2, device);
	}

	return 0;
}