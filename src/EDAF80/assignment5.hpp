#pragma once

#include "core/InputHandler.h"
#include "core/FPSCamera.h"
#include "core/WindowManager.hpp"

#include "core/ShaderProgramManager.hpp"

class Window;


namespace edaf80
{
	//! \brief Wrapper class for Assignment 5
	class Assignment5 {
	public:
		//! \brief Default constructor.
		//!
		//! It will initialise various modules of bonobo and retrieve a
		//! window to draw to.
		Assignment5(WindowManager& windowManager);

		//! \brief Default destructor.
		//!
		//! It will release the bonobo modules initialised by the
		//! constructor, as well as the window.
		~Assignment5() = default;

		void setup_camera();
		void setup_program_manager();

		glm::vec3 get_step( float path_pos,
							std::vector<glm::vec3>& path );

		enum game_state {
			begin,		//! State for beginning of the game
			play,		//! State for playing the game
			game_over,	//! State for end of the game
		};

		//! \brief Contains the logic of the assignment, along with the
		//! render loop.
		void run();

	private:
		FPSCameraf     _camera;
		InputHandler   _inputHandler;
		WindowManager& _windowManager;
		GLFWwindow*    _window;
		ShaderProgramManager _program_manager;
		game_state _game_state;

		// Shaders identifiers
		GLuint _texcoord_shader;
		GLuint _dory_shader;
		GLuint _fallback_shader;
		GLuint _water_shader;
		GLuint _skybox_shader;
		GLuint _phong_shader;


	};
}
