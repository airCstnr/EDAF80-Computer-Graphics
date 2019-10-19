#pragma once

#include "core/InputHandler.h"
#include "core/FPSCamera.h"
#include "core/WindowManager.hpp"
#include "core/ShaderProgramManager.hpp"

#include "core/node.hpp"
#include "core/helpers.hpp"

#include "external/glad/glad.h"

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
		void setup_meshes();
		void setup_uniforms();
		void setup_nodes();


		glm::vec3 get_step( float path_pos,
							std::vector<glm::vec3>& path );

		//! \brief Contains the logic of the assignment, along with the
		//! render loop.
		void run();

	private:
		FPSCameraf     _camera;
		InputHandler   _inputHandler;
		WindowManager& _windowManager;
		GLFWwindow*    _window;
		ShaderProgramManager _program_manager;
		float _time = 0.0f;

		// Shaders identifiers
		GLuint _texcoord_shader;
		GLuint _dory_shader;
		GLuint _fallback_shader;
		GLuint _water_shader;
		GLuint _skybox_shader;
		GLuint _phong_shader;

		// Objects meshes
		bonobo::mesh_data _water_shape;
		bonobo::mesh_data _sky_shape;
		bonobo::mesh_data _dory_shape;
		bonobo::mesh_data _mine_shape;
		bonobo::mesh_data _nemo_shape;


		std::function<void( GLuint )> _simple_set_uniforms;
		std::function<void( GLuint )> _phong_set_uniforms;
		std::function<void( GLuint )> _nemo_set_uniforms;

		// Assert nodes
		Node _water_node;
		Node _sky_node;
		Node _dory_node;
		Node _mine_node;
		Node _nemo_node;

	};
}
