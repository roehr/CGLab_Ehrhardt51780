#ifndef NODE_HPP
#define NODE_HPP
#include <vector>
#include <iterator>
#include <glm/gtc/matrix_transform.hpp>
class Node
{
protected:
	Node* parent;
	std::vector<Node*> children;
	std::string name;
	std::string path;
	int depth;
	float rotationY=1.0f;
	glm::fmat4  localTransform;
	glm::fmat4  worldTransform;
	bool timeDependency=false;

public:
	//used for rotation updates around the yaxis
	//more of a convenient function, as this could also be achieved using the setLocalTransform()
	bool useFrameTime();
	void setRotationY(float);
	void updateRotationY(float);
	void changeTimeDependency();

	Node* getParent();
	void setParent(Node*);
	Node* getChildren(std::string);
	//using vector here in case we want to access a child by position
	std::vector<Node*> getChildrenList();
	std::string getName();
	void setName(std::string);
	std::string getPath();
	int getDepth();
	glm::fmat4  getLocalTransform();
	void setLocalTransform(glm::fmat4);
	glm::fmat4  getWorldTransform();
	void setWorldTransform(glm::fmat4);
	void addChildren(Node*);
	//Helper to determine different Types quickly - dirty solution but works for now
	virtual std::string getType() { return "Node"; }

	Node();
	Node(std::string);
	~Node();
};

#endif
