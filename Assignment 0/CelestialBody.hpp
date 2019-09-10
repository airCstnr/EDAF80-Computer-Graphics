#pragma once

#include "core/helpers.hpp"
#include "core/node.hpp"

/** EDAF 80
 * Raphael Castanier / Niklas Karlsson
 * 2019
 */

class CelestialBody
{
public:
	/**
	 * \param shape contains information about the geometry used to model the celestial body
	 * \param program is the shader program used to render the celestial body
	 * \param diffuse_texture_id is the identifier of the diffuse texture used
	*/
	CelestialBody( bonobo::mesh_data const& shape,
				   GLuint const* program,
				   GLuint diffuse_texture_id );

	/**
	 * \param ellapsed_time is the amount of time between two frames
	 * \param view_projection is the matrix for going from world-space to clipspace
	 * \param parent_transform transforms from the local-space of your parent, to world-space, and defaults to the identity matrix if unspecified
	 */
	void render( float ellapsed_time,
				 glm::mat4 const& view_projection,
				 glm::mat4 const& parent_transform = glm::mat4( 1.0f ) );

private:
	Node _node;
};
