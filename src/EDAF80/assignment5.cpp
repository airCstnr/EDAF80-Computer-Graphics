#include "assignment5.hpp"

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
#include "interpolation.hpp"

edaf80::Assignment5::Assignment5(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("EDAF80: Assignment 5", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}
}

void
edaf80::Assignment5::run()
{
	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, -10.0f, 0.0f)); // set start position underwater
	mCamera.mMouseSensitivity = 0.003f;
	mCamera.mMovementSpeed = 0.025;

	/* --------------------------------- Create the shader programs ---------------------------------------*/
	// Set program manager
	ShaderProgramManager program_manager;

	// Create and load the fallback shader
	GLuint texcoord_shader = 0u;
	program_manager.CreateAndRegisterProgram("Texture coords",
	                                         { { ShaderType::vertex, "EDAF80/texcoord.vert" },
	                                           { ShaderType::fragment, "EDAF80/texcoord.frag" } },
											texcoord_shader);
	if (texcoord_shader == 0u) {
		LogError("Failed to load texcoord shader");
		return;
	}

	// Add texture shader, using Celestial Ring shagers from assignment 1
	GLuint dory_shader = 0u;
	program_manager.CreateAndRegisterProgram("Dory texture mapping",
												{ { ShaderType::vertex, "EDAF80/dory.vert" },
												{ ShaderType::fragment, "EDAF80/dory.frag" } },
												dory_shader);
	if (dory_shader == 0u) {
		LogError("Failed to load Celestial Ring shader");
		return;
	}

	// Create and load the fallback shader
	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram("Fallback",
		{ { ShaderType::vertex, "EDAF80/fallback.vert" },
		  { ShaderType::fragment, "EDAF80/fallback.frag" } },
		fallback_shader);
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}

	// Create and load the water shader
	GLuint water_shader = 0u;
	program_manager.CreateAndRegisterProgram("Water",
											{ { ShaderType::vertex, "EDAF80/water.vert" },
											{ ShaderType::fragment, "EDAF80/water.frag" } },
											water_shader);
	if (water_shader == 0u) {
		LogError("Failed to load water shader");
		return;
	}

	// Create and load the skybox shader
	GLuint skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram("Sky Box",
											{ { ShaderType::vertex, "EDAF80/skybox.vert" },
											{ ShaderType::fragment, "EDAF80/skybox.frag" } },
											skybox_shader);
	if (skybox_shader == 0u) {
		LogError("Failed to load skybox shader");
		return;
	}

	GLuint phong_shader = 0u;
	program_manager.CreateAndRegisterProgram( "Phong shader",
											{ { ShaderType::vertex, "EDAF80/phong.vert" },
												{ ShaderType::fragment, "EDAF80/phong.frag" } },
											phong_shader );
	if(phong_shader == 0u) {
		LogError( "Failed to load phong shader" );
		return;
	}

	/* --------------------------------- Load  geometry ---------------------------------------*/

	// Load the quad shape for water geometry
	auto const water_shape = parametric_shapes::createQuadTess(200u, 200u, 100u);
	if (water_shape.vao == 0u) {
		LogError("Failed to retrieve the shape mesh");
		return;
	}

	// Load the sphere shape for sky box
	auto const sky_shape = parametric_shapes::createSphere(24, 20, 500);
	if (sky_shape.vao == 0u) {
		LogError("Failed to retrieve the shape mesh");
		return;
	}


	/* --------------------------------- Load  models  ---------------------------------------*/
	// Load Dory
	std::vector<bonobo::mesh_data> const dory_object = bonobo::loadObjects("dory.obj");
	if (dory_object.empty()) {
		LogError("Failed to load the dory model");
		return;
	}
	bonobo::mesh_data const& dory = dory_object.front();

	// Load mine
	std::vector<bonobo::mesh_data> const mine_object = bonobo::loadObjects( "mine.obj" );
	if(mine_object.empty()) {
		LogError( "Failed to load the mine model" );
		return;
	}
	bonobo::mesh_data const& mine = mine_object.front();

	// Load nemo
	std::vector<bonobo::mesh_data> const nemo_object = bonobo::loadObjects( "nemo.obj" );
	if(nemo_object.empty()) {
		LogError( "Failed to load the nemo model" );
		return;
	}
	bonobo::mesh_data const& nemo = nemo_object.front();

	/* --------------------------------- Set up uniforms ---------------------------------------*/

	auto light_position = glm::vec3(-2.0f, 4.0f, 2.0f);
	auto camera_position = mCamera.mWorld.GetTranslation();
	auto time = 0.0f;
	auto const set_uniforms = [&light_position, &camera_position, &time](GLuint program) {
		glUniform3fv(glGetUniformLocation(program, "light_position"), 1, glm::value_ptr(light_position));
		glUniform3fv(glGetUniformLocation(program, "camera_position"), 1, glm::value_ptr(camera_position));
		glUniform1f(glGetUniformLocation(program, "time"), time);
	};

	auto ambient = glm::vec3( 0.2f, 0.2f, 0.2f );
	auto diffuse = glm::vec3( 0.1f, 0.1f, 0.4f );
	auto specular = glm::vec3( 1.0f, 1.0f, 1.0f );
	auto shininess = 1.0f;
	auto const phong_set_uniforms = [&light_position, &camera_position, &ambient, &diffuse, &specular, &shininess]( GLuint program ) {
		glUniform3fv( glGetUniformLocation( program, "light_position" ), 1, glm::value_ptr( light_position ) );
		glUniform3fv( glGetUniformLocation( program, "camera_position" ), 1, glm::value_ptr( camera_position ) );
		glUniform3fv( glGetUniformLocation( program, "ambient" ), 1, glm::value_ptr( ambient ) );
		glUniform3fv( glGetUniformLocation( program, "diffuse" ), 1, glm::value_ptr( diffuse ) );
		glUniform3fv( glGetUniformLocation( program, "specular" ), 1, glm::value_ptr( specular ) );
		glUniform1f( glGetUniformLocation( program, "shininess" ), shininess );
	};

	/* --------------------------------- Set up nodes ---------------------------------------*/

	// Set up node for water
	auto water_node = Node();
	water_node.set_geometry(water_shape);
	water_node.set_program(&water_shader, set_uniforms);
	// Translate waves to set them at the center of the skybox
	water_node.get_transform().SetTranslate(glm::vec3(-100, 0, -100));

	// Set up node for skybox
	auto sky_node = Node();
	sky_node.set_geometry(sky_shape);
	sky_node.set_program(&skybox_shader, set_uniforms);

	// Set up node for dory
	auto dory_node = Node();
	dory_node.set_geometry(dory);
	dory_node.set_program(&dory_shader, set_uniforms);
	// Translate dory to set her in front of me
	dory_node.get_transform().SetTranslate( glm::vec3( 0, -15, -20 ) );
	// dory has to look in the same direction than me
	dory_node.get_transform().RotateX( glm::half_pi<float>() );
	dory_node.get_transform().RotateZ( glm::pi<float>() );

	// Set up node for mine
	auto mine_node = Node();
	mine_node.set_geometry( mine );
	mine_node.set_program( &phong_shader, phong_set_uniforms );
	mine_node.get_transform().SetScale( 0.1 );
	mine_node.get_transform().SetTranslate( glm::vec3( 20, -15, -20 ) );

	// Set up node for nemo
	auto nemo_node = Node();
	nemo_node.set_geometry( nemo );
	nemo_node.set_program( &phong_shader, phong_set_uniforms );
	nemo_node.get_transform().SetScale( 0.1 );
	nemo_node.get_transform().SetTranslate( glm::vec3( 0, -15, -20 ) );
	nemo_node.get_transform().RotateY( -glm::half_pi<float>() );


	/* --------------------------------- Load textures ---------------------------------------*/

	// Cloudy hills cubemap set
	auto sky_map = bonobo::loadTextureCubeMap("cloudyhills/posx.png", "cloudyhills/negx.png",
		"cloudyhills/posy.png", "cloudyhills/negy.png",
		"cloudyhills/posz.png", "cloudyhills/negz.png",
		true);

	// Add cube map to current node
	sky_node.add_texture("cube_map", sky_map, GL_TEXTURE_CUBE_MAP);
	water_node.add_texture("cube_map", sky_map, GL_TEXTURE_CUBE_MAP);

	// For wave ripples
	GLuint const wave_ripple_texture = bonobo::loadTexture2D("waves.png");
	water_node.add_texture("wave_ripple_texture", wave_ripple_texture, GL_TEXTURE_2D);

	// Load dory texture
	GLuint const dory_texture = bonobo::loadTexture2D("dory_texture.jpg");
	dory_node.add_texture("dory_texture", dory_texture, GL_TEXTURE_2D);


	/* --------------------------------- Motion management ---------------------------------------*/
	bool enable_dory_motion = true;
	float catmull_rom_tension = 0.5f;

	// Create random path
	size_t dory_path_length = 5; // number of points in the path
	float dory_path_keypoint; // random y value for the path generation
	std::vector<glm::vec3> dory_path_vector;


	for(size_t i = 0; i < dory_path_length; i++) {
		dory_path_keypoint = ((float( rand() ) / float( RAND_MAX )) * 2) - 1; // generate random number between -1 and 1
		dory_path_vector.push_back( glm::vec3( dory_path_keypoint, 0, -static_cast<float>(i) ) );
	}

	glm::vec3 dory_initial_position = dory_node.get_transform().GetTranslation(); // store initial position
	float dory_path_pos = 0.0f;
	float dory_velocity = 0.005f;
	int dory_current_point_index = 0;
	int dory_next_point_index = 0;
	int dory_next2_point_index = 0;
	int dory_previous_point_index = 0;
	float dory_distance_ratio = 0;


	/* --------------------------------- GL Parameters ---------------------------------------*/

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);

	/* --------------------------------- Time management ---------------------------------------*/

	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime, lastTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;

	/* --------------------------------- Log Windows Parameters ---------------------------------------*/

	bool show_logs = false;
	bool show_gui = false;
	bool shader_reload_failed = false;

	auto polygon_mode = bonobo::polygon_mode_t::fill;


	/* --------------------------------- Render loop ---------------------------------------*/

	while (!glfwWindowShouldClose(window)) {
		nowTime = GetTimeMilliseconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1000.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		// Increment time for waves movement
		time += 0.01;

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
		if(inputHandler.GetKeycodeState( GLFW_KEY_P ) & JUST_PRESSED) {
			// Switch between polygon modes
			polygon_mode = static_cast<bonobo::polygon_mode_t>((static_cast<int>(polygon_mode) + 1) % 3);
		}
		if(inputHandler.GetKeycodeState( GLFW_KEY_M ) & JUST_PRESSED) {
			// Enable/Disable Dory Motion
			enable_dory_motion = !enable_dory_motion;
		}

		ImGui_ImplGlfwGL3_NewFrame();

		// Move Dory
		if(enable_dory_motion) {

			// TODO : use modulo instead of this ugly if statements
			// TODO : create function to compute this path instead of pulluting main function

			dory_current_point_index = floor( dory_path_pos );
			dory_next_point_index = dory_current_point_index + 1;
			dory_next2_point_index = dory_current_point_index + 2;
			dory_previous_point_index = dory_current_point_index - 1;
			dory_distance_ratio = dory_path_pos - dory_current_point_index;

			// manage path circularity
			if(dory_current_point_index == dory_path_vector.size() - 2)
			{
				dory_next2_point_index = 0;
			}
			if(dory_current_point_index == dory_path_vector.size() - 1)
			{
				dory_next_point_index = 0;
				dory_next2_point_index = 1;
			}
			if(dory_current_point_index == dory_path_vector.size())
			{
				dory_current_point_index = 0;
				dory_next_point_index = 1;
				dory_next2_point_index = 2;
				dory_path_pos = 0;
			}
			if(dory_current_point_index == 0)
			{
				// initialize dory position after path loop
				dory_node.get_transform().SetTranslate( dory_initial_position );

				dory_previous_point_index = dory_path_vector.size() - 1;
			}

			// compute linear translation 
			glm::vec3 q_step = interpolation::evalCatmullRom(   dory_path_vector[dory_previous_point_index],
																dory_path_vector[dory_current_point_index],
																dory_path_vector[dory_next_point_index],
																dory_path_vector[dory_next2_point_index],
																catmull_rom_tension,
																dory_distance_ratio );

			// translate dory along the interpolated path
			dory_node.get_transform().Translate( q_step );
			dory_path_pos += dory_velocity;
		}

		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		bonobo::changePolygonMode( polygon_mode );

		if (!shader_reload_failed) {
			//
			// Render all geometries
			//
			//water_node.render(mCamera.GetWorldToClipMatrix());
			//sky_node.render(mCamera.GetWorldToClipMatrix());
			//dory_node.render(mCamera.GetWorldToClipMatrix());
			//mine_node.render( mCamera.GetWorldToClipMatrix() );
			nemo_node.render( mCamera.GetWorldToClipMatrix() );
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// custom ImGUI window
		//
		bool opened = ImGui::Begin( "Scene Control", &opened, ImVec2( 300, 100 ), -1.0f, 0 );
		if(opened) {
			bonobo::uiSelectPolygonMode( "Polygon mode", polygon_mode );
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
		edaf80::Assignment5 assignment5(framework.GetWindowManager());
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
