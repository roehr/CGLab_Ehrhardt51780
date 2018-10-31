#ifndef SCENEGRAPH_HPP
#define SCENEGRAPH_HPP

#include <iostream>
#include "Node.hpp"
#include "CameraNode.hpp"
//TODO: code cleanups: destructors, consts/reference usage on all classes
class SceneGraph 
{
private:
	void setName(std::string);
	std::string name;
	Node* root;
	void setRoot(Node*);
	SceneGraph();
	~SceneGraph();
	CameraNode* activeCam;
	bool hasActiveCam=false;
public:
	std::string printNode(Node*, std::string&);
	std::string getName();
	Node* getRoot();
	std::string printGraph();
	CameraNode* getActiveCamera();
	void lookUpCameraNode(Node*);
	static SceneGraph& getInstance()
	{
		static SceneGraph  instance;
		return instance;
	}

};

#endif