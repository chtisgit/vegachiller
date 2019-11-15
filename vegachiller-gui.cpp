#include "imgui.h"
#include "imgui_impl_opengl2.h"
#include "imgui_impl_sdl.h"
#include <SDL.h>
#include <SDL_opengl.h>
#include <cstdio>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "amd.h"
#include "control.h"

#define FONT_TTF "/usr/share/fonts/truetype/roboto/hinted/Roboto-Regular.ttf"

std::map<int, const char *> sus_vendor = {{0x1002, "AMD"},	{0x1043, "ASUSTeK"},
					  {0x196D, "Club 3D"},    {0x1092, "Diamond Multimedia"},
					  {0x18BC, "GeCube"},     {0x1458, "Gigabyte"},
					  {0x17AF, "HIS"},	{0x16F3, "Jetway"},
					  {0x1462, "MSI"},	{0x1DA2, "Sapphire"},
					  {0x148C, "PowerColor"}, {0x1545, "VisionTek"},
					  {0x1682, "XFX"},	{0x1025, "Acer"},
					  {0x106B, "Apple"},      {0x1028, "Dell"},
					  {0x107B, "Gateway"},    {0x103C, "HP"},
					  {0x17AA, "Lenovo"},     {0x104D, "Sony"},
					  {0x1179, "Toshiba"},    {0x1849, "ASRock"}};

std::vector<Card> detectCards()
{
	std::vector<Card> res;
	char path[100];

	for (int i = 0; i != 1000; i++) {
		sprintf(path, "/sys/class/drm/card%d", i);
		char *name = path + 15;
		const int len = strlen(path);

		struct stat st;
		if (stat(path, &st) != 0)
			break;

		Card card;
		strcpy(path + len, "/device");
		card.path = strdup(path);
		amdGetVendorProduct(path, &card.vendorID, &card.productID);
		amdGetSubsystemIDs(path, &card.sub_vendorID, &card.sub_deviceID);
		amdGetFanMinRPM(path, &card.fan_min);
		amdGetFanMaxRPM(path, &card.fan_max);
		sprintf(path + len, ": %s", sus_vendor.at(card.sub_vendorID));
		card.name = strdup(name);
		res.push_back(card);
	}

	return res;
}

int measureAmd(const char *path, struct Measurements *m)
{
	int (*todo[])(const char *, int *) = {amdGetTemp, amdGetBusyPercent, amdGetPowerAvg,
					      amdGetFanPWM};
	int *dsts[] = {&m->temp, &m->busy, &m->power_avg, &m->pwm};

	for (int i = 0; i != sizeof(dsts) / sizeof(*dsts); i++) {
		int err = todo[i](path, dsts[i]);
		if (err != 0)
			return err;
	}

	return 0;
}

struct MeasurementBuffer {
	std::vector<Measurements> buf;
	size_t i;

	MeasurementBuffer(size_t N) : buf(N), i(0)
	{
	}

	void add(const Measurements &m)
	{
		buf[i++] = m;
		if (i == size())
			i = 0;
	}

	size_t size()
	{
		return buf.size();
	}

	Measurements &newest()
	{
		return i != 0 ? buf[i - 1] : buf[buf.size() - 1];
	}

	Measurements &operator[](size_t n)
	{
		n = (n + i) % buf.size();
		return buf[n];
	}
};

enum Page { PAGE_INFO, PAGE_FAN };

