#include "CelestialBody.hpp"

CelestialBody::CelestialBody( bonobo::mesh_data const & shape,
							  GLuint const * program,
							  GLuint diffuse_texture_id ) :
	_scale( 1 ),
	_spinning_axis(	0, 1, 0 ),
	_spinning_speed(0),
	_spin_angle(0)
{
	// Node::set_geometry(bonobo::mesh_data const& shape)
	_node.set_geometry( shape );

	// Node::set_program( GLuint const* const program, std::function<void( GLuint )> const& set_uniforms )
	// Note : we just sent empty lambda function as second parameter
	_node.set_program( program, []( GLuint /*program*/ ) {} );

	// Node::add_texture(std::string const& name, GLuint tex_id, GLenum type)
	_node.add_texture( "diffuse_texture", diffuse_texture_id, GL_TEXTURE_2D );
}

void CelestialBody::render( float ellapsed_time,
							glm::mat4 const & view_projection,
							glm::mat4 const & parent_transform )
{
	// ompute how much the celestial body rotated since last frame
	_spin_angle += ellapsed_time * _spinning_speed;

	// compute the spin matrix and use it for the rendering.
	// identity matrix as first argument
	glm::mat4 spinning_matrix = glm::rotate( glm::mat4( 1.0f ), _spin_angle, _spinning_axis );

	// scaling matrix
	// first argument should be an identity matrix
	glm::mat4 scaling_matrix = glm::scale( glm::mat4( 1.0f ), _scale );

	// Node::render()
	_node.render( view_projection, spinning_matrix * scaling_matrix);
	
	return;
}

void CelestialBody::set_scale( glm::vec3 const & scale )
{
	_scale = scale;
}

void CelestialBody::set_spinning( glm::vec3 const & spinning_axis, float spinning_speed, float initial_spin_angle )
{
	_spinning_axis = spinning_axis;
	_spinning_speed = spinning_speed;
	_spin_angle = initial_spin_angle;
}
