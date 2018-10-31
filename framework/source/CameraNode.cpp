#include "CameraNode.hpp"



bool CameraNode::getPerspective()
{
	return isPerspective;
}

bool CameraNode::getEnabled()
{
	return isEnabled;
}

void CameraNode::setEnabled(bool val)
{
	isEnabled = val;
}

glm::fmat4 CameraNode::getProjectionMatrix()
{
	return projectionMatrix;
}

void CameraNode::setProjectionMatrix(glm::fmat4 newMatrix)
{
	projectionMatrix = newMatrix;
}

CameraNode::CameraNode()
{
}

CameraNode::CameraNode(std::string _name)
{ 
	Node::setName(_name);
}


CameraNode::~CameraNode()
{
}