int main(int, char **)
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
		printf("Error: %s\n", SDL_GetError());
		return -1;
	}

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_WindowFlags window_flags =
	    (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Window *window = SDL_CreateWindow("Vegachiller GUI", SDL_WINDOWPOS_CENTERED,
					      SDL_WINDOWPOS_CENTERED, 960, 540, window_flags);
	SDL_GLContext gl_context = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, gl_context);
	SDL_GL_SetSwapInterval(1); // Enable vsync

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO &io = ImGui::GetIO();

	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowRounding = 0;

	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL2_Init();

	ImVec4 clear_color = ImVec4(0.45f, 0.1f, 0.1f, 1.00f);

	ImFont *font1 = io.Fonts->AddFontFromFileTTF(FONT_TTF, 18);
	ImFont *font_title = io.Fonts->AddFontFromFileTTF(FONT_TTF, 24);

	auto cards = detectCards();
	auto selectedCard = 0;
	auto activePage = PAGE_INFO;
	std::mutex meas_lock;
	std::vector<MeasurementBuffer> meas(cards.size(), MeasurementBuffer(120));

	std::thread measure_thread([&] {
		std::vector<Measurements> tmp(cards.size());
		for (;;) {
			for (size_t i = 0; i != meas.size(); i++) {
				measureAmd(cards[i].path, &tmp[i]);
			}
			{
				std::lock_guard<std::mutex> lock(meas_lock);
				for (size_t i = 0; i != meas.size(); i++) {
					meas[i].add(tmp[i]);
				}
			}

			usleep(1000000);
		}
	});

	bool done = false;
	while (!done) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL2_ProcessEvent(&event);
			if (event.type == SDL_QUIT)
				done = true;
		}

		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplSDL2_NewFrame(window);
		ImGui::NewFrame();
		ImGui::PushFont(font1);

		ImGui::Begin("sidebar", nullptr,
			     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
				 ImGuiWindowFlags_NoResize);
		ImGui::SetWindowPos(ImVec2{10, 60});
		ImGui::SetWindowSize(ImVec2{100, 470});
		if (ImGui::Button("Info", ImVec2{80, 30})) {
			activePage = PAGE_INFO;
		}
		if (ImGui::Button("Fan", ImVec2{80, 30})) {
			activePage = PAGE_FAN;
		}
		ImGui::End();

		ImGui::Begin("top", nullptr,
			     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
				 ImGuiWindowFlags_NoResize);
		ImGui::SetWindowPos(ImVec2{120, 10});
		ImGui::SetWindowSize(ImVec2{820, 40});
		if (ImGui::BeginCombo("Graphics Card", cards.size() ? cards[selectedCard].name
								    : "<no cards detected>")) {
			for (size_t i = 0; i != cards.size(); i++) {
				ImGui::Selectable(cards[i].name, selectedCard == i);
			}
			ImGui::EndCombo();
		}
		ImGui::End();

		ImGui::Begin("main", nullptr,
			     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove |
				 ImGuiWindowFlags_NoResize);
		ImGui::SetWindowPos(ImVec2{120, 60});
		ImGui::SetWindowSize(ImVec2{820, 470});

		if (activePage == PAGE_INFO) {
			std::lock_guard<std::mutex> lock(meas_lock);
			ImGui::PushFont(font_title);
			ImGui::Text("Info");
			ImGui::PopFont();

			auto &m = meas[selectedCard];

			ImGui::Text("Temperature: %d °C", m.newest().temp / 1000);
			ImGui::SameLine(400);
			ImGui::Text("GPU Load: %d %%", m.newest().busy);

			ImGui::PlotLines("",
					 [](void *data, int i) -> float {
						 auto *meas =
						     static_cast<MeasurementBuffer *>(data);
						 return (*meas)[i].temp / 1000;
					 },
					 &m, m.size(), 0, nullptr, 0, 100, ImVec2{350, 100});

			ImGui::SameLine(400);

			ImGui::PlotLines("",
					 [](void *data, int i) -> float {
						 auto *meas =
						     static_cast<MeasurementBuffer *>(data);
						 return (*meas)[i].busy;
					 },
					 &m, m.size(), 0, nullptr, 0, 100, ImVec2{350, 100});

			ImGui::Text("Fan Speed: %.1f %%", m.newest().pwm / 2.55);
			ImGui::SameLine(400);
			ImGui::Text("Avg. Power used: %d W", m.newest().power_avg / 1000000);

			ImGui::PlotLines("",
					 [](void *data, int i) -> float {
						 auto *meas =
						     static_cast<MeasurementBuffer *>(data);
						 return (*meas)[i].pwm / 2.55;
					 },
					 &m, m.size(), 0, nullptr, 0, 100, ImVec2{350, 100});

			ImGui::SameLine(400);

			ImGui::PlotLines("",
					 [](void *data, int i) -> float {
						 auto *meas =
						     static_cast<MeasurementBuffer *>(data);
						 return (*meas)[i].power_avg / 1000000;
					 },
					 &m, m.size(), 0, nullptr, 0, 300, ImVec2{350, 100});
		}

		ImGui::End();

		ImGui::PopFont();
		ImGui::Render();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);
	}

	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
