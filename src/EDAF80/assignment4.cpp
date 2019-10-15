#include "assignment4.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/Misc.h"
#include "core/ShaderProgramManager.hpp"

#include <imgui.h>
#include <external/imgui_impl_glfw_gl3.h>
#include <tinyfiledialogs.h>

#include <stdexcept>

#include "parametric_shapes.hpp"
#include "core/node.hpp"
#include <glm/gtc/type_ptr.hpp>

edaf80::Assignment4::Assignment4(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 4", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}
}

void
edaf80::Assignment4::run()
{

	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 3.0f, 6.0f));
	mCamera.mWorld.SetRotateX(50.0f);
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.025;

	// Create the shader programs
	ShaderProgramManager program_manager;
	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram("Fallback",
	                                         { { ShaderType::vertex, "EDAF80/fallback.vert" },
	                                           { ShaderType::fragment, "EDAF80/fallback.frag" } },
	                                         fallback_shader);
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}

	//
	// Creation of other shader programs.
	//

	// Create and load the water shader
	GLuint water_shader = 0u;
	program_manager.CreateAndRegisterProgram("Water",
											{ { ShaderType::vertex, "EDAF80/water.vert" },
											{ ShaderType::fragment, "EDAF80/water.frag" } },
											water_shader);
	if (water_shader == 0u)
		LogError("Failed to load water shader");

	// Create and load the skybox shader
	GLuint skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram( "Sky Box",
											 { { ShaderType::vertex, "EDAF80/skybox.vert" },
											   { ShaderType::fragment, "EDAF80/skybox.frag" } },
											 skybox_shader );
	if(skybox_shader == 0u)
		LogError( "Failed to load texcoord shader" );


	// Load the quad shape for water geometry
	auto const water_shape = parametric_shapes::createQuadTess(200u, 200u, 100u);
	if (water_shape.vao == 0u) {
		LogError("Failed to retrieve the shape mesh");
		return;
	}

	// Load the sphere shape for sky box
	auto const sky_shape = parametric_shapes::createSphere( 24, 20, 200 );
	if(sky_shape.vao == 0u) {
		LogError( "Failed to retrieve the shape mesh" );
		return;
	}

	// Set up the uniforms
	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto time = 0.0f;
	auto const set_uniforms = [&light_position, &camera_position, &time](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform1f(glGetUniformLocation(program, "time" ), time );
	};

	// Set up node for water
	auto water_node = Node();
	water_node.set_geometry(water_shape);
	water_node.set_program(&water_shader, set_uniforms);

	// Set up node for skybox
	auto sky_node = Node();
	sky_node.set_geometry( sky_shape );
	sky_node.set_program( &skybox_shader, set_uniforms );

	
	// Cloudy hills cubemap set
	auto sky_map = bonobo::loadTextureCubeMap(	"cloudyhills/posx.png", "cloudyhills/negx.png",
												"cloudyhills/posy.png", "cloudyhills/negy.png",
												"cloudyhills/posz.png", "cloudyhills/negz.png",
												true);

	// Add cube map to current node
	sky_node.add_texture("cube_map", sky_map, GL_TEXTURE_CUBE_MAP);

	// For wave ripples
	GLuint const wave_ripple_texture = bonobo::loadTexture2D("waves.png");
	water_node.add_texture("wave_ripple_texture", wave_ripple_texture, GL_TEXTURE_2D);

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);


	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;

	std::int32_t fallback_program_index = 0;
	auto polygon_mode = bonobo::polygon_mode_t::fill;
	bool show_logs = false;
	bool show_gui = false;
	bool shader_reload_failed = false;

	while (!glfwWindowShouldClose(window)) {
		nowTime = GetTimeMilliseconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1000.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		time += 0.1;

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(ddeltatime, inputHandler);

		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
				                   "An error occurred while reloading shader programs; see the logs for details.\n"
				                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
				                   "error");
		}

		ImGui_ImplGlfwGL3_NewFrame();

		//
		// Todo: If you need to handle inputs, you can do it here
		//




		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		bonobo::changePolygonMode( polygon_mode );

		if (!shader_reload_failed) {
			//
			// Render all geometry
			//
			water_node.render(mCamera.GetWorldToClipMatrix());
			sky_node.render( mCamera.GetWorldToClipMatrix() );
		}


		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// custom ImGUI window
		//
		bool opened = ImGui::Begin( "Scene Control", &opened, ImVec2( 300, 100 ), -1.0f, 0 );
		if(opened) {
			bonobo::uiSelectPolygonMode( "Polygon mode", polygon_mode );
			/*auto geometry_node_selection_result = program_manager.SelectProgram( "Fallback", fallback_program_index );
			if(geometry_node_selection_result.was_selection_changed) {
				geometry_node.set_program( geometry_node_selection_result.program, set_uniforms );
			}*/
		}
		ImGui::End();

		if (show_logs)
			Log::View::Render();
		if (show_gui)
			ImGui::Render();

		glfwSwapBuffers(window);
		lastTime = nowTime;
	}
}

int main()
{
	Bonobo framework;

	try {
		edaf80::Assignment4 assignment4(framework.GetWindowManager());
		assignment4.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
