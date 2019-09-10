#include "CelestialBody.hpp"

CelestialBody::CelestialBody( bonobo::mesh_data const & shape,
							  GLuint const * program,
							  GLuint diffuse_texture_id )
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
	// Node::render()
	_node.render( view_projection, parent_transform );

	// ellapsed_time : unused for now

	return;
}
