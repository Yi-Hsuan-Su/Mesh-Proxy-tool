#include "MeshObject.h"
#include <Eigen/Sparse>
#include <map>
#include <algorithm>
#include <commdlg.h>

#define Quad
//#define Harmonic

struct OpenMesh::VertexHandle const OpenMesh::PolyConnectivity::InvalidVertexHandle;

#pragma region MyMesh

MyMesh::MyMesh()
{
	request_vertex_normals();
	request_vertex_status();
	request_face_status();
	request_edge_status();
}

MyMesh::~MyMesh()
{

}

int MyMesh::FindVertex(MyMesh::Point pointToFind)
{
	int idx = -1;
	for (MyMesh::VertexIter v_it = vertices_begin(); v_it != vertices_end(); ++v_it)
	{
		MyMesh::Point p = point(*v_it);
		if (pointToFind == p)
		{
			idx = v_it->idx();
			break;
		}
	}

	return idx;
}

void MyMesh::ClearMesh()
{
	if (!faces_empty())
	{
		for (MyMesh::FaceIter f_it = faces_begin(); f_it != faces_end(); ++f_it)
		{
			delete_face(*f_it, true);
		}

		garbage_collection();
	}
}

#pragma endregion

#pragma region GLMesh

GLMesh::GLMesh()
{

}

GLMesh::~GLMesh()
{

}

bool GLMesh::Init(std::string fileName)
{
	if (fileName.empty()) 
	{
		return false;
	}
	if (LoadModel(fileName))
	{
		LoadToShader();
		return true;
	}
	return false;
}

void GLMesh::Render()
{
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, mesh.n_faces() * 3, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}


bool GLMesh::LoadModel(std::string fileName)
{
	OpenMesh::IO::Options ropt;
	if (OpenMesh::IO::read_mesh(mesh, fileName, ropt))
	{
		if (!ropt.check(OpenMesh::IO::Options::VertexNormal) && mesh.has_vertex_normals())
		{
			mesh.request_face_normals();
			mesh.update_normals();
			mesh.release_face_normals();
		}

		return true;
	}

	return false;
}

