#include "assignment3.hpp"
#include "interpolation.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/Misc.h"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"

#include <imgui.h>
#include <external/imgui_impl_glfw_gl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tinyfiledialogs.h>

#include <cstdlib>
#include <stdexcept>

edaf80::Assignment3::Assignment3(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 3", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}
}

void
edaf80::Assignment3::run()
{

	//Load the geometry
	//auto const shape = parametric_shapes::createCircleRing(4u, 60u, 1.0f, 2.0f);
	auto const shape = parametric_shapes::createSphere(24, 20, 1);
	if (shape.vao == 0u){
		LogError("Failed to retrieve the shape mesh");
		return;
	}

	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
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

	GLuint texcoord_shader = 0u;
	program_manager.CreateAndRegisterProgram("Texture coords",
	                                         { { ShaderType::vertex, "EDAF80/texcoord.vert" },
	                                           { ShaderType::fragment, "EDAF80/texcoord.frag" } },
	                                         texcoord_shader);
	if (texcoord_shader == 0u)
		LogError("Failed to load texcoord shader");

	GLuint phong_shader = 0u;
	program_manager.CreateAndRegisterProgram("Phong shader",
											{ { ShaderType::vertex, "EDAF80/phong.vert" },
												{ ShaderType::fragment, "EDAF80/phong.frag" } },
											phong_shader);
	if (phong_shader == 0u)
		LogError("Failed to load phong shader");

	GLuint normal_mapping_shader = 0u;
	program_manager.CreateAndRegisterProgram("Normal mapping shader",
											{ { ShaderType::vertex, "EDAF80/normal_mapping.vert" },
												{ ShaderType::fragment, "EDAF80/normal_mapping.frag" } },
											normal_mapping_shader);
	if (normal_mapping_shader == 0u)
		LogError("Failed to load normal mapping shader");

	GLuint skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram( "Sky Box",
											 { { ShaderType::vertex, "EDAF80/skybox.vert" },
											   { ShaderType::fragment, "EDAF80/skybox.frag" } },
											 skybox_shader );
	if(skybox_shader == 0u)
		LogError( "Failed to load texcoord shader" );
	
	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto const set_uniforms = [&light_position](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
	};

	auto camera_position = mCamera.mWorld.GetTranslation();
	auto ambient = glm::vec3(0.2f, 0.2f, 0.2f);
	auto diffuse = glm::vec3(0.7f, 0.2f, 0.4f);
	auto specular = glm::vec3(1.0f, 1.0f, 1.0f);
	auto shininess = 1.0f;
	auto const phong_set_uniforms = [&light_position,&camera_position,&ambient,&diffuse,&specular,&shininess](GLuint program){
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform3fv(glGetUniformLocation(program, "ambient"), 1, glm::value_ptr(ambient));
		glUniform3fv(glGetUniformLocation(program, "diffuse"), 1, glm::value_ptr(diffuse));
		glUniform3fv(glGetUniformLocation(program, "specular"), 1, glm::value_ptr(specular));
		glUniform1f(glGetUniformLocation(program, "shininess"), shininess);
	};

	// Set up node for the selected geometry
	auto geometry_node = Node();
	geometry_node.set_geometry(shape);
	geometry_node.set_program(&fallback_shader, set_uniforms);

	// Add texture shader, using Celestial Ring shagers from assignment 1
	GLuint texture_shader = 0u;
	program_manager.CreateAndRegisterProgram("Texture mapping",
		{ { ShaderType::vertex, "EDAF80/celestial_ring.vert" },
		{ ShaderType::fragment, "EDAF80/celestial_ring.frag" } },
		texture_shader);
	if (texture_shader == 0u)
		LogError("Failed to generate the “Celestial Ring” shader program: exiting.");

	GLuint const geometry_node_texture = bonobo::loadTexture2D("earth_diffuse.png"); //load the earth texture
	geometry_node.add_texture("diffuse_texture", geometry_node_texture, GL_TEXTURE_2D);

	// For normal/bump mapping
	GLuint const fieldstone_bump_texture = bonobo::loadTexture2D("fieldstone_bump.png");		//load the fieldstone bump texture
	geometry_node.add_texture("fieldstone_bump_texture", fieldstone_bump_texture, GL_TEXTURE_2D);

	// Cube map load texture
	auto my_cube_map_id = bonobo::loadTextureCubeMap( "sunset_sky/posx.png", "sunset_sky/negx.png",
													  "sunset_sky/posy.png", "sunset_sky/negy.png",
													  "sunset_sky/posz.png", "sunset_sky/negz.png",
													  true );

	// Add cube map to current node
	geometry_node.add_texture( "cube_map", my_cube_map_id, GL_TEXTURE_CUBE_MAP );

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);


	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;

	std::int32_t circle_ring_program_index = 0;
	auto polygon_mode = bonobo::polygon_mode_t::fill;
	bool show_logs = true;
	bool show_gui = true;
	bool shader_reload_failed = false;

	while (!glfwWindowShouldClose(window)) {
		nowTime = GetTimeMilliseconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1000.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		mCamera.Update(ddeltatime, inputHandler);

		ImGui_ImplGlfwGL3_NewFrame();

		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
				                   "An error occurred while reloading shader programs; see the logs for details.\n"
				                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
				                   "error");
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;

		camera_position = mCamera.mWorld.GetTranslation();

		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
		bonobo::changePolygonMode(polygon_mode);

		geometry_node.render(mCamera.GetWorldToClipMatrix());

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		bool opened = ImGui::Begin("Scene Control", &opened, ImVec2(300, 100), -1.0f, 0);
		if (opened) {
			bonobo::uiSelectPolygonMode("Polygon mode", polygon_mode);
			auto geometry_node_selection_result = program_manager.SelectProgram("Circle ring", circle_ring_program_index);
			if (geometry_node_selection_result.was_selection_changed) {
				geometry_node.set_program(geometry_node_selection_result.program, phong_set_uniforms);
			}
			ImGui::Separator();
			ImGui::ColorEdit3("Ambient", glm::value_ptr(ambient));
			ImGui::ColorEdit3("Diffuse", glm::value_ptr(diffuse));
			ImGui::ColorEdit3("Specular", glm::value_ptr(specular));
			ImGui::SliderFloat("Shininess", &shininess, 0.0f, 1000.0f);
			ImGui::SliderFloat3("Light Position", glm::value_ptr(light_position), -20.0f, 20.0f);
		}
		ImGui::End();

		ImGui::Begin("Render Time", &opened, ImVec2(120, 50), -1.0f, 0);
		if (opened)
			ImGui::Text("%.3f ms", ddeltatime);
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
		edaf80::Assignment3 assignment3(framework.GetWindowManager());
		assignment3.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
