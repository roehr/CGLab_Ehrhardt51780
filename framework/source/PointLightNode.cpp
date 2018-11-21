#include "..\include\PointLightNode.hpp"





glm::fvec3 PointLightNode::getLightColor()
{
	return lightColor;
}

void PointLightNode::setLightColor(glm::fvec3 lightcol)
{
	lightColor = lightcol;
}

glm::fvec3 PointLightNode::getLightPosition()
{
	return lightPosition;
}

void PointLightNode::setLightPosition(glm::fvec3 pos)
{
	lightPosition = pos;
}

float PointLightNode::getLightIntensity()
{
	return lightIntensity;
}

void PointLightNode::setLightIntensity(float lightint)
{
	lightIntensity = lightint;
}

PointLightNode::PointLightNode(std::string _name)
{
	name = _name;
}

PointLightNode::~PointLightNode()
{
}

std::string PointLightNode::getType()
{
	return "PointLight";
}
