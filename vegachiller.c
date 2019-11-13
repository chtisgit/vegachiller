#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "amd.h"
#include "control.h"
#include "controls/curve/curve.h"

#define MAX_DEVICES 1

static volatile int terminate_req = 0;

void setTerminateReq(int sig)
{
	terminate_req = 1;
}

struct Args {
	int verbose;  // bool
	int interval; // ms
	const char *dev[MAX_DEVICES];
	const char *name[MAX_DEVICES];
	const char *parameters[MAX_DEVICES];
	const char *type[MAX_DEVICES];
};

struct Args defaultArgs()
{
	struct Args args = {.verbose = 0, .interval = 6000};

	return args;
}

struct Args parseArgs(int argc, char **argv)
{
	struct Args args = defaultArgs();

	int devices = -1;
	for (;;) {
		int option_index = 0;
		static struct option long_options[] = {{"device", required_argument, 0, 'd'},
						       {"name", required_argument, 0, 'l'},
						       {"interval", required_argument, 0, 'i'},
						       {"verbose", no_argument, 0, 'v'},
						       {"parameters", required_argument, 0, 'p'},
						       {"type", required_argument, 0, 't'},
						       {0, 0, 0, 0}};

		int c = getopt_long(argc, argv, "d:i:l:p:t:v", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'd':
			++devices;
			args.dev[devices] = optarg;
			args.name[devices] = "<unnamed>";
			args.parameters[devices] = "";
			args.type[devices] = NULL;
			break;

		case 'i':
			args.interval = atoi(optarg);
			break;

		case 'l':
			if (devices < 0) {
				fprintf(stderr, "error: provide device path first");
				exit(1);
			}

			args.name[devices] = optarg;
			break;

		case 'p':
			if (devices < 0) {
				fprintf(stderr, "error: provide device path first");
				exit(1);
			}

			args.parameters[devices] = optarg;
			break;

		case 't':
			if (devices < 0) {
				fprintf(stderr, "error: provide device path first");
				exit(1);
			}

			args.type[devices] = optarg;
			break;

		case 'v':
			args.verbose = 1;
			break;

		default:
			fprintf(stderr, "error: unknown parameter %d. Exiting", c);
			exit(1);
		}
	}

	return args;
}

int run(struct Control *const ctrl)
{
	const useconds_t usec = ctrl->interval * 1000;
	int err = 0;

	if (ctrl->init(&ctrl->state) != 0) {
		fprintf(stderr, "error: failed to initialize control\n");
			goto fail;
	}

	if (ctrl->parseParameters(ctrl->state, ctrl->parameters) != 0) {
		fprintf(stderr, "error: failed to parse parameters\n");
			goto fail;
	}

	int lastPWM = -1;

	fprintf(stderr, "Switching to manual control for '%s' (%s)\n", ctrl->card.name, ctrl->card.path);
	amdSetControlMode(ctrl->card.path, MANUAL);
	while (!terminate_req) {
		struct Measurements m;
		struct Action action;

		if (amdReadTemp(ctrl->card.path, &m.temp) != 0) {
			fprintf(stderr, "error: cannot read temperature\n");
			goto fail;
		}

		if (ctrl->verbose)
			fprintf(stderr, "info: temp: %d °C  pwm: %d (range 0-255)\n", m.temp / 1000, lastPWM);

		if (ctrl->control(ctrl->state, &m, &action) != 0) {
			fprintf(stderr, "error: control function failed\n");
			goto fail;
		}

		if (action.pwm < 0 || action.pwm > 255) {
			fprintf(stderr, "error: returned out-of-range value\n");
			goto fail;
		}

		if (lastPWM != action.pwm) {
			if (ctrl->verbose)
				fprintf(stderr, "info: adjusting Fan PWM to %d\n", action.pwm);
			amdSetFanPWM(ctrl->card.path, lastPWM = action.pwm);
		}

		usleep(usec);
	}

	goto good;
fail:
	err = 1;
good:

	fprintf(stderr, "Switching to automatic control for '%s' (%s)\n", ctrl->card.name, ctrl->card.path);
	amdSetControlMode(ctrl->card.path, AUTOMATIC);

	ctrl->finalize(ctrl->state);

	return err;
}

int main(int argc, char **argv)
{
	struct Args args = parseArgs(argc, argv);

	if (args.type[0] == NULL) {
		fprintf(stderr, "error: select a control type.\n");
		return 1;
	}

	struct Control ctrl;
	if (strcmp(args.type[0], "curve") == 0) {
		newCurveControl(&ctrl);
	} else {
		fprintf(stderr, "error: no such control type.\n");
		return 1;
	}

	newCard(&ctrl.card, args.name[0], args.dev[0]);
	ctrl.interval = args.interval;
	ctrl.verbose = args.verbose;
	ctrl.parameters = args.parameters[0];

	signal(SIGINT, setTerminateReq);
	signal(SIGTERM, setTerminateReq);
	signal(SIGSEGV, setTerminateReq);
	signal(SIGQUIT, setTerminateReq);

	run(&ctrl);

	return 0;
}
