#include "CelestialBody.hpp"
#include "glm/gtc/matrix_transform.hpp"

//Constructor of CelestialBody class
CelestialBody::CelestialBody(	bonobo::mesh_data const & shape,
								GLuint const * program,
								GLuint diffuse_texture_id):

	//Setting default values for private attributes
	//vectors
	_scale(1), //passing one argument to a vector makes all values the same, in this case (1,1,1)
	//floats 
	_spinning_speed(0),
	_spin_angle(0),
	_orbit_inclination(0),
	_orbit_speed(0),
	_orbit_radius(0),
	_orbit_angle(0)
{

	//Forward the shape
	_node.set_geometry(shape);

	//Forward the shader program
	_node.set_program(program, [](GLuint /*program*/ ) {} );

	//Forward the diffuse texture
	_node.add_texture("diffuse_texture", diffuse_texture_id, GL_TEXTURE_2D);
	
};

//Render function
void CelestialBody::render(	float ellapsed_time,
							glm::mat4 const & view_projection,
							glm::mat4 const & parent_transform
)
{
	//Calculate the new spin angle if rotation is enabled
	// ellapsed time * _spinning_speed <=> s * rad/s
	// += incrementing the value on the left be the value to the right
	_spin_angle += ellapsed_time * _spinning_speed;

	//Calculate the new orbit angle 
	_orbit_angle += ellapsed_time * _orbit_speed;

	//Compute the scaled matrix
	glm::mat4 scaled_matrix;
	scaled_matrix = glm::scale(glm::mat4(1.0f), _scale);

	//Compute the spin matrix
	//"The axis for the spin matrix should be the y-axis"
	glm::mat4 spin_matrix;
	spin_matrix = glm::rotate(glm::mat4(1.0f), _spin_angle, glm::vec3(0, 1, 0));

	//Compute the orbit matrix
	glm::mat4 orbit_matrix;
	orbit_matrix = glm::rotate(glm::mat4(1.0f), _orbit_angle, glm::vec3(0, 1, 0));

	//Compute the translation matrix
	glm::mat4 translation_matrix = glm::mat4(1.0f);
	translation_matrix[3][0] = _orbit_radius; //glm defaults to [col][row]

	//Compute the title matrix
	//"The matrix is a a roatiation of angle _spinning_inclination around the z-axis"
	glm::mat4 orbit_tilt_matrix;
	orbit_tilt_matrix = glm::rotate(glm::mat4(1.0f), _orbit_inclination, glm::vec3(0,0,1));

	//Compute the spinningtilt_matrix 
	//"The matrix is a a roatiation of angle _spinning_inclination around the z-axis"
	glm::mat4 spinning_tilt_matrix;
	spinning_tilt_matrix = glm::rotate(glm::mat4(1.0f), _spinning_inclination, glm::vec3(0, 0, 1));

	//Combine the matrices
	//"The most right matrix gets applied first" 
	//"Apply the tilt matrix AFTER the spin/rotation matrix"
	glm::mat4 matrix;
	matrix = orbit_tilt_matrix * orbit_matrix * translation_matrix * spinning_tilt_matrix * spin_matrix  * scaled_matrix;
	
	//Call the render function and forward the two matrices
	_node.render(view_projection, matrix);

}

void CelestialBody::set_scale(glm::vec3 const& scale)
{
	_scale = scale;
}

void CelestialBody::set_spinning(	float spinning_inclination,
									float spinning_speed,
									float initial_spin_angle)
{
	_spinning_inclination	= spinning_inclination;
	_spinning_speed			= spinning_speed;
	_spin_angle				= initial_spin_angle;
}

void CelestialBody::set_orbit(	float orbit_inclination,
								float orbit_speed,
								float orbit_radius,
								float initial_orbit_angle)
{
	_orbit_inclination	= orbit_inclination;
	_orbit_speed		= orbit_speed;
	_orbit_radius		= orbit_radius;
	_orbit_angle		= initial_orbit_angle;
}


