#include "assignment5.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/Misc.h"

#include <imgui.h>
#include <external/imgui_impl_glfw_gl3.h>
#include <tinyfiledialogs.h>

#include <stdexcept>

#include "parametric_shapes.hpp"
#include "core/node.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "interpolation.hpp"

using namespace edaf80;

Assignment5::Assignment5( WindowManager& windowManager ) :
	_camera( 0.5f * glm::half_pi<float>(),
			static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
			0.01f, 1000.0f ),
	_inputHandler(),
	_windowManager( windowManager ),
	_window( nullptr ),
	_program_manager()
{
	WindowManager::WindowDatum window_datum{ _inputHandler, _camera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	_window = _windowManager.CreateGLFWWindow("EDAF80: Assignment 5", window_datum, config::msaa_rate);
	if (_window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}
}

void edaf80::Assignment5::setup_camera()
{
	/* --------------------------------- Setup camera & motion ---------------------------------------*/
	// Set up the camera
	_camera.mWorld.SetTranslate( glm::vec3( 0.0f, -10.0f, 0.0f ) ); // set start position underwater
	_camera.mMouseSensitivity = 0.003f;
	_camera.mMovementSpeed = 0.07;
}

void edaf80::Assignment5::setup_program_manager()
{
	// Set program manager
	_program_manager.CreateAndRegisterProgram( "Texture coords",
												 { { ShaderType::vertex, "EDAF80/texcoord.vert" },
												   { ShaderType::fragment, "EDAF80/texcoord.frag" } },
												_texcoord_shader );
	if(_texcoord_shader == 0u) {
		LogError( "Failed to load texcoord shader" );
		return;
	}

	_program_manager.CreateAndRegisterProgram( "Dory texture mapping",
												{ { ShaderType::vertex, "EDAF80/dory.vert" },
												{ ShaderType::fragment, "EDAF80/dory.frag" } },
												_dory_shader );
	if(_dory_shader == 0u) {
		LogError( "Failed to load Celestial Ring shader" );
		return;
	}

	_program_manager.CreateAndRegisterProgram( "Fallback",
												{ { ShaderType::vertex, "EDAF80/fallback.vert" },
												  { ShaderType::fragment, "EDAF80/fallback.frag" } },
												_fallback_shader );
	if(_fallback_shader == 0u) {
		LogError( "Failed to load fallback shader" );
		return;
	}

	_program_manager.CreateAndRegisterProgram( "Water",
												{ { ShaderType::vertex, "EDAF80/water.vert" },
												{ ShaderType::fragment, "EDAF80/water.frag" } },
												_water_shader );
	if(_water_shader == 0u) {
		LogError( "Failed to load water shader" );
		return;
	}

	_program_manager.CreateAndRegisterProgram( "Sky Box",
												{ { ShaderType::vertex, "EDAF80/skybox.vert" },
												{ ShaderType::fragment, "EDAF80/skybox.frag" } },
												_skybox_shader );
	if(_skybox_shader == 0u) {
		LogError( "Failed to load skybox shader" );
		return;
	}

	_program_manager.CreateAndRegisterProgram( "Phong shader",
												{ { ShaderType::vertex, "EDAF80/phong.vert" },
												{ ShaderType::fragment, "EDAF80/phong.frag" } },
												_phong_shader );
	if(_phong_shader == 0u) {
		LogError( "Failed to load phong shader" );
		return;
	}

	_program_manager.CreateAndRegisterProgram("Countdown shader ",
		{ { ShaderType::vertex, "EDAF80/countdown.vert" },
		{ ShaderType::fragment, "EDAF80/countdown.frag" } },
		_countdown_shader);
	if (_countdown_shader == 0u) {
		LogError("Failed to countdown_shader shader");
		return;
	}
}


/* Returns step vector for next position
 * @param path position
 * @param path
 */
glm::vec3 Assignment5::get_step( float path_pos,
								 std::vector<glm::vec3>& path ) {
	float catmull_rom_tension = 0.5f;
	float frequency = 0.1;

	int p0 = ((int)floor( path_pos ) -1 + path.size()) % path.size();
	int p1 = ((int)floor( path_pos ) +0 + path.size()) % path.size();
	int p2 = ((int)floor( path_pos ) +1 + path.size()) % path.size();
	int p3 = ((int)floor( path_pos ) +2 + path.size()) % path.size();

	float distance_ratio = path_pos - floor( path_pos );

	// Switch between linear and catmull-rom
	//*
	glm::vec3 step = interpolation::evalCatmullRom( path[p0],
													path[p1],
													path[p2],
													path[p3],
													catmull_rom_tension,
													distance_ratio );
	/*/
	glm::vec3 step = interpolation::evalLERP( path[p1],
											  path[p2],
											  distance_ratio );
	//*/

	// add sinus to y
	step[1] = sin( path_pos * glm::two_pi<float>() * frequency );

	return step;
}


void
edaf80::Assignment5::run()
{
	/* --------------------------------- Setup camera & motion ---------------------------------------*/
	setup_camera();

	/* --------------------------------- Load the shader programs ---------------------------------------*/
	setup_program_manager();

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

	// Load the quad shape for the countdown
	auto const quad_shape = parametric_shapes::createQuadTess(10, 10, 10);
	if (quad_shape.vao == 0u) {
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

	auto light_position = glm::vec3(-2.0f, 40.0f, 2.0f);
	auto camera_position = _camera.mWorld.GetTranslation();
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

	auto nemo_ambient = glm::vec3( (252/255), (140/255), (3/255) );
	auto nemo_diffuse = glm::vec3( 0.9f, 0.3f, 0.1f );
	auto nemo_specular = glm::vec3( 1.0f, 1.0f, 1.0f );
	auto nemo_shininess = 2.0f;
	auto const nemo_set_uniforms = [&light_position, &camera_position, &nemo_ambient, &nemo_diffuse, &nemo_specular, &nemo_shininess]( GLuint program ) {
		glUniform3fv( glGetUniformLocation( program, "light_position" ), 1, glm::value_ptr( light_position ) );
		glUniform3fv( glGetUniformLocation( program, "camera_position" ), 1, glm::value_ptr( camera_position ) );
		glUniform3fv( glGetUniformLocation( program, "ambient" ), 1, glm::value_ptr( nemo_ambient ) );
		glUniform3fv( glGetUniformLocation( program, "diffuse" ), 1, glm::value_ptr( nemo_diffuse ) );
		glUniform3fv( glGetUniformLocation( program, "specular" ), 1, glm::value_ptr( nemo_specular ) );
		glUniform1f( glGetUniformLocation( program, "shininess" ), nemo_shininess );
	};

	/* --------------------------------- Set up nodes ---------------------------------------*/

	// Set up node for water
	auto water_node = Node();
	water_node.set_geometry(water_shape);
	water_node.set_program(&_water_shader, set_uniforms);
	// Translate waves to set them at the center of the skybox
	water_node.get_transform().SetTranslate(glm::vec3(-100, 0, -100));

	// Set up node for skybox
	auto sky_node = Node();
	sky_node.set_geometry(sky_shape);
	sky_node.set_program(&_skybox_shader, set_uniforms);

	// Set up node for dory
	auto dory_node = Node();
	dory_node.set_geometry(dory);
	dory_node.set_program(&_dory_shader, set_uniforms);
	// Translate dory to set her in front of me
	dory_node.get_transform().SetTranslate( glm::vec3( 0, -15, -25 ) );
	// dory has to look in the same direction than me
	dory_node.get_transform().RotateX( glm::half_pi<float>() );
	dory_node.get_transform().RotateZ( glm::pi<float>() );
	// Add hitbox value for Dory
	dory_node.set_hitbox_radius( 10 );

	// Set up node for mine
	int mines_number = 20;
	int x_value = 1;
	std::vector<Node> mine_node_vector = std::vector<Node>(mines_number);
	for(size_t i = 1; i < mines_number; i++)
	{
		auto mine_node = Node();
		mine_node.set_geometry( mine );
		mine_node.set_program( &_phong_shader, phong_set_uniforms );
		mine_node.get_transform().SetScale( 0.1 );
		mine_node.get_transform().SetTranslate( glm::vec3( 20*x_value + 5*cos(i), -15 + 3*sin(i), -(20*(float)i) ) );
		mine_node.set_hitbox_radius( 10 );
		mine_node_vector.push_back( mine_node );
		x_value *= -1;
	}

	// Set up node for nemo
	auto nemo_node = Node();
	nemo_node.set_geometry( nemo );
	nemo_node.set_program( &_phong_shader, nemo_set_uniforms );
	nemo_node.get_transform().SetScale( 0.1 );
	//nemo_node.get_transform().SetTranslate( glm::vec3( -2.5, -15, -10 ) );
	auto nemo_camera_translation = glm::vec3( -2.5, -15, -10 );
	nemo_node.get_transform().RotateY( -glm::half_pi<float>() );
	nemo_node.set_hitbox_radius( 5 );

	// Set up node for quad
	auto quad_node = Node();
	quad_node.set_geometry(quad_shape);
	quad_node.set_program(&_countdown_shader, set_uniforms);
	quad_node.get_transform().SetTranslate(glm::vec3(0, -13, -20));
	quad_node.get_transform().RotateX(-glm::half_pi<float>());

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

	// Load number texture
	GLuint const number_three_texture = bonobo::loadTexture2D("three.png");
	quad_node.add_texture("number_trhee_texture", number_three_texture, GL_TEXTURE_2D);
	GLuint const number_two_texture = bonobo::loadTexture2D("two.png");
	quad_node.add_texture("number_two_texture", number_two_texture, GL_TEXTURE_2D);
	GLuint const number_one_texture = bonobo::loadTexture2D("one.png");
	quad_node.add_texture("number_one_texture", number_one_texture, GL_TEXTURE_2D);


	/* --------------------------------- Motion management ---------------------------------------*/
	bool enable_dory_motion = true;

	// Create random path
	size_t dory_path_length = 16; // number of points in the path
	float dory_path_keypoint = 1; // random y value for the path generation
	std::vector<glm::vec3> dory_path_vector;

	for(size_t i = 0; i < dory_path_length; i++) {
		dory_path_keypoint = ((float( rand() ) / float( RAND_MAX )) * 2) - 1; // generate random float number between -1 and 1
		//dory_path_keypoint *= 5; // transform number to be between -5 and 5
		dory_path_vector.push_back( glm::vec3( dory_path_keypoint, 0, -1 ) );
	}

	float dory_path_pos = 0.0f;
	float dory_velocity = 0.1f;


	/* --------------------------------- GL Parameters ---------------------------------------*/

	glEnable(GL_DEPTH_TEST);

	// Enable face culling to improve performance:
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_FRONT);
	//glCullFace(GL_BACK);

	/* --------------------------------- Time management ---------------------------------------*/

	f64 ddeltatime;
	size_t fpsSamples = 0;
	double nowTime;
	double lastTime = GetTimeMilliseconds();
	double startTime = GetTimeMilliseconds();
	double fpsNextTick = lastTime + 1000.0;

	/* --------------------------------- Log Windows Parameters ---------------------------------------*/

	bool show_logs = false;
	bool show_gui = true;
	bool shader_reload_failed = false;

	auto polygon_mode = bonobo::polygon_mode_t::fill;

	/* --------------------------------- Setup game state ---------------------------------------*/
	_game_state = game_state::begin; // when starting game, the state is "begin"
	enable_dory_motion = false; // dory is stopped

	/* --------------------------------- Render loop ---------------------------------------*/

	while (!glfwWindowShouldClose(_window)) {

		// handle time between two frames
		nowTime = GetTimeMilliseconds();
		ddeltatime = nowTime - lastTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1000.0;
			fpsSamples = 0;
		}
		fpsSamples++;

		// Increment time for waves movement
		time += 0.01;

		// Game logic if current state is begin ("loading" before game start to give player a chance to prepare")
		if (_game_state == game_state::begin) {
			// Update game state after 3 seconds
			if (nowTime - startTime > 3000) {
				_game_state = game_state::play;
			}
		}

		// Game logic if current state is play
		if (_game_state == game_state::play) {

			// Update game state to game_over if Dory gets to far away
			if (nemo_node.distance(dory_node) > 100) {
				std::cerr << "You failed, Dorry got to far away" << std::endl;
				_game_state = game_state::game_over;
			}

			// Update game state according to hitboxes (Dory)
			if (nemo_node.hits(dory_node)) {
				std::cerr << "You failed hitting Dory!" << std::endl;
				_game_state = game_state::game_over;
			}

			// Update game state according to hitboxes (mine)
			for (Node mine_node : mine_node_vector) {
				if (nemo_node.hits(mine_node)) {
					std::cerr << "You failed hitting a mine!" << std::endl;
					_game_state = game_state::game_over;
				}
			}
			// Update the game after
			if (nowTime - startTime > 60000) {
				std::cerr << "You win! You followed Dory all the way!" << std::endl;
				_game_state = game_state::game_over;
			}
		}
		// Update variables according to game state
		switch(_game_state)
		{
			case edaf80::Assignment5::begin:
				enable_dory_motion = false;
				break;
			case edaf80::Assignment5::play:
				enable_dory_motion = true;
				break;
			case edaf80::Assignment5::game_over:
				enable_dory_motion = false;
				break;
			default:
				// do nothing
				break;
		}

		auto& io = ImGui::GetIO();
		_inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		_inputHandler.Advance();
		_camera.Update(ddeltatime, _inputHandler);

		if (_inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (_inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (_inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			shader_reload_failed = !_program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
				                   "An error occurred while reloading shader programs; see the logs for details.\n"
				                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
				                   "error");
		}
		if(_inputHandler.GetKeycodeState( GLFW_KEY_P ) & JUST_PRESSED) {
			// Switch between polygon modes
			polygon_mode = static_cast<bonobo::polygon_mode_t>((static_cast<int>(polygon_mode) + 1) % 3);
		}
		if(_inputHandler.GetKeycodeState( GLFW_KEY_SPACE ) & JUST_PRESSED) {
			// Enable/Disable Dory Motion
			enable_dory_motion = !enable_dory_motion;
		}

		ImGui_ImplGlfwGL3_NewFrame();

		// Move Dory
		if(enable_dory_motion) {
			// translate dory of one step
			glm::vec3 step = get_step( dory_path_pos, dory_path_vector ); // get step
			step = glm::normalize( step ) * dory_velocity; // make this step proportional to velocity
			dory_node.get_transform().Translate( step ); // apply step
			// dory looks in her translation direction!
			dory_node.get_transform().RotateX( glm::dot( step, glm::vec3( 0, 1, 0 ) ) * dory_velocity );
			dory_node.get_transform().RotateY( glm::dot( step, dory_node.get_transform().GetBack() ) * dory_velocity );
			// increase dory postion using her velocity
			dory_path_pos += dory_velocity;
		}
		
		// Move Nemo
		nemo_node.get_transform().SetTranslate( _camera.mWorld.GetTranslation() + nemo_camera_translation );

		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(_window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);
		glClearDepthf(1.0f);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		bonobo::changePolygonMode( polygon_mode );

		if (!shader_reload_failed) {
			//
			// Render all geometries
			//
			water_node.render(_camera.GetWorldToClipMatrix());
			sky_node.render(_camera.GetWorldToClipMatrix());
			dory_node.render(_camera.GetWorldToClipMatrix());
			for(Node mine_node : mine_node_vector) {
				mine_node.render( _camera.GetWorldToClipMatrix() );
			}
			nemo_node.render( _camera.GetWorldToClipMatrix() );

			if (_game_state == game_state::begin) {
				quad_node.render(_camera.GetWorldToClipMatrix());
			}
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// custom ImGUI window
		//
		//bool opened = ImGui::Begin( "Scene Control", &opened, ImVec2( 300, 100 ), -1.0f, 0 );
		//if(opened) {
		//	bonobo::uiSelectPolygonMode( "Polygon mode", polygon_mode );
		//}
		//ImGui::End();

		bool game_stats = ImGui::Begin( "Game Stats", &game_stats, ImVec2( 300, 100 ), -1.0f, 0 );
		if(game_stats) {
			/*
			This text should be rendered on screen before the game starts and not in GUI
			Not sure if GLUT is needed or there is some other way
			ImGui::Text("Dory has finally learned the address to Marvin");
			ImGui::Text("But now she is in a hurry");
			ImGui::Text("Try to keep up and avoid any mines along the way");
			ImGui::Text("The game will start in 3 seconds");
			*/
			if (_game_state == game_state::begin) {
				ImGui::Text("Get ready!");
				ImGui::Text("Distance : %.0f cm", dory_path_pos);
				ImGui::Text("Time : %.0f s", time);
			}
			if (_game_state == game_state::play) {
				if (nemo_node.distance(dory_node) < 75) {
					ImGui::Text("Keep going, you're doing great!");
				}
				else {
					ImGui::Text("Dory is getting away, hurry up!");
				}
				ImGui::Text("Distance : %.0f cm", dory_path_pos);
				ImGui::Text("Time : %.0f s", time);
			}
			if (_game_state == game_state::game_over) {
				if (nowTime - startTime > 60000) {
					ImGui::Text("You win!You followed Dory all the way!");
				}
				else {
					ImGui::Text("Good try, you will make it next time!");
				}
			}

			// TODO : print best score?
			//ImGui::Text( "Best Score: " );
		}
		ImGui::End();

		if (show_logs)
			Log::View::Render();
		if (show_gui)
			ImGui::Render();


		glfwSwapBuffers(_window);
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
