#include "assignment2.hpp"
#include "interpolation.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/Misc.h"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"
#include <imgui.h>
#include "external/imgui_impl_glfw_gl3.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <stdexcept>

edaf80::Assignment2::Assignment2(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 2", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}
}

void
edaf80::Assignment2::run()
{
	//Load the geometry
	//auto const shape = parametric_shapes::createCircleRing(4u, 60u, 1.0f, 2.0f);
	//auto const shape = parametric_shapes::createQuad(1u, 1u);
	auto const shape = parametric_shapes::createSphere(24, 20, 1);
	if (shape.vao == 0u)
		return;

	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.25f * 12.0f;

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

	GLuint diffuse_shader = 0u;
	program_manager.CreateAndRegisterProgram("Diffuse",
	                                         { { ShaderType::vertex, "EDAF80/diffuse.vert" },
	                                           { ShaderType::fragment, "EDAF80/diffuse.frag" } },
	                                         diffuse_shader);
	if (diffuse_shader == 0u)
		LogError("Failed to load diffuse shader");

	GLuint normal_shader = 0u;
	program_manager.CreateAndRegisterProgram("Normal",
	                                         { { ShaderType::vertex, "EDAF80/normal.vert" },
	                                           { ShaderType::fragment, "EDAF80/normal.frag" } },
	                                         normal_shader);
	if (normal_shader == 0u)
		LogError("Failed to load normal shader");

	GLuint tangent_shader = 0u;
	program_manager.CreateAndRegisterProgram("Tangent",
	                                         { { ShaderType::vertex, "EDAF80/tangent.vert" },
	                                           { ShaderType::fragment, "EDAF80/tangent.frag" } },
	                                         tangent_shader);
	if (tangent_shader == 0u)
		LogError("Failed to load tangent shader");

	GLuint binormal_shader = 0u;
	program_manager.CreateAndRegisterProgram("Bitangent",
	                                         { { ShaderType::vertex, "EDAF80/binormal.vert" },
	                                           { ShaderType::fragment, "EDAF80/binormal.frag" } },
	                                         binormal_shader);
	if (binormal_shader == 0u)
		LogError("Failed to load binormal shader");

	GLuint texcoord_shader = 0u;
	program_manager.CreateAndRegisterProgram("Texture coords",
	                                         { { ShaderType::vertex, "EDAF80/texcoord.vert" },
	                                           { ShaderType::fragment, "EDAF80/texcoord.frag" } },
	                                         texcoord_shader);
	if (texcoord_shader == 0u)
		LogError("Failed to load texcoord shader");

	auto const light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_uniforms = [&light_position](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

	// Set the default tensions value; it can always be changed at runtime
	// through the "Scene Controls" window.
	float catmull_rom_tension = 0.0f;

	// Set whether the default interpolation algorithm should be the linear one;
	// it can always be changed at runtime through the "Scene Controls" window.
	bool use_linear = true;

	// Set whether to interpolate the position of an object or not; it can
	// always be changed at runtime through the "Scene Controls" window.
	bool interpolate = true;

	// Set up node for the selected geometry
	auto geometry_node = Node();
	geometry_node.set_geometry(shape);
	geometry_node.set_program(&fallback_shader, set_uniforms);
	TRSTransformf& geometry_transform_ref = geometry_node.get_transform();

	//! \todo Create a tesselated torus

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);

	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeSeconds();
	double fpsNextTick = lastTime + 1.0;

	std::int32_t program_index = 0;
	auto polygon_mode = bonobo::polygon_mode_t::fill;
	bool show_logs = true;
	bool show_gui = true;

	// Create random path
	size_t path_length( 10 ); // number of points in the path
	float y_rand; // random y value for the path generation
	std::vector<glm::vec3>interpolation_path; // path vector

	for(size_t i=0; i < path_length; i++) {
		y_rand = ((float( rand() ) / float( RAND_MAX )) * 2) -1; // generate random number between -1 and 1
		interpolation_path.push_back( glm::vec3( static_cast<float>(i), y_rand, 0 ) );
	}

	while (!glfwWindowShouldClose(window)) {
		nowTime = GetTimeSeconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(ddeltatime, inputHandler);

		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;

		ImGui_ImplGlfwGL3_NewFrame();

		geometry_transform_ref.RotateY(0.01f);

		// set transform following a path

		if (interpolate) {
			//! \todo Interpolate the movement of a shape between various control points
			// increment the loop position along the path
			// Be careful to set it circular
			if (use_linear) {

				//! \todo Compute the interpolated position
				//!       using the linear interpolation.
			}
			else {
				//! \todo Compute the interpolated position
				//!       using the Catmull-Rom interpolation;
				//!       use the `catmull_rom_tension`
				//!       variable as your tension argument.
			}
		}

		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		bonobo::changePolygonMode(polygon_mode);

		geometry_node.render(mCamera.GetWorldToClipMatrix());

		bool const opened = ImGui::Begin("Scene Controls", nullptr, ImVec2(300, 100), -1.0f, 0);
		if (opened) {
			bonobo::uiSelectPolygonMode("Polygon mode", polygon_mode);
			auto selection_result = program_manager.SelectProgram("Shader", program_index);
			if (selection_result.was_selection_changed) {
				geometry_node.set_program(selection_result.program, set_uniforms);
			}
			ImGui::Separator();
			ImGui::Checkbox("Enable interpolation", &interpolate);
			ImGui::Checkbox("Use linear interpolation", &use_linear);
			ImGui::SliderFloat("Catmull-Rom tension", &catmull_rom_tension, 0.0f, 1.0f);
		}
		ImGui::End();

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

	float const tao = 0.5; //tense factor
	glm::mat4 mat4(glm::vec4(0, -tao, 2*tao, -tao), glm::vec4(1, 0, tao-3, 2-tao), glm::vec4(0, tao, 3 - 2 * tao, tao-2), glm::vec4(0, 0, -tao, tao));
	glm::mat3x4 mat3x4(glm::vec4(0,0,0,0), glm::vec4(1, 1, 1, 1), glm::vec4(2, 2, 2, 2));

	std::cout << mat4 << std::endl;
	std::cout << mat3x4 << std::endl;

	glm::vec3 p0(0, 0, 0),
		p1(0, 1, 2),
		p2(1, 1, 2),
		p3(1, 1, 0);
	//float x(0);
	std::cout << interpolation::evalLERP(p0, p1, 0) << std::endl;
	std::cout << interpolation::evalLERP(p0, p1, 0.5) << std::endl;
	std::cout << interpolation::evalLERP(p0, p1, 0.75) << std::endl;
	std::cout << interpolation::evalLERP(p0, p1, 1) << std::endl;

	for (float x = 0; x <= 1; x += 0.1) {
		std::cout << interpolation::evalCatmullRom(p0, p1, p2, p3, tao, x) << std::endl;
	}

	try {
		edaf80::Assignment2 assignment2(framework.GetWindowManager());
		assignment2.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}

}
