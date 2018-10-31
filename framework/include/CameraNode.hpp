#ifndef CAMERANODE_HPP
#define CAMERANODE_HPP
#include"Node.hpp"


class CameraNode : public Node
{
private:
	bool isPerspective;
	bool isEnabled;
	glm::fmat4  projectionMatrix;
public:
	bool getPerspective();
	bool getEnabled();
	void setEnabled(bool);
	glm::fmat4 getProjectionMatrix();
	void setProjectionMatrix(glm::fmat4);
	std::string getType() {return "Camera"; }

	CameraNode();
	CameraNode(std::string);
	~CameraNode();
};
#endif 