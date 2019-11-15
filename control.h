#pragma once

struct Card {
	const char *name;
	const char *path;

	int vendorID, productID;
	int sub_vendorID, sub_deviceID;
};

struct Measurements {
	int temp;      // milli-degrees Celsius
	int busy;      // 0 - 100%
	int power_avg; // microwatts

	int fan_min; // RPM
	int fan_max; // RPM
};

struct Action {
	int pwm; // 0-255
};

struct Control {
	struct Card card;

	int interval;
	int verbose;
	const char *parameters;

	void *state;

	int (*init)(void **state);
	int (*finalize)(void *state);
	int (*parseParameters)(void *state, const char *p);
	int (*control)(void *state, const struct Measurements *const m, struct Action *const a);
};

void newCard(struct Card *c, const char *name, const char *path);
void deleteCard(struct Card *c);
