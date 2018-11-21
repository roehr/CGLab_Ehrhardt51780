#ifndef POINTLIGHTNODE_HPP
#define POINTLIGHTNODE_HPP
#include "Node.hpp"
class PointLightNode : public Node
{
private:
	glm::fvec3 lightColor;
	float lightIntensity;
	glm::fvec3 lightPosition;

public:
	glm::fvec3 getLightColor();
	void setLightColor(glm::fvec3);
	glm::fvec3 getLightPosition();
	void setLightPosition(glm::fvec3);
	float getLightIntensity();
	void setLightIntensity(float);
	PointLightNode(std::string);
	~PointLightNode();

	std::string getType();
};
#endif 


