#include "config.hpp"
#include "parametric_shapes.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/Misc.h"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"

#include <imgui.h>
#include <external/imgui_impl_glfw_gl3.h>

#include <stack>
#include <set>
#include <cstdlib>

#include "CelestialBody.hpp"

#include "glm/trigonometric.hpp"

#include "glm/gtc/constants.hpp"

/*
	EDAF 80 Fall 2019
	Raphael Castanier
	Niklas Karlsson
*/

int main()
{
	//
	// Set up the framework
	//
	Bonobo framework;

	//
	// Set up the camera
	//
	InputHandler input_handler;
	FPSCameraf camera(0.5f * glm::half_pi<float>(),
		static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
		0.01f, 1000.0f);
	camera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	camera.mMouseSensitivity = 0.003f;
	camera.mMovementSpeed = 0.25f * 12.0f;

	//
	// Create the window
	//
	WindowManager& window_manager = framework.GetWindowManager();
	WindowManager::WindowDatum window_datum{ input_handler, camera, config::resolution_x, config::resolution_y, 0, 0, 0, 0 };
	GLFWwindow* window = window_manager.CreateGLFWWindow("EDAF80: Assignment 1", window_datum, config::msaa_rate);
	if (window == nullptr) {
		LogError("Failed to get a window: exiting.");

		return EXIT_FAILURE;
	}

	//
	// Load the sphere geometry
	//
	std::vector<bonobo::mesh_data> const objects = bonobo::loadObjects("sphere.obj");
	if (objects.empty()) {
		LogError("Failed to load the sphere geometry: exiting.");

		return EXIT_FAILURE;
	}
	bonobo::mesh_data const& sphere = objects.front();


	//
	// Create the shader program
	//
	ShaderProgramManager program_manager;
	GLuint celestial_body_shader = 0u;
	program_manager.CreateAndRegisterProgram("Celestial Body",
		{ { ShaderType::vertex, "EDAF80/default.vert" },
		  { ShaderType::fragment, "EDAF80/default.frag" } },
		celestial_body_shader);
	if (celestial_body_shader == 0u) {
		LogError("Failed to generate the “Celestial Body” shader program: exiting.");

		Log::View::Destroy();
		Log::Destroy();

		return EXIT_FAILURE;
	}
	GLuint celestial_ring_shader = 0u;
	program_manager.CreateAndRegisterProgram("Celestial Ring",
		{ { ShaderType::vertex, "EDAF80/celestial_ring.vert" },
		  { ShaderType::fragment, "EDAF80/celestial_ring.frag" } },
		celestial_ring_shader);
	if (celestial_ring_shader == 0u) {
		LogError("Failed to generate the “Celestial Ring” shader program: exiting.");

		Log::View::Destroy();
		Log::Destroy();

		return EXIT_FAILURE;
	}


	//---------------------------------------------------------------------------------------------------------------------------------------------
	//Lab modifications

	//Set up the sun node and other related attributes
	GLuint const sun_texture = bonobo::loadTexture2D("sunmap.png");						//load the texture
	CelestialBody sun_node(sphere, &celestial_body_shader, sun_texture);				//create the celestialBody node 
	sun_node.set_scale(glm::vec3(0.5, 0.5, 0.5));										//scaling
	sun_node.set_spinning(glm::radians(10.0), glm::pi<float>(), glm::radians(45.0));	//spinning
	sun_node.set_orbit(0.0f, glm::radians(10.0f), 0.0f, 0.0f);							// orbiting

	/*
	Create a ring shape
	Arguments:
		radial_resolution = how many vertices will be present across a radial cut of the ring
		angular_resolution = how many vertices will be used to form the outer border of the ring
		inner_radius = the discance between the center and the inner border of the ring
		outer_radius = the distance between the center and the outer border of the ring
	*/
	bonobo::mesh_data rings_shape = parametric_shapes::createCircleRing(10, 256, 1.2f, 2.0f);

	/*
	Load textures from res/textures folder.
		Diffuse texture : http://planetpixelemporium.com/download/download.php?saturnringcolor.jpg
		Opacity texture : http://planetpixelemporium.com/download/download.php?saturnringpattern.gif
		The diffuse texture uses the name "diffuse_texture" and is of type GL_TEXTURE_2D
		the opacity texture uses the name "opacity_texture" but is also of type GL_TEXTURE_2D
	*/
	GLuint const rings_diffuse_texture = bonobo::loadTexture2D( "saturnringcolor.jpg" ); //load the texture
	GLuint const rings_opacity_texture = bonobo::loadTexture2D( "saturnringpattern.gif" ); //load the texture

	// Add rings to sun
	sun_node.add_rings( rings_shape,
						{ 0, 0 },
						&celestial_ring_shader,
						rings_diffuse_texture,
						rings_opacity_texture);

	//---------------------------------------------------------------------------------------------------------------------------------------------

	Node solar_system_node;
	//solar_system_node.add_child(&sun_node);

	//
	// Nodes for the remaining of the solar system
	//

	// Set up the Mercury node and other related attributes
	GLuint const mercury_texture = bonobo::loadTexture2D("mercurymap.jpg");						// load the texture
	CelestialBody mercury_node(sphere, &celestial_body_shader, mercury_texture);				// create the earth node 
	mercury_node.set_scale(glm::vec3(0.02, 0.02, 0.02));										// set scaling
	mercury_node.set_spinning(glm::radians(2.0), 2*glm::pi<float>()/180, glm::radians(45.0));	// set spinning
	mercury_node.set_orbit(glm::radians(3.4), 2*glm::pi<float>()/4, 2);							// set orbiting

	// Set up the Venus node and other related attributes
	GLuint const venus_texture = bonobo::loadTexture2D("venusmap.jpg");								// load the texture
	CelestialBody venus_node(sphere, &celestial_body_shader, venus_texture);						// create the earth node 
	venus_node.set_scale(glm::vec3(0.05, 0.05, 0.05));												// set scaling
	venus_node.set_spinning(glm::radians(177.0), 2 * glm::pi<float>() / 600, glm::radians(45.0));	// set spinning
	venus_node.set_orbit(glm::radians(3.9), 2 * glm::pi<float>() / 12, 3);							// set orbiting


	// Set up the Earth node and other related attributes
	GLuint const earth_texture = bonobo::loadTexture2D( "earth_diffuse.png" );				 // load the texture
	CelestialBody earth_node( sphere, &celestial_body_shader, earth_texture );				 // create the earth node 
	earth_node.set_scale( glm::vec3( 0.25, 0.25, 0.25 ) );									 // set scaling
	earth_node.set_spinning( glm::radians( 10.0 ), glm::pi<float>(), glm::radians( 45.0 ) ); // set spinning
	earth_node.set_orbit( 0.0f, glm::radians( 180.0f ), 2 );								 

	// Set up the Moon node and other related attributes
	GLuint const moon_texture = bonobo::loadTexture2D( "noise.png" );						 // load the texture
	CelestialBody moon_node( sphere, &celestial_body_shader, moon_texture );				 // create the moon node 
	moon_node.set_scale( glm::vec3( 0.25, 0.25, 0.25 ) );									 // set scaling
	moon_node.set_spinning( glm::radians( 10.0 ), glm::pi<float>(), glm::radians( 45.0 ) );  // set spinning
	moon_node.set_orbit( 0.0f, glm::radians( 180.0f ), 3.5 );								 // set orbiting

	// Set up the Mars node and other related attributes
	GLuint const mars_texture = bonobo::loadTexture2D("marsmap1k.jpg");								// load the texture
	CelestialBody mars_node(sphere, &celestial_body_shader, moon_texture);							// create the moon node 
	mars_node.set_scale(glm::vec3(0.03, 0.03, 0.03));												// set scaling
	mars_node.set_spinning(glm::radians(25.0), (2.0 * glm::pi<float>())/3, glm::radians(45.0));		// set spinning
	mars_node.set_orbit(5.7f, (2.0 * glm::pi<float>()) / 36, 5.0);									// set orbiting

	// Make the Earth a child of the Sun, and the Moon a child of the Earth
	sun_node.add_child( &mercury_node);
	sun_node.add_child( &venus_node);
	sun_node.add_child( &earth_node );
	earth_node.add_child( &moon_node );

	// Retrieve the actual framebuffer size: for HiDPI monitors, you might
	// end up with a framebuffer larger than what you actually asked for.
	// For example, if you ask for a 1920x1080 framebuffer, you might get a
	// 3840x2160 one instead.
	int framebuffer_width, framebuffer_height;
	glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);

	glViewport(0, 0, framebuffer_width, framebuffer_height);
	glClearDepthf(1.0f);
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glEnable(GL_DEPTH_TEST);


	size_t fpsSamples = 0;
	double lastTime = GetTimeSeconds();
	double fpsNextTick = lastTime + 1.0;


	bool show_logs = true;
	bool show_gui = true;

	while (!glfwWindowShouldClose(window)) {
		//
		// Compute timings information
		//
		double const nowTime = GetTimeSeconds();
		double const delta_time = nowTime - lastTime;
		lastTime = nowTime;
		if (nowTime > fpsNextTick) {
			fpsNextTick += 1.0;
			fpsSamples = 0;
		}
		++fpsSamples;


		//
		// Process inputs
		//
		glfwPollEvents();

		ImGuiIO const& io = ImGui::GetIO();
		input_handler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);
		input_handler.Advance();
		camera.Update(delta_time, input_handler);

		if (input_handler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (input_handler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;


		//
		// Start a new frame for Dear ImGui
		//
		ImGui_ImplGlfwGL3_NewFrame();


		//
		// Clear the screen
		//
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


		//
		// Update the transforms
		//
		//sun_transform_reference.RotateY(sun_spin_speed * delta_time);


		//
		// Traverse the scene graph and render all nodes
		//
		std::stack<CelestialBody*> node_stack({ &sun_node });
		std::set<CelestialBody*> discovered_nodes;
		std::stack<glm::mat4> matrix_stack({ glm::mat4(1.0f) });

		// traversal of the scene graph and rendering of all its nodes.
		/*
		Pseudocode from https://en.wikipedia.org/wiki/Depth-first_search#Pseudocode
		1  procedure DFS-iterative(G,v):
		2      let S be a stack
		3      S.push(v)
		4      while S is not empty
		5          v = S.pop()
		6          if v is not labeled as discovered:
		7              label v as discovered
		8              for all edges from v to w in G.adjacentEdges(v) do
		9                  S.push(w)
		*/
		CelestialBody* current_node = node_stack.top();
		while(!node_stack.empty()) {
			current_node = node_stack.top();

			// render
			current_node->render( delta_time, camera.GetWorldToClipMatrix() );

			// traverse children
			node_stack.pop();
			if(discovered_nodes.find( current_node ) == discovered_nodes.end()) {
				discovered_nodes.insert( current_node );
				for(auto child : current_node->get_children()) {
					node_stack.push( child );
				}
			}
		}

		//
		// Display Dear ImGui windows
		//
		if (show_logs)
			Log::View::Render();
		if (show_gui)
			ImGui::Render();


		//
		// Queue the computed frame for display on screen
		//
		glfwSwapBuffers(window);
	}

	glDeleteTextures(1, &sun_texture);

	return EXIT_SUCCESS;
}
