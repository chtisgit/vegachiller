#include "amd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int amdReadTemp(const char *path_, int *temp)
{
	static const char file[] = "/hwmon/hwmon0/temp1_input";
	char *path = malloc(strlen(path_) + sizeof file);
	strcpy(path, path_);
	strcat(path, file);

	FILE *f = fopen(path, "rb");
	if (f == NULL)
		return 1;

	if (fscanf(f, "%d\n", temp) != 1)
		return 1;

	fclose(f);

	return 0;
}

int amdSetControlMode(const char *path_, enum ControlMode mode)
{
	if(mode != AUTOMATIC && mode != MANUAL) {
		return 1;
	}

	static const char file[] = "/hwmon/hwmon0/pwm1_enable";
	char *path = malloc(strlen(path_) + sizeof file);
	strcpy(path, path_);
	strcat(path, file);

	FILE *f = fopen(path, "wb");
	if (f == NULL)
		return 1;

	fprintf(f, "%d\n", (int)mode);
	fclose(f);

	return 0;
}

int amdSetFanPWM(const char *path_, int pwm)
{
	if(pwm < 0 || pwm > 255) {
		return 1;
	}

	static const char file[] = "/hwmon/hwmon0/pwm1";
	char *path = malloc(strlen(path_) + sizeof file);
	strcpy(path, path_);
	strcat(path, file);

	FILE *f = fopen(path, "wb");
	if (f == NULL)
		return 1;

	fprintf(f, "%d\n", pwm);
	fclose(f);

	return 0;
}

