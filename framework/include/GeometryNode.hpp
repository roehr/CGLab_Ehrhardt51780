#ifndef GEOMETRY_NODE_HPP
#define GEOMETRY_NODE_HPP
#include "model.hpp"
#include "Node.hpp"
class GeometryNode : public Node
{
private:
	model geometry;
	bool isStar = false;
	bool isOrbit = false;
public:

	bool useStar();
	bool useOrbit();
	void setStar(bool);
	void setOrbit(bool);
	GeometryNode();
	GeometryNode(std::string);
	~GeometryNode();
	model getModel();
	void setModel(model);
	std::string getType();
};

#endif