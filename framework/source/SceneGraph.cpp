#include "SceneGraph.hpp"


void SceneGraph::setName(std::string _name)
{
	name = _name;
}

void SceneGraph::setRoot(Node* _node)
{
	root = _node;
}

SceneGraph::SceneGraph()
{
	root = new Node("root");
	root->setParent(nullptr);
	root->setWorldTransform(glm::fmat4{});
	root->setLocalTransform(glm::fmat4{});

	name = "Scene";
}


SceneGraph::~SceneGraph()
{
}

std::string SceneGraph::getName()
{
	return name;
}

Node * SceneGraph::getRoot()
{
	return root;
}

std::string SceneGraph::printGraph()
{
	std::string result = name + ":\n";
	result=printNode(root,result);

	return result;
}
std::string SceneGraph::printNode(Node* currNode, std::string &res)
{
	res = res + currNode->getPath() + "\n";
	if (!currNode->getChildrenList().empty()) {
		std::vector<Node*>::iterator it;
		std::vector<Node*>children = currNode->getChildrenList();
		for(it = children.begin(); it != children.end(); it++) {
			printNode(*it, res);
		}
	}
	return res;
}

CameraNode * SceneGraph::getActiveCamera()
{
	//Needs to be adjusted for multiple Cam usage - For now only look up scene graph for cam once and set the 
	//result as active cam - if a cam is already set, skip that part
	// maybe direct setting of the cam should be supported as well to avoid the whole traversal
	if (activeCam == nullptr) {
		lookUpCameraNode(root);
	}
	//if no cam is found: setup a default cam 
	if (activeCam == nullptr) {
		activeCam = new CameraNode("Camera");
		activeCam->setParent(root);
		root->addChildren(activeCam);
		activeCam->setEnabled(true);
		//that projection will most likely look weird, but that's fine since it gives a visual feedback, that the cam was 
		//not set correctly in the SceneGraph. Another option would be to throw an error at this point
		activeCam->setProjectionMatrix(glm::fmat4{});
		activeCam->setLocalTransform(glm::fmat4{});
		activeCam->setWorldTransform(glm::fmat4{});
	}

	return activeCam;
}

void SceneGraph::lookUpCameraNode(Node* currNode)
{

	if (currNode->getType() == "Camera") {
		//check if that Camera is enabled
		CameraNode* cam = static_cast<CameraNode*>(currNode);
		//Note that if multiple Cameras are set to enabled, only the first hit will be used
		
		if (cam->getEnabled()) {
			activeCam=cam;
			hasActiveCam= true;

		}
	}
	if (activeCam == nullptr) {
		std::vector<Node*>::iterator it;
		std::vector<Node*>children = currNode->getChildrenList();
		for (it = children.begin(); it != children.end(); it++) {
			lookUpCameraNode(*it);
		}
	}
	
}
