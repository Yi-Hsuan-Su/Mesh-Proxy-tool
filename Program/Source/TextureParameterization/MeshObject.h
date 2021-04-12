#pragma once


#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>
#include <Common.h>
#include <string>

typedef OpenMesh::TriMesh_ArrayKernelT<>  TriMesh;

class MyMesh : public TriMesh
{
public:
	MyMesh();
	~MyMesh();

	int FindVertex(MyMesh::Point pointToFind);
	void ClearMesh();
};

class GLMesh
{
public:
	GLMesh();
	~GLMesh();

	bool Init(std::string fileName);
	void Render();

	MyMesh mesh;
	GLuint vao;
	GLuint ebo;
	GLuint vboVertices, vboNormal;

	bool LoadModel(std::string fileName);
	void LoadToShader();
};

class facedata 
{
public:
	std::vector<OpenMesh::Vec3f> verteices;
	std::vector <std::vector<int>> face;
	std::vector <OpenMesh::Vec3f>normal;
};

class MeshObject
{
public:
	MeshObject();
	~MeshObject();
	std::string openfile();
	bool Init(std::string fileName);
	void Render();
	void RenderSelectedFace();
	bool AddSelectedFace(unsigned int faceID);
	void DeleteSelectedFace(unsigned int faceID);
	MyMesh::Point  findcloseinfd(MyMesh::Point p);
	bool findexistp(MyMesh::Point p);
	int  findcloseid(MyMesh::Point p);
	void undoadd();
	bool AddSelectedPoint(unsigned int PointID);
	void DeleteSelectedPoint(unsigned int PointID);

	bool FindClosestPoint(unsigned int faceID, glm::vec3 worldPos, glm::vec3& closestPos );
	void sw();
	void fcoffsetsw();
	bool FaceToPoint();
	bool addfacecheck = false;

	GLMesh model;
	std::vector<unsigned int> selectedFace;
	std::string path;
	facedata fd;
	int fdidx=0;
	std::vector<unsigned int> selectedPoint;
	std::vector < OpenMesh::VertexHandle > pointque;
	std::vector<OpenMesh::Vec3f>vertexqueue;
	std::vector<int> selectPoint_Seq;
	void MY_LoadToShader();
	bool selectsw = true;
	bool faceoffsetsw = false;
	int swcount=0;
	int faceoffsetcount=0;
	std::vector<unsigned int*> fvIDsPtr;
	std::vector<int> elemCount;

};

