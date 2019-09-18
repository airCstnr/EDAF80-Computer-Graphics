#include "core/helpers.hpp"
#include "core/node.hpp"
#include "glm/vec3.hpp"

/*
	EDAF 80 Fall 2019
	Raphael Castanier
	Niklas Karlsson
*/

class CelestialBody
{
public:

	/*
	 Shape contains information about the geometry used to model the celestial body
	 Program is the shader program used to render the celestial body
	 diffuse_texture_id is the identifier of the diffuse texture used
	*/

	CelestialBody(	bonobo::mesh_data const& shape,
					GLuint const* program,
					GLuint diffuse_texture_id
	);

	/*
	ellapsed_time is the amount of time between two frames
	view_projection is the matrix for going from world-space to clip-space
	parent_transform tranforms from from the localspace of your parent, to world-space and defaults to the identity matrix if unspecified
	*/
	void render(	float ellapsed_time,
					glm::mat4 const& view_projection,
					glm::mat4 const& parent_transform = glm::mat4(1.0f)
	);

	void set_scale(	glm::vec3 const& scale);

	void set_spinning(	float spinning_inclination,
						float spinning_speed,
						float initial_spin_angle = 0.0f);
	/*
	The orbit inclination is relative to the Sun's equator
	*/
	void set_orbit(	float orbit_inclination,
					float orbit_speed,
					float orbit_radius,
					float initial_orbit_angle = 0.0f);
	/*
	Shape = the shape to use for the rings
	scaling = how to scale the ring along the x- and y-axis (assuming you are looking at it from the top)
	program = the ID of the shader program used to render the rings
	diffuse_texture_id = the ID of the diffuse texture
	opacity texture_id = the ID of the opacity texture
	*/
	void add_rings(	bonobo::mesh_data const& shape,
					glm::vec2 const& scaling,
					GLuint const* program,
					GLuint diffuse_texture_id,
					GLuint opacity_texture_id);

	void add_child(CelestialBody* child);

	std::vector<CelestialBody*> const& get_children() const;

	glm::mat4 get_transform() const;

private:

	//Nodes
	Node _node;
	Node _rings_node;

	//Vectors
	glm::vec3 _scale;

	//Matrices
	glm::mat4 _transform_matrix;

	//Float
	float _spinning_inclination;
	float _spinning_speed; //radians/s
	float _spin_angle; //radians
	float _orbit_inclination;
	float _orbit_speed;
	float _orbit_radius;
	float _orbit_angle;

	std::vector<CelestialBody*> _children_nodes;
};