void GLMesh::LoadToShader()
{
	std::vector<MyMesh::Point> vertices;
	vertices.reserve(mesh.n_vertices());
	for (MyMesh::VertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it)
	{
		vertices.push_back(mesh.point(*v_it));

		MyMesh::Point p = mesh.point(*v_it);
	}

	std::vector<MyMesh::Normal> normals;
	normals.reserve(mesh.n_vertices());
	for (MyMesh::VertexIter v_it = mesh.vertices_begin(); v_it != mesh.vertices_end(); ++v_it)
	{
		normals.push_back(mesh.normal(*v_it));
	}

	std::vector<unsigned int> indices;
	indices.reserve(mesh.n_faces() * 3);
	for (MyMesh::FaceIter f_it = mesh.faces_begin(); f_it != mesh.faces_end(); ++f_it)
	{
		for (MyMesh::FaceVertexIter fv_it = mesh.fv_iter(*f_it); fv_it.is_valid(); ++fv_it)
		{
			indices.push_back(fv_it->idx());
		}
	}

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vboVertices);
	glBindBuffer(GL_ARRAY_BUFFER, vboVertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(MyMesh::Point) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &vboNormal);
	glBindBuffer(GL_ARRAY_BUFFER, vboNormal);
	glBufferData(GL_ARRAY_BUFFER, sizeof(MyMesh::Normal) * normals.size(), &normals[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * indices.size(), &indices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

#pragma endregion

MeshObject::MeshObject()
{
	
}

MeshObject::~MeshObject()
{
}

std::string MeshObject::openfile()
{
	OPENFILENAME ofn;
	TCHAR szFile[MAX_PATH];

	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "All(*.*)\0*.*\0Model(*.obj)\0Text(*.txt)\0*.TXT\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn))
	{
		OutputDebugString(szFile);
		OutputDebugString("\r\n");
	}

	path = szFile;

	return path;
}

bool MeshObject::Init(std::string fileName)
{
	selectedFace.clear();

	return model.Init(fileName);
}

void MeshObject::Render()
{
	glBindVertexArray(model.vao);
	glDrawElements(GL_TRIANGLES, model.mesh.n_faces() * 3, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void MeshObject::RenderSelectedFace()
{
	if (selectedFace.size() > 0)
	{
		std::vector<unsigned int*> offsets(selectedFace.size());
		for (int i = 0; i < offsets.size(); ++i)
		{
			offsets[i] = (GLuint*)(selectedFace[i] * 3 * sizeof(GLuint));
		}

		std::vector<int> count(selectedFace.size(), 3);

		glBindVertexArray(model.vao);
		glMultiDrawElements(GL_TRIANGLES, &count[0], GL_UNSIGNED_INT, (const GLvoid **)&offsets[0], selectedFace.size());
		glBindVertexArray(0);
	}
}

bool MeshObject::AddSelectedFace(unsigned int faceID)
{
	if (std::find(selectedFace.begin(), selectedFace.end(), faceID) == selectedFace.end() &&
		faceID >= 0 && faceID < model.mesh.n_faces())
	{
		selectedFace.push_back(faceID);
		return true;
	}
	return false;
}

void MeshObject::DeleteSelectedFace(unsigned int faceID)
{
	selectedFace.erase(std::remove(selectedFace.begin(), selectedFace.end(), faceID), selectedFace.end());
}

bool MeshObject::AddSelectedPoint(unsigned int PointID)
{
	if (std::find(selectedPoint.begin(), selectedPoint.end(), PointID) == selectedPoint.end() &&
		PointID >= 0 && PointID < model.mesh.n_vertices())
	{
		selectedPoint.push_back(PointID);
		return true;
	}
	return false;
}

void MeshObject::DeleteSelectedPoint(unsigned int PointID)
{
	selectedPoint.erase(std::remove(selectedPoint.begin(), selectedPoint.end(), PointID), selectedPoint.end());
}

void MeshObject::undoadd() 
{
	if (vertexqueue.size() > 0)
	{
		vertexqueue.pop_back();
		std::cout << "queue size " << vertexqueue.size() << std::endl;
	}
}

bool findqueue(OpenMesh::VertexHandle cur , std::vector< OpenMesh::VertexHandle> clvh)
{
	for (int i = 0; i < clvh.size(); i++) 
	{
		if(clvh[i] == cur)
		{
			return true;
		}
	}
	return false;
}
bool MeshObject::findexistp(MyMesh::Point p) 
{
	for (int i = 0; i < fd.verteices.size(); i++)
	{
		if ((p -fd.verteices[i]).norm() <0.1)
		{
			return true;
		}
	}
	return false;
}
MyMesh::Point  MeshObject::findcloseinfd(MyMesh::Point p)
{
	for (int i = 0; i < fd.verteices.size(); i++) 
	{
		if ((p - fd.verteices[i]).norm() < 0.3) 
		{
			return fd.verteices[i];
		}
	}

	if (vertexqueue.size() > 0) 
	{
		if (abs((p[0] - vertexqueue[0][0]))<0.1) 
		{
			std::cout<<"x - x  " << abs((p[0] - vertexqueue[0][0])) << std::endl;
			p[0] = vertexqueue[0][0];
		}
		if (abs((p[1] - vertexqueue[0][1])) < 0.1) 
		{
			std::cout << "y- y  " << abs((p[1] - vertexqueue[0][1])) << std::endl;
			p[1] = vertexqueue[0][1];
		}
		if (abs((p[2] - vertexqueue[0][2])) < 0.1)
		{
			std::cout << "z - z  " << abs((p[2] - vertexqueue[0][2])) << std::endl;
			p[2] = vertexqueue[0][2];
		}
	}
	return p;
}

int  MeshObject::findcloseid(MyMesh::Point p)
{
	for (int i = 0; i < fd.verteices.size(); i++)
	{
		if ((p - fd.verteices[i]).norm() < 0.1)
		{
			return i;
		}
	}
	return -1;
}

bool MeshObject::FindClosestPoint(unsigned int faceID, glm::vec3 worldPos, glm::vec3& closestPos )
{
	OpenMesh::FaceHandle fh = model.mesh.face_handle(faceID);
	if (!fh.is_valid())
	{
		return false;
	}
	
	double minDistance = 0.0;
	MyMesh::Point p(worldPos.x, worldPos.y, worldPos.z);
	MyMesh::FVIter fv_it = model.mesh.fv_iter(fh);
	MyMesh::VertexHandle closestVH = *fv_it;
	MyMesh::Point v1 = model.mesh.point(*fv_it);
	++fv_it;

	minDistance = (p - v1).norm();
	for (; fv_it.is_valid(); ++fv_it)
	{
		MyMesh::Point v = model.mesh.point(*fv_it);
		double distance = (p - v).norm();
		if (minDistance > distance)
		{
			minDistance = distance;
			closestVH = *fv_it;
		}
	}
	if (vertexqueue.size() < 4)
	{

		if (selectsw == true)
		{

			if (!findqueue(closestVH, pointque))
			{
				vertexqueue.push_back(model.mesh.point(closestVH));
				std::cout << "queue size " << vertexqueue.size() << std::endl;
			}
		}
		else
		{

			vertexqueue.push_back(findcloseinfd(p));
			std::cout << "queue size " << vertexqueue.size() << std::endl;

			closestPos.x = p[0];
			closestPos.y = p[1];
			closestPos.z = p[2];
		}
	}
	//std::cout << "queue size " << clvh.size() << std::endl;

	MyMesh::Point closestPoint = model.mesh.point(closestVH);
	//closestPos.x = closestPoint[0];
	//closestPos.y = closestPoint[1];
	//closestPos.z = closestPoint[2];

	return true;
}

bool MeshObject::FaceToPoint()
{
	for (int i = 0; i < selectedFace.size(); i++)
	{
		OpenMesh::FaceHandle fh = model.mesh.face_handle(selectedFace[i]);
		if (!fh.is_valid())
		{
			return false;
		}
		
		MyMesh::FVIter fv_it = model.mesh.fv_iter(fh);
		MyMesh::VertexHandle VH = *fv_it;
		for (; fv_it.is_valid(); ++fv_it)
		{
			VH = *fv_it;
			AddSelectedPoint(VH.idx());
			selectPoint_Seq.push_back(VH.idx());
			//selectedPoint.push_back(VH.idx());
		}
	}

	return true;
}

void MeshObject::fcoffsetsw() 
{
	faceoffsetcount+= 1;
	if (faceoffsetcount % 2 == 1)
	{
		faceoffsetsw = true;
		faceoffsetcount = 1;
	}
	else
	{
		faceoffsetsw = false;
		faceoffsetcount = 0;
	}
}

void MeshObject::sw() 
{
	swcount += 1;
	if (swcount % 2 == 1) 
	{
		selectsw = false;
		swcount = 1;
	}
	else 
	{
		selectsw = true;
		swcount = 0;
	}
}

void MeshObject::MY_LoadToShader()
{
	model.LoadToShader();
}