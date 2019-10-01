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
	auto const shape = parametric_shapes::createSphere(124, 120, 1);
	//auto const shape = parametric_shapes::createDice( 1.0f );

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

	// Add texture shader, using Celestial Ring shagers from assignment 1
	GLuint texture_shader = 0u;
	program_manager.CreateAndRegisterProgram("Texture mapping",
											 { { ShaderType::vertex, "EDAF80/celestial_ring.vert" },
											 { ShaderType::fragment, "EDAF80/celestial_ring.frag" } },
											 texture_shader );
	if(texture_shader == 0u)
		LogError( "Failed to generate the “Celestial Ring” shader program: exiting." );


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
	bool interpolate = false;

	// Set up node for the selected geometry
	auto geometry_node = Node();
	geometry_node.set_geometry(shape);
	geometry_node.set_program(&fallback_shader, set_uniforms);
	TRSTransformf& geometry_transform_ref = geometry_node.get_transform();

	//Load a texture for the geometry
	GLuint const geometry_node_texture = bonobo::loadTexture2D( "earth_diffuse.png" ); //load the earth texture
	geometry_node.add_texture( "diffuse_texture", geometry_node_texture, GL_TEXTURE_2D );

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
	size_t path_length( 5 ); // number of points in the path
	float y_rand; // random y value for the path generation
	std::vector<glm::vec3>interpolation_path; // path vector

	for(size_t i=0; i < path_length; i++) {
		y_rand = ((float( rand() ) / float( RAND_MAX )) * 2) -1; // generate random number between -1 and 1
		interpolation_path.push_back( glm::vec3( static_cast<float>(i), y_rand, 0 ) );
	}

	float path_pos = 0.0f;
	float pos_velocity = 0.05f;
	int current_point_index = 0;
	int next_point_index = 0;
	int next2_point_index = 0;
	int previous_point_index = 0;
	float distance_ratio = 0;

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

		if (interpolate) {
			// Interpolate the movement of a shape between various control points

			if (use_linear) {
				current_point_index = floor(path_pos);
				next_point_index = current_point_index + 1;
				distance_ratio = path_pos - current_point_index;

				// manage path circularity
				if (current_point_index == interpolation_path.size() - 1)
				{
					next_point_index = 0;
				}
				if (current_point_index == interpolation_path.size())
				{
					current_point_index = 0;
					next_point_index = 1;
					path_pos = 0;
				}

				// compute linear translation 
				glm::vec3 p_step = interpolation::evalLERP(	interpolation_path[current_point_index],
															interpolation_path[next_point_index],
															distance_ratio);

				// set translation interpolatewd along the path
				geometry_transform_ref.SetTranslate(p_step);				

			}
			else {
				//!		 Compute the interpolated position
				//!       using the Catmull-Rom interpolation;
				//!       use the `catmull_rom_tension`
				//!       variable as your tension argument.

				current_point_index = floor(path_pos);
				next_point_index = current_point_index + 1;
				next2_point_index = current_point_index + 2;
				previous_point_index = current_point_index - 1;
				distance_ratio = path_pos - current_point_index;

				// manage path circularity
				if (current_point_index == interpolation_path.size() - 2)
				{
					next2_point_index = 0;
				}
				if (current_point_index == interpolation_path.size() - 1)
				{
					next_point_index = 0;
					next2_point_index = 1;
				}
				if (current_point_index == interpolation_path.size())
				{
					current_point_index = 0;
					next_point_index = 1;
					next2_point_index = 2;
					path_pos = 0;
				}
				if (current_point_index == 0)
				{
					previous_point_index = interpolation_path.size() - 1;
				}

				// compute linear translation 
				glm::vec3 q_step = interpolation::evalCatmullRom(	interpolation_path[previous_point_index],
																	interpolation_path[current_point_index],
																	interpolation_path[next_point_index],
																	interpolation_path[next2_point_index],
																	catmull_rom_tension,
																	distance_ratio);

				// set translation interpolatewd along the path
				geometry_transform_ref.SetTranslate(q_step);

			}

			path_pos += pos_velocity;
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

	try {
		edaf80::Assignment2 assignment2(framework.GetWindowManager());
		assignment2.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}

}
