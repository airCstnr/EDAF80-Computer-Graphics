#pragma once

#include "core/helpers.hpp"
#include "core/node.hpp"
#include "glm/vec3.hpp"

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

	/** Will scale the geometry( to atten the earth at the poles, for example ), and store the value as a new attribute */
	void set_scale( glm::vec3 const& scale );

	/**  
	 * \param spinning_axis
	 * \param spinning_axis rad/sec
	 * \param initial_spin_angle rad
	 */
	void set_spinning( glm::vec3 const& spinning_axis,
					   float spinning_speed,
					   float initial_spin_angle = 0.0f );

private:
	Node _node;
	glm::vec3 _scale;
	glm::vec3 _spinning_axis;
	float _spinning_speed; // radians/sec
	float _spin_angle; // radians
};
