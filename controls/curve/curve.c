#include "curve.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Config {
	int points;
	int *temp;
	int *pwm;
};

struct State {
	struct Config conf;
};

static int init(void **state)
{
	*state = calloc(1, sizeof(struct State));
	if(*state == NULL){
		return 1;
	}

	return 0;    
}

static int finalize(void *state_)
{
	struct State *state = state_;

	if(state->conf.temp) {
		free(state->conf.temp);
	}

	if(state->conf.pwm) {
		free(state->conf.pwm);
	}

	free(state);
	return 0;
}

static int parse(struct Config *conf, int i, const char *p)
{
	int temp;
	int pwm;

	if (i < 0 || i >= conf->points) {
		return 1;
	}

	if (sscanf(p, "%d:%d", &temp, &pwm) != 2) {
		return 1;
	}

	conf->temp[i] = temp * 1000;
	conf->pwm[i] = pwm;

	return 0;
}

static int parseParameters(void *state_, const char *p)
{
	struct State *state = state_;

	int points = 1;
	for (int i = 0; p[i] != '\0'; i++) {
		if (p[i] == ',') {
			points++;
		}
	}
	state->conf.points = points;
	state->conf.temp = malloc(sizeof(*state->conf.temp) * points);
	state->conf.pwm = malloc(sizeof(*state->conf.pwm) * points);

	int i;
	for (i = 0; i != points; i++) {
		const char *comma = strchr(p, ',');
		if (comma == NULL)
			break;
		
		if(parse(&state->conf, i, p) != 0){
			return 1;
		}

		p = comma+1;
	}

	return parse(&state->conf, i, p);
}

static int control(void *state_, const struct Measurements *const meas, struct Action *const action)
{
	struct State *state = state_;

	const int *const temp = state->conf.temp;
	const int *const pwm = state->conf.pwm;
	const int points = state->conf.points;

	int i;
	for(i = 0; i != points; i++){
		if(meas->temp >= temp[i]){
			break;
		}
	}

	if(i == points) {
		// temperature is below all thresholds. Stop fans.
		action->pwm = 0;
		return 0;
	}

	if(i == points-1){
		// temperature is in the highest segment of the curve.
		action->pwm = pwm[i];
		return 0;
	}

	// calculate arithmetic mean
	const int pwmdiff = (pwm[i+1] - pwm[i]);
	const int tempdiff = (temp[i+1] - temp[i]);
	action->pwm = (meas->temp - temp[i]) * pwmdiff / tempdiff + pwm[i];
	
	return 0;
}

void newCurveControl(struct Control *c)
{
	c->init = init;
	c->finalize = finalize;
	c->parseParameters = parseParameters;
	c->control = control;
}