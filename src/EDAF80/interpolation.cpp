#include "interpolation.hpp"

glm::vec3
interpolation::evalLERP(glm::vec3 const& p0, glm::vec3 const& p1, float const x)
{
	glm::vec2 vec2(1,x);

	glm::mat2 mat2x2(	glm::vec2(1,-1),
						glm::vec2(0,1)); //[col],[row]

	glm::mat3x2 mat3x2(	glm::vec2(p0[0], p1[0]),
						glm::vec2(p0[1], p1[1]),
						glm::vec2(p0[2], p1[2])); //[col][row]

	glm::vec3 px = vec2 * mat2x2 * mat3x2;

	return px;

}

glm::vec3
interpolation::evalCatmullRom(glm::vec3 const& p0, glm::vec3 const& p1,
                              glm::vec3 const& p2, glm::vec3 const& p3,
                              float const t, float const x)
{

	glm::vec4 vec4(1, x, pow(x, 2), pow(x, 3));

	glm::mat4 mat4(	glm::vec4(0, -t, 2 * t, -t),
					glm::vec4(1, 0, t - 3, 2 - t),
					glm::vec4(0, t, 3 - 2 * t, t - 2),
					glm::vec4(0, 0, -t, t)); //[col][row]

	glm::mat3x4 mat3x4(	glm::vec4(p0[0], p1[0], p2[0], p3[0]),
						glm::vec4(p0[1], p1[1], p2[1], p3[1]),
						glm::vec4(p0[2], p1[2], p2[2], p3[2])); //[col][row]

	glm::vec3 qx = vec4 * mat4 * mat3x4;

	return qx;
}
