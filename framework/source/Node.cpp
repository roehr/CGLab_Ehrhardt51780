#include "Node.hpp"



Node::Node()
{
}

Node::Node(std::string _name)
{
	name = _name;
}


Node::~Node()
{
}

void Node::setRotationY(float valy)
{
	//default value for rotation
	rotationY = valy;
}

void Node::updateRotationY(float val){
	//rotate local by the given value
	if (timeDependency) {
		localTransform = glm::rotate(localTransform, val*rotationY, glm::vec3{ 0.0f,1.0f,0.0f });
	}
}

Node* Node::getParent()
{
	return parent;
}

void Node::setParent(Node *_parent)
{
	parent = _parent;
	if (parent != nullptr) {
		depth = parent->getDepth() + 1;
		path = parent->getPath() + "-" + name;
	}
	else {
		depth = 0;
		path = name;
	}
}

bool Node::useFrameTime()
{
	return timeDependency;
}

void Node::changeTimeDependency()
{
	//Enable or disable the usage of time dependent rotation
	timeDependency = !timeDependency;
}

Node * Node::getChildren(std::string name)
{
	//Search for a Child with this name
	//Note that this function only checks the current list of children, not childrens children 
	//at this point that is intentional because I only want to see if the child is within the direct children of the current Node
	std::vector<Node*>::iterator it;

	for (it = children.begin(); it < children.end(); it++) {
		Node* actnode = *it;
		if( (*actnode).getName()==name){
			//found a child, return it
			return *it;
		}
		//nothing found
		return nullptr;
	}

}

std::vector<Node*> Node::getChildrenList()
{
	return children;
}

std::string Node::getName()
{
	return name;
}

void Node::setName(std::string _name)
{
	name = _name;
	depth = 0;
	path = name;
}

std::string Node::getPath()
{
	if (parent != nullptr) {
		return parent->getPath() + "-" + name;
	}
	return name;
}

int Node::getDepth()
{
	if (parent != nullptr) {
		depth = parent->getDepth() + 1;
	}
	else {
		depth = 0;
	}
	return depth;
}

glm::fmat4 Node::getLocalTransform()
{
	return localTransform;
}

void Node::setLocalTransform(glm::fmat4 _transform)
{
	localTransform = _transform;
}

glm::fmat4 Node::getWorldTransform()
{
	return worldTransform;
}

void Node::setWorldTransform(glm::fmat4 _transform)
{
	worldTransform = _transform;
}

void Node::addChildren(Node *_child)
{
	children.push_back(_child);
}

