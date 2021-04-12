#include <Common.h>
#include <ViewManager.h>
#include <AntTweakBar/AntTweakBar.h>
#include <ResourcePath.h>

#include <Eigen/Sparse>

#include "OpenMesh.h"
#include "MeshObject.h"
#include "DrawModelShader.h"
#include "PickingShader.h"
#include "PickingTexture.h"
#include "DrawPickingFaceShader.h"
#include "DrawTextureShader.h"
#include "DrawPointShader.h"
#include"DrawLineshader.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../../Include/STB/stb_image_write.h"

using namespace glm;
using namespace std;
using namespace Eigen;

glm::vec3 worldPos;
bool updateFlag = false;
bool isRightButtonPress = false;
GLuint currentFaceID = 0;
int currentMouseX = 0;
int currentMouseY = 0;
int windowWidth = 1200;
int windowHeight = 600;
const std::string ProjectName = "TextureParameterization";
float noraml_length=1.0;
float model_falpha=0.5;
bool isdraw_model = true;
bool isdraw_normal = true;
bool isdraw_polygonmode =false;
GLuint			program;			// shader program
mat4			proj_matrix;		// projection matrix
GLuint pBuffer;
GLuint pVBuffer;
float			aspect;
ViewManager		meshWindowCam;

MeshObject model;
MeshObject BeSelectModel;

vector<unsigned int> OuterPoint;
vector<unsigned int> InnerPoint;

float OuterLengh = 0;



// shaders
DrawModelShader drawModelShader;
DrawPickingFaceShader drawPickingFaceShader;
PickingShader pickingShader;
PickingTexture pickingTexture;
DrawPointShader drawPointShader;
DrawLineshader drawlineshader;
// vbo for drawing point
GLuint vboPoint;
GLuint vertexbuffer;
int mainWindow;
enum SelectionMode
{
	ADD_FACE,
	DEL_FACE,
	SELECT_POINT,
	ADD_POINT,
	DEL_POINT,
	ONE_RING

};
SelectionMode selectionMode = ADD_POINT;

TwBar* bar;
TwEnumVal SelectionModeEV[] = { {ADD_FACE, "Add face"}, {DEL_FACE, "Delete face"}, {SELECT_POINT, "Point"},{ADD_POINT,"Add point"},{DEL_POINT,"Delete point"},{ONE_RING,"One Ring"} };
TwType SelectionModeType;

void TW_CALL savecb(void* clientData);
void TW_CALL loadcb(void* clientData);
void SetupGUI()
{
#ifdef _MSC_VER
	TwInit(TW_OPENGL, NULL);
#else
	TwInit(TW_OPENGL_CORE, NULL);
#endif
	TwGLUTModifiersFunc(glutGetModifiers);
	bar = TwNewBar("Texture Parameter Setting");
	TwDefine(" 'Texture Parameter Setting' size='220 180' ");
	TwDefine(" 'Texture Parameter Setting' fontsize='3' color='96 216 224'");

	// Defining season enum type
	SelectionModeType = TwDefineEnum("SelectionModeType", SelectionModeEV, 5);
	// Adding season to bar
	TwAddVarRW(bar, "SelectionMode", SelectionModeType, &selectionMode, NULL);
	TwAddVarRW(bar, "Normal Lengeth", TW_TYPE_FLOAT, &noraml_length,
		" min=0.1 max=1 step=0.1  help='Noraml Length (10=original size).' ");
	TwAddVarRW(bar, "Face alpha", TW_TYPE_FLOAT, &model_falpha,
		" min=0.1 max=1 step=0.1  help='Face Alpha (10=original size).' ");
	TwAddVarRW(bar, "Render Model ",TW_TYPE_BOOLCPP, &isdraw_model, " true = 1 false = 0");
	TwAddVarRW(bar, "Render Normal ", TW_TYPE_BOOLCPP, &isdraw_normal, " true = 1 false = 0");
	TwAddVarRW(bar, "Render Proxy Face ", TW_TYPE_BOOLCPP, &isdraw_polygonmode, " true = 1 false = 0");
	TwAddButton(bar, "Load", loadcb, NULL, " label = 'Load Model'");
	TwAddButton(bar, "Save", savecb, NULL," label = 'Save Model'");

}

void My_LoadModel()
{
	if (model.Init(model.openfile()))
	{
		model.vertexqueue.clear();
		model.fd.face.clear();
		model.fd.verteices.clear();
		/*int id = 0;
		while (model.AddSelectedFace(id))
		{
			++id;
		}
		model.Parameterization();
		drawTexture = true;*/

		puts("Load Model");
	}
	else
	{
		puts("Load Model Failed");
	}
}

void SaveModel() 
{
	string s = "D:\\OpenMeshUV/ouptput/";
	string fn = "group_" + to_string(1) + ".obj";
	string path = s + fn;
	ofstream fout(path);

	for (int i = 0; i <model.fd.verteices.size(); i++)
	{
			fout << "v " << model.fd.verteices[i] << std::endl;
	}
	fout << std::endl;
	for (int i = 0; i < model.fd.face.size(); i++)
	{
		fout << "f ";
		for (int j = 0; j < 4; j++)
		{
			fout << " " << model.fd.face[i][j] + 1;
		}
		fout << std::endl;
	}
	fout.close();
}

void TW_CALL savecb(void* clientData)
{
	SaveModel();
	// do something
}

void TW_CALL loadcb(void* clientData)
{
	My_LoadModel();
	// do something
}

void InitOpenGL()
{
	glEnable(GL_TEXTURE_2D);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_POINT_SMOOTH);
}

void InitData()
{
	ResourcePath::shaderPath = "./Shader/" + ProjectName + "/";
	ResourcePath::imagePath = "./Imgs/" + ProjectName + "/";
	ResourcePath::modelPath = "./Model/UnionSphere.obj";

	//Initialize shaders
	///////////////////////////	
	drawModelShader.Init();
	pickingShader.Init();
	pickingTexture.Init(windowWidth, windowHeight);
	drawPickingFaceShader.Init();
	drawPointShader.Init();
	drawlineshader.Init();
	glGenBuffers(1, &vboPoint);

	//Load model to shader program
	My_LoadModel();
}

void Reshape(int width, int height)
{
	windowWidth = width;
	windowHeight = height;

	TwWindowSize(width, height);
	glutReshapeWindow(windowWidth, windowHeight);
	glViewport(0, 0, windowWidth, windowHeight);

	aspect = windowWidth * 1.0f / windowHeight;
	meshWindowCam.SetWindowSize(windowWidth, windowHeight);
	pickingTexture.Init(windowWidth, windowHeight);
}


void fcoffset(bool sw)
{
	OpenMesh::Vec3f a, b, norm;

	a = model.vertexqueue[2] - model.vertexqueue[1];
	b = model.vertexqueue[0] - model.vertexqueue[1];
	norm = OpenMesh::cross(a, b);
	norm = norm.normalize();

	for (int i = 0; i < model.vertexqueue.size(); i++)
	{
		if (sw == true)
		{
			if (model.findcloseid(model.vertexqueue[i]) != -1)
			{
				 model.fd.verteices[model.findcloseid(model.vertexqueue[i])] += norm * 0.01;
			}
				model.vertexqueue[i] += norm * 0.01;			
		}
		else 
		{
			if (model.findcloseid(model.vertexqueue[i]) != -1)
			{
				model.fd.verteices[model.findcloseid(model.vertexqueue[i])] +=-1* norm * 0.01;
			}
			model.vertexqueue[i] +=-1* norm * 0.01;
		}
	}
}

void calnormal(std::vector<int>tmpface)
{
	OpenMesh::Vec3f a, b ,norm;

	a = model.fd.verteices[tmpface[2]] - model.fd.verteices[tmpface[1]];
	b= model.fd.verteices[tmpface[0]] - model.fd.verteices[tmpface[1]];
	norm = OpenMesh::cross(a, b);
	norm =norm.normalize();
	model.fd.normal.push_back(norm);
}

void addface(std::vector<int> &tmpface)
{
	if (model.vertexqueue.size() == 4)
	{
		model.vertexqueue.clear();
		model.fd.face.push_back(tmpface);
	}
}

void rendercenter(glm::vec3 fc_center , int i )
{
	glm::vec3 endp;
	OpenMesh::Vec3f a, b, norm;

	a = model.fd.verteices[model.fd.face[i][2]] - model.fd.verteices[model.fd.face[i][1]];
	b = model.fd.verteices[model.fd.face[i][0]] - model.fd.verteices[model.fd.face[i][1]];
	norm = OpenMesh::cross(a, b);
	norm = norm.normalize();

	endp = fc_center + noraml_length* glm::vec3(norm[0], norm[1], norm[2]); //glm::vec3(model.fd.normal[i][0], model.fd.normal[i][1] , model.fd.normal[i][2]);
	
	std::vector<glm::vec3> normline;
	normline.push_back(fc_center);
	normline.push_back(endp);
	glGenVertexArrays(1, &pBuffer);
	glBindVertexArray(pBuffer);

	glGenBuffers(1, &pVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, pVBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*normline.size() ,&normline[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);


	drawlineshader.Enable();
	glm::mat4 mvMat = meshWindowCam.GetViewMatrix() * meshWindowCam.GetModelMatrix();
	glm::mat4 pMat = meshWindowCam.GetProjectionMatrix(aspect);
	drawlineshader.SetMVMat(mvMat);
	drawlineshader.SetPMat(pMat);
	drawlineshader.SetPointColor(glm::vec4(0.5,0.0,1.0,0.7));
	drawPointShader.SetPointSize(15.0);
	glLineWidth(15.0);
	glDrawArrays(GL_LINE_STRIP, 0, normline.size());
	drawlineshader.Disable();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void renderline(std::vector<glm::vec3> g_vertex_data , glm::vec4 color)
{
	
	glGenVertexArrays(1, &pBuffer);
	glBindVertexArray(pBuffer);

	glGenBuffers(1, &pVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, pVBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) *g_vertex_data.size() , &g_vertex_data[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	/*
	glGenBuffers(1, &vboNormal);
	glBindBuffer(GL_ARRAY_BUFFER, vboNormal);
	glBufferData(GL_ARRAY_BUFFER, sizeof(MyMesh::Normal) * normals.size(), &normals[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);*/
	/*
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * indices.size(), &indices[0], GL_STATIC_DRAW);
	*/


	//glBindVertexArray(pBuffer);
	drawlineshader.Enable();
	glm::mat4 mvMat = meshWindowCam.GetViewMatrix() * meshWindowCam.GetModelMatrix();
	glm::mat4 pMat = meshWindowCam.GetProjectionMatrix(aspect);
	drawlineshader. SetMVMat(mvMat);
	drawlineshader.SetPMat(pMat);
	drawlineshader.SetPointColor(color);
	drawPointShader.SetPointSize(200.0);
	glLineWidth(20.0);
	glDrawArrays(GL_LINE_STRIP, 0, g_vertex_data.size());
	drawlineshader.Disable();
	glBindBuffer(GL_ARRAY_BUFFER,0);
	glBindVertexArray(0);
	/*
	drawlineshader.Enable();
	drawlineshader.SetPointColor(glm::vec4(1.0, 0.0, 0.0, 0.2));
	drawPointShader.SetPointSize(200.0);
	glDrawArrays(GL_LINE_STRIP , 0, g_vertex_data.size());
	drawlineshader.Disable();
	glBindVertexArray(0);*/
}


void renderpolygon(std::vector<glm::vec3> g_vertex_data, glm::vec4 color)
{

	glGenVertexArrays(1, &pBuffer);
	glBindVertexArray(pBuffer);

	glGenBuffers(1, &pVBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, pVBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * g_vertex_data.size(), &g_vertex_data[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	/*
	glGenBuffers(1, &vboNormal);
	glBindBuffer(GL_ARRAY_BUFFER, vboNormal);
	glBufferData(GL_ARRAY_BUFFER, sizeof(MyMesh::Normal) * normals.size(), &normals[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);*/
	/*
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * indices.size(), &indices[0], GL_STATIC_DRAW);
	*/


	//glBindVertexArray(pBuffer);
	drawlineshader.Enable();
	glm::mat4 mvMat = meshWindowCam.GetViewMatrix() * meshWindowCam.GetModelMatrix();
	glm::mat4 pMat = meshWindowCam.GetProjectionMatrix(aspect);
	drawlineshader.SetMVMat(mvMat);
	drawlineshader.SetPMat(pMat);
	drawlineshader.SetPointColor(color);
	glDrawArrays(GL_POLYGON, 0, g_vertex_data.size());
	drawlineshader.Disable();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	/*
	drawlineshader.Enable();
	drawlineshader.SetPointColor(glm::vec4(1.0, 0.0, 0.0, 0.2));
	drawPointShader.SetPointSize(200.0);
	glDrawArrays(GL_LINE_STRIP , 0, g_vertex_data.size());
	drawlineshader.Disable();
	glBindVertexArray(0);*/
}

// GLUT callback. Called to draw the scene.
void RenderMeshWindow()
{
	//Update shaders' input variable
	///////////////////////////	

	glm::mat4 mvMat = meshWindowCam.GetViewMatrix() * meshWindowCam.GetModelMatrix();
	glm::mat4 pMat = meshWindowCam.GetProjectionMatrix(aspect);

	// write faceID+1 to framebuffer
	pickingTexture.Enable();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	pickingShader.Enable();
	pickingShader.SetMVMat(value_ptr(mvMat));
	pickingShader.SetPMat(value_ptr(pMat));


	model.Render();



	pickingShader.Disable();
	pickingTexture.Disable();


	// draw model
	glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (isdraw_model)
	{
		drawModelShader.Enable();
		glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(mvMat)));

		drawModelShader.SetWireColor(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
		drawModelShader.SetFaceColor(glm::vec4(1.0f, 1.0f, 1.0f, model_falpha));
		drawModelShader.UseLighting(true);
		drawModelShader.DrawWireframe(true);
		drawModelShader.SetNormalMat(normalMat);
		drawModelShader.SetMVMat(mvMat);
		drawModelShader.SetPMat(pMat);

		model.Render();
	}

	if (model.selectedPoint.size() > 0)
	{
		drawModelShader.SetWireColor(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
		drawModelShader.SetFaceColor(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

		BeSelectModel.Render();

	}
	else
	{

	}
	drawModelShader.Disable();


	if (model.selectedPoint.size() > 0)
	{


		glColor3f(0, 0, 1);
		glBegin(GL_LINES);
		glLineWidth(5);

		for (int i = 0; i < (OuterPoint.size() + 1); i++)
		{
			MyMesh::VHandle DrawPoint1 = BeSelectModel.model.mesh.vertex_handle(OuterPoint[i%OuterPoint.size()]);
			MyMesh::VHandle DrawPoint2 = BeSelectModel.model.mesh.vertex_handle(OuterPoint[(i + 1) % OuterPoint.size()]);
			MyMesh::TexCoord2D outPoint1_p = BeSelectModel.model.mesh.texcoord2D(DrawPoint1);
			MyMesh::TexCoord2D outPoint2_p = BeSelectModel.model.mesh.texcoord2D(DrawPoint2);

			glVertex3f(outPoint1_p[0], outPoint1_p[1], 0);
			glVertex3f(outPoint2_p[0], outPoint2_p[1], 0);
		}
		glEnd();



		glColor3f(0, 1, 0);

		glBegin(GL_POINTS);

		glPointSize(20);

		for (int i = 0; i < (OuterPoint.size() + 1); i++)
		{
			MyMesh::VHandle DrawPoint1 = BeSelectModel.model.mesh.vertex_handle(OuterPoint[i%OuterPoint.size()]);
			MyMesh::TexCoord2D outPoint1_p = BeSelectModel.model.mesh.texcoord2D(DrawPoint1);

			glColor3f(0, 1,(float)i/ OuterPoint.size());
			glVertex3f(outPoint1_p[0], outPoint1_p[1], 0);
		}
		glEnd();







	}





	// render selected face
	if (selectionMode == SelectionMode::ADD_FACE || selectionMode == SelectionMode::DEL_FACE || selectionMode == SelectionMode::ADD_POINT)
	{
		drawPickingFaceShader.Enable();
		drawPickingFaceShader.SetMVMat(value_ptr(mvMat));
		drawPickingFaceShader.SetPMat(value_ptr(pMat));
		model.RenderSelectedFace();
		drawPickingFaceShader.Disable();
	}

	glUseProgram(0);

	// render closest point
	if (selectionMode == SelectionMode::SELECT_POINT ) 
	{
		if (updateFlag)
		{
			float depthValue = 0;
			int windowX = currentMouseX;
			int windowY = windowHeight - currentMouseY;
			glReadPixels(windowX, windowY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depthValue);

			GLint _viewport[4];
			glGetIntegerv(GL_VIEWPORT, _viewport);
			glm::vec4 viewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
			glm::vec3 windowPos(windowX, windowY, depthValue);
			glm::vec3 wp = glm::unProject(windowPos, mvMat, pMat, viewport);
			model.FindClosestPoint(currentFaceID - 1, wp, worldPos );//從這裡讀到點
			model.pointque.clear();
			updateFlag = false;
		}


		glBindBuffer(GL_ARRAY_BUFFER, vboPoint);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), glm::value_ptr(worldPos), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(0);

		glm::vec4 pointColor(1.0, 0.0, 1.0, 1.0);
		drawPointShader.Enable();
		drawPointShader.SetMVMat(mvMat);
		drawPointShader.SetPMat(pMat);
		drawPointShader.SetPointColor(pointColor);
		drawPointShader.SetPointSize(15.0);

		glDrawArrays(GL_POINTS, 0, 1);

		drawPointShader.Disable();


		glBindBuffer(GL_ARRAY_BUFFER, 0);

	}

	if (selectionMode == SelectionMode::ADD_POINT)
	{
		if (updateFlag)
		{
			float depthValue = 0;
			int windowX = currentMouseX;
			int windowY = windowHeight - currentMouseY;
			glReadPixels(windowX, windowY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depthValue);

			GLint _viewport[4];
			glGetIntegerv(GL_VIEWPORT, _viewport);
			glm::vec4 viewport(_viewport[0], _viewport[1], _viewport[2], _viewport[3]);
			glm::vec3 windowPos(windowX, windowY, depthValue);
			glm::vec3 wp = glm::unProject(windowPos, mvMat, pMat, viewport);
			model.FindClosestPoint(currentFaceID - 1, wp, worldPos);//從這裡讀到點
			model.pointque.clear();
			updateFlag = false;
		}
		//-----
		if (model.fd.face.size() > 0)
		{
			for (int i = 0; i < model.fd.face.size(); i++)
			{
				for (int j = 0; j < 4; j++)
				{
					if (model.vertexqueue.size() > 3)
					{
						if (model.fd.verteices[model.fd.face[i][j]] != model.vertexqueue[j])
						{
							glm::vec3 tmp;
							tmp.x = model.fd.verteices[model.fd.face[i][j]][0];
							tmp.y = model.fd.verteices[model.fd.face[i][j]][1];
							tmp.z = model.fd.verteices[model.fd.face[i][j]][2];
							//g_vertex_data.push_back(tmp);

							glBindBuffer(GL_ARRAY_BUFFER, vboPoint);
							glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), glm::value_ptr(tmp), GL_STATIC_DRAW);
							glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
							glEnableVertexAttribArray(0);

							glm::vec4 pointColor(0.0, 0.0, 1.0, 1.0);
							drawPointShader.Enable();
							drawPointShader.SetMVMat(mvMat);
							drawPointShader.SetPMat(pMat);
							drawPointShader.SetPointColor(pointColor);
							drawPointShader.SetPointSize(15.0);

							glDrawArrays(GL_POINTS, 0, 1);
						}
					}
					else
					{
						glm::vec3 tmp;
						tmp.x = model.fd.verteices[model.fd.face[i][j]][0];
						tmp.y = model.fd.verteices[model.fd.face[i][j]][1];
						tmp.z = model.fd.verteices[model.fd.face[i][j]][2];
						//g_vertex_data.push_back(tmp);

						glBindBuffer(GL_ARRAY_BUFFER, vboPoint);
						glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), glm::value_ptr(tmp), GL_STATIC_DRAW);
						glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
						glEnableVertexAttribArray(0);

						glm::vec4 pointColor(0.0, 0.0, 1.0, 1.0);
						drawPointShader.Enable();
						drawPointShader.SetMVMat(mvMat);
						drawPointShader.SetPMat(pMat);
						drawPointShader.SetPointColor(pointColor);
						drawPointShader.SetPointSize(15.0);

						glDrawArrays(GL_POINTS, 0, 1);
					}
				}
			}
		}

		//-----


		for (int i = 0; i < model.vertexqueue.size(); i++)
		{
			glm::vec3 tmp;

			tmp.x = model.vertexqueue[i][0];
			tmp.y = model.vertexqueue[i][1];
			tmp.z = model.vertexqueue[i][2];

			//g_vertex_data.push_back(tmp);

			glBindBuffer(GL_ARRAY_BUFFER, vboPoint);
			glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), glm::value_ptr(tmp), GL_STATIC_DRAW);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(0);

			glm::vec4 pointColor(1.0, 0.0, 1.0, 1.0);
			drawPointShader.Enable();
			drawPointShader.SetMVMat(mvMat);
			drawPointShader.SetPMat(pMat);
			drawPointShader.SetPointColor(pointColor);
			drawPointShader.SetPointSize(15.0);

			glDrawArrays(GL_POINTS, 0, 1);
			drawPointShader.Disable();
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			
		}

	}

	if (model.vertexqueue.size() > 1) 
	{
		std::vector<glm::vec3>g_vertex_q;
		for (int i = 0; i < model.vertexqueue.size(); i++) 
		{
			glm::vec3 from = glm::vec3();
			from[0] =model.vertexqueue[i][0];
			from[1] = model.vertexqueue[i][1];
			from[2] = model.vertexqueue[i][2];
			g_vertex_q.push_back(from);

			if (i == 3) 
			{
				glm::vec3 from = glm::vec3();
				from[0] = model.vertexqueue[0][0];
				from[1] = model.vertexqueue[0][1];
				from[2] = model.vertexqueue[0][2];
				g_vertex_q.push_back(from);
			}
		}

			renderline(g_vertex_q, glm::vec4(1.0, 0.0, 0.0, 1));
		
	}


	if (model.addfacecheck == true)
	{
			if (model.vertexqueue.size() == 4) 
			{
				int tmpidx = model.fdidx;
				std::vector<int> tmpface;
				for (int i = 0; i < 4; i++) 
				{
					if (model.findexistp(model.vertexqueue[i]))
					{

					}
					else 
					{
						model.fd.verteices.push_back(model.vertexqueue[i]);
					}
					//std::cout << "size  " << model.fd.verteices.size() << std::endl;
					model.fdidx++;
					tmpface.push_back(model.findcloseid(model.vertexqueue[i]));	
				}

					addface(tmpface);
					calnormal(tmpface);
					model.addfacecheck = false;
			}
		}


	

		/*
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glLineWidth(200.0f);
		glBegin(GL_LINES);
		glColor4f(1, 0, 0,0.2);
		*/

		//點的vector
		std::vector<glm::vec3> g_vertex_data;
		glm::vec3 fc_center ;
		if (model.fd.face.size() > 0)
		{
			for (int i = 0; i < model.fd.face.size(); i++)
			{
				fc_center = vec3(0, 0, 0);
				for (int j = 0; j < 4; j++)
				{
					glm::vec3 from = glm::vec3();
					from[0] = model.fd.verteices[model.fd.face[i][j]][0];
					from[1] = model.fd.verteices[model.fd.face[i][j]][1];
					from[2] = model.fd.verteices[model.fd.face[i][j]][2];
					g_vertex_data.push_back(from);

					fc_center += from;
					if (j == 3)
					{
						glm::vec3 from = glm::vec3();
						from[0] = model.fd.verteices[model.fd.face[i][0]][0];
						from[1] = model.fd.verteices[model.fd.face[i][0]][1];
						from[2] = model.fd.verteices[model.fd.face[i][0]][2];
						g_vertex_data.push_back(from);
					}
				}
				fc_center[0] /= 4;
				fc_center[1] /= 4;
				fc_center[2] /= 4;
				if (isdraw_normal)
				{
					rendercenter(fc_center, i);
				}
			}
			
			if (isdraw_polygonmode) 
			{
				renderpolygon(g_vertex_data, glm::vec4(0.0, 0.5, 0.5, 0.5));
			}
			renderline(g_vertex_data , glm::vec4(0.0,1.0,0.0,1.0));
		}
			//glm::mat4 vm = meshWindowCam.GetModelViewProjectionMatrix(aspect);
			//vm = glm::inverse(vm);




			/*
			for (int i = 0; i < g_vertex_data.size(); i++)
			{
				g_vertex_data[i] =vm * g_vertex_data[i];
				g_vertex_data[i] /= meshWindowCam.Zoom;
			}*/
			

			//理論上應該要畫線的code
		/*
			for (int i = 0; i < g_vertex_data.size(); i++)
			{
				std::cout << "v  " << i << "    " << g_vertex_data[i][0] << "  " << g_vertex_data[i][1] << "  " << g_vertex_data[i][2] << "   " << std::endl;
			}
			*/
			/*
			for (int i = 0; i < g_vertex_data.size(); i++) 
			{
				glVertex3f(g_vertex_data[(i + 1) % g_vertex_data.size()][0], g_vertex_data[(i + 1) % g_vertex_data.size()][1], g_vertex_data[(i + 1) % g_vertex_data.size()][2]);
				glVertex3f(g_vertex_data[(i) % g_vertex_data.size()][0], g_vertex_data[(i) % g_vertex_data.size()][1], g_vertex_data[(i) % g_vertex_data.size()][2]);
			}

		}
		glEnd();*/
	

	TwDraw();
	glutSwapBuffers();
}

void RenderAll()
{
	RenderMeshWindow();
}


//Timer event
void My_Timer(int val)
{
	glutPostRedisplay();
	glutTimerFunc(16, My_Timer, val);
}

void SelectionHandler(unsigned int x, unsigned int y)
{
	GLuint faceID = pickingTexture.ReadTexture(x, windowHeight - y - 1);
	if (faceID != 0)
	{
		currentFaceID = faceID;
	}

	if (selectionMode == ADD_FACE)
	{
		if (faceID != 0)
		{
			model.AddSelectedFace(faceID - 1);
		}
	}
	else if (selectionMode == DEL_FACE)
	{
		if (faceID != 0)
		{
			model.DeleteSelectedFace(faceID - 1);
		}
	}
	else if (selectionMode == SELECT_POINT)
	{
		currentMouseX = x;
		currentMouseY = y;
		updateFlag = true;
	}
	else if (selectionMode == ADD_POINT)
	{
		//model.FaceToPoint();
		currentMouseX = x;
		currentMouseY = y;
		updateFlag = true;
		//model.AddSelectedPoint();
	}
	else if (selectionMode == DEL_POINT)
	{

	}
}

//Mouse event
void MyMouse(int button, int state, int x, int y)
{
	if (!TwEventMouseButtonGLUT(button, state, x, y))
	{
		meshWindowCam.mouseEvents(button, state, x, y);

		if (button == GLUT_RIGHT_BUTTON)
		{
			if (state == GLUT_DOWN)
			{
				isRightButtonPress = true;
				SelectionHandler(x, y);
			}
			else if (state == GLUT_UP)
			{
				isRightButtonPress = false;
			}
		}
	}
}

//Keyboard event
void MyKeyboard(unsigned char key, int x, int y)
{
	if (!TwEventKeyboardGLUT(key, x, y))
	{
		meshWindowCam.keyEvents(key);
	}
	cout << "Input : " << key << endl;
	if (key == 'f')
	{



		model.FaceToPoint();

		for (int i = 0; i < model.selectPoint_Seq.size(); i++)
		{
			for (int j = 0; j < model.selectedPoint.size(); j++)
			{
				if (model.selectedPoint[j] == model.selectPoint_Seq[i])
				{
					model.selectPoint_Seq[i] = j;
				}
			}
		}


		std::vector < MyMesh::VertexHandle> vhandle;
		//vhandle = new MyMesh::VertexHandle[model.selectedPoint.size()];

		for (int i = 0; i < model.selectedPoint.size(); i++)
		{
			MyMesh::VertexHandle TempPoint = model.model.mesh.vertex_handle(model.selectedPoint[i]);
			MyMesh::Point closestPoint = model.model.mesh.point(TempPoint);
			vhandle.push_back(BeSelectModel.model.mesh.add_vertex(closestPoint));
			cout << "X: " << closestPoint[0] << "Y: " << closestPoint[1] << "Z: " << closestPoint[2] << endl;
		}
		std::vector<MyMesh::VertexHandle>  face_vhandles;
		for (int i = 0; i < model.selectPoint_Seq.size() / 3; i++)
		{
			face_vhandles.clear();
			face_vhandles.push_back(vhandle[model.selectPoint_Seq[i * 3]]);
			face_vhandles.push_back(vhandle[model.selectPoint_Seq[(i * 3) + 1]]);
			face_vhandles.push_back(vhandle[model.selectPoint_Seq[(i * 3) + 2]]);
			BeSelectModel.model.mesh.add_face(face_vhandles);
			cout << "FACE " << i << " Is be Rebuild" << endl;
		}

		MyMesh::HalfedgeIter HFI = BeSelectModel.model.mesh.halfedges_begin();
		MyMesh::HalfedgeHandle HFh;
		for (; HFI != BeSelectModel.model.mesh.halfedges_end(); ++HFI)
		{
			HFh = BeSelectModel.model.mesh.halfedge_handle(HFI->idx());

			bool ISBOUND = BeSelectModel.model.mesh.is_boundary(HFh);
			if (ISBOUND)
			{
				MyMesh::VertexHandle outPoint = BeSelectModel.model.mesh.from_vertex_handle(HFh);
				OuterPoint.push_back(outPoint.idx());
				break;
			}
		}

		HFh = BeSelectModel.model.mesh.next_halfedge_handle(HFh);
		for (;;)
		{
			MyMesh::VertexHandle outPoint = BeSelectModel.model.mesh.from_vertex_handle(HFh);
			if (outPoint.idx() == OuterPoint[0])
			{
				break;
			}
			OuterPoint.push_back(outPoint.idx());
			HFh = BeSelectModel.model.mesh.next_halfedge_handle(HFh);
		}



		//caclu Outer lengh
		for (int i = 0; i < OuterPoint.size(); i++)
		{
			MyMesh::VertexHandle outPoint1 = BeSelectModel.model.mesh.vertex_handle(OuterPoint[(i) % OuterPoint.size()]);
			MyMesh::VertexHandle outPoint2 = BeSelectModel.model.mesh.vertex_handle(OuterPoint[(i + 1) % OuterPoint.size()]);
			MyMesh::Point outPoint1_p = BeSelectModel.model.mesh.point(outPoint1);
			MyMesh::Point outPoint2_p = BeSelectModel.model.mesh.point(outPoint2);
			MyMesh::Point disPoint = outPoint1_p - outPoint2_p;
			float dis = disPoint[0] * disPoint[0] + disPoint[1] * disPoint[1];
			dis = sqrt(dis);
			dis = sqrt(dis*dis + disPoint[2] * disPoint[2]);
			OuterLengh += dis;
		}
		cout << "Outer Lengh is " << OuterLengh << endl;
		//Make UV
		BeSelectModel.model.mesh.request_vertex_texcoords2D();// _vertex_texcoords2D();
		float nowDis = 0;


		MyMesh::VertexHandle tempoutPoint1 = BeSelectModel.model.mesh.vertex_handle(OuterPoint[(0) % OuterPoint.size()]);
		MyMesh::TexCoord2D tempUV;
		tempUV[0] = 0;
		tempUV[1] = 0;
		cout << "Point " << 0 << " UV " << tempUV[0] << " " << tempUV[1] << endl;
		BeSelectModel.model.mesh.set_texcoord2D(tempoutPoint1, tempUV);

		for (int i = 0; i < OuterPoint.size(); i++)
		{
			MyMesh::VertexHandle outPoint1 = BeSelectModel.model.mesh.vertex_handle(OuterPoint[(i) % OuterPoint.size()]);
			MyMesh::VertexHandle outPoint2 = BeSelectModel.model.mesh.vertex_handle(OuterPoint[(i + 1) % OuterPoint.size()]);
			MyMesh::Point outPoint1_p = BeSelectModel.model.mesh.point(outPoint1);
			MyMesh::Point outPoint2_p = BeSelectModel.model.mesh.point(outPoint2);
			MyMesh::Point disPoint = outPoint1_p - outPoint2_p;
			float dis = disPoint[0] * disPoint[0] + disPoint[1] * disPoint[1];
			dis = sqrt(dis);
			dis = sqrt(dis*dis + disPoint[2] * disPoint[2]);
			cout << "Dis "  <<dis;
			nowDis = nowDis + dis;
			if (nowDis <= (OuterLengh / 4.0))
			{
				MyMesh::TexCoord2D UV;
				UV[0] = 0;
				UV[1] = nowDis / (OuterLengh / 4.0);
				BeSelectModel.model.mesh.set_texcoord2D(outPoint2, UV);
				cout << "Point " << i + 1 << " UV " << UV[0] << " " << UV[1] << endl;
			}
			else if ((nowDis <= ((OuterLengh / 4.0) * 2.0)) && (nowDis > ((OuterLengh / 4.0)*1.0)))
			{
				MyMesh::TexCoord2D UV;
				UV[0] = ((nowDis - (OuterLengh / 4.0)*1.0) / (OuterLengh / 4.0));
				UV[1] = 1;
				BeSelectModel.model.mesh.set_texcoord2D(outPoint2, UV);
				cout << "Point " << i + 1 << " UV " << UV[0] << " " << UV[1] << endl;
			}
			else if ((nowDis <= ((OuterLengh / 4.0) * 3.0)) && (nowDis > ((OuterLengh / 4.0)*2.0)))
			{
				MyMesh::TexCoord2D UV;
				UV[0] = 1;
				UV[1] = 1 - ((nowDis - (OuterLengh / 4.0)*2.0)) / (OuterLengh / 4.0);
				BeSelectModel.model.mesh.set_texcoord2D(outPoint2, UV);
				cout << "Point " << i + 1 << " UV " << UV[0] << " " << UV[1] << endl;
			}
			else if ((nowDis <= (OuterLengh)) && (nowDis > ((OuterLengh / 4.0)*3.0)))
			{
				MyMesh::TexCoord2D UV;
				UV[0] = 1 - ((nowDis - (OuterLengh / 4.0)*3.0)) / (OuterLengh / 4.0);
				UV[1] = 0;
				BeSelectModel.model.mesh.set_texcoord2D(outPoint2, UV);
				cout << "Point " << i + 1 << " UV " << UV[0] << " " << UV[1] << endl;
			}
		}



		MyMesh::VertexIter VI = BeSelectModel.model.mesh.vertices_begin();
		for (; VI != BeSelectModel.model.mesh.vertices_end(); ++VI)
		{
			MyMesh::VertexHandle SeachPoint = BeSelectModel.model.mesh.vertex_handle(VI->idx());// VI.handle();// BeSelectModel.model.mesh.vertex_handle(VI);

			for (int j = 0; j < OuterPoint.size(); j++)
			{
				if (OuterPoint[j] != SeachPoint.idx())
				{
					InnerPoint.push_back(SeachPoint.idx());
				}
			}
		}
		BeSelectModel.MY_LoadToShader();




		SparseMatrix<double> A(3, 3);
		A.insert(0, 0) = 1.0;
		A.insert(1, 1) = 1.0;
		A.insert(2, 2) = 1.0;
 
		OpenMesh::EPropHandleT<MyMesh::Point> Wi;
		BeSelectModel.model.mesh.add_property(Wi);

		for (MyMesh::EdgeIter EI = BeSelectModel.model.mesh.edges_begin(); EI != BeSelectModel.model.mesh.edges_begin();++EI)
		{
			MyMesh::Point tmepWi;

			MyMesh::EdgeHandle Eh = BeSelectModel.model.mesh.edge_handle(EI->idx());
			MyMesh::HalfedgeHandle HeH = BeSelectModel.model.mesh.halfedge_handle(Eh,0);

			MyMesh::VertexHandle FromVertex = BeSelectModel.model.mesh.from_vertex_handle(HeH);
			MyMesh::VertexHandle TOVertex = BeSelectModel.model.mesh.to_vertex_handle(HeH);
			MyMesh::VertexHandle OppositeVertex = BeSelectModel.model.mesh.to_vertex_handle(HeH);
			tmepWi[0] = 0;
			tmepWi[1] = 1;
			tmepWi[2] = 20;

			BeSelectModel.model.mesh.property(Wi, *EI) = tmepWi;
		}




	}
	else if (key == 'z') 
	{
		model.undoadd();
	}
	else if (key == 'c') 
	{
	model.sw();
	}
	else if (key == 'v') 
	{
	model.addfacecheck = true;
	}
	else if (key == 'g') 
	{
		model.fcoffsetsw();
	}
	else if (key == 'h') 
	{
		if (model.faceoffsetsw == true && model.vertexqueue.size() ==4)
		{
			fcoffset( true);
		}
	}
	else if (key == 'j')
	{
	if (model.faceoffsetsw == true && model.vertexqueue.size() == 4)
	{
		fcoffset(false);
	}
	}
	else if (key == 'g')
	{

	}
}


void MyMouseMoving(int x, int y) {
	if (!TwEventMouseMotionGLUT(x, y))
	{
		meshWindowCam.mouseMoveEvent(x, y);

		if (isRightButtonPress)
		{
		//	SelectionHandler(x, y);
		}
}
}

int main(int argc, char* argv[])
{
#ifdef __APPLE__
	//Change working directory to source code path
	chdir(__FILEPATH__("/../Assets/"));
#endif
	// Initialize GLUT and GLEW, then create a window.
	////////////////////
	glutInit(&argc, argv);
#ifdef _MSC_VER
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_ALPHA);
#else
	glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
#endif

	glutInitWindowPosition(100, 100);
	glutInitWindowSize(windowWidth, windowHeight);
	mainWindow = glutCreateWindow(ProjectName.c_str()); // You cannot use OpenGL functions before this line; The OpenGL context must be created first by glutCreateWindow()!
#ifdef _MSC_VER
	glewInit();
#endif

	glutReshapeFunc(Reshape);
	glutIdleFunc(RenderAll);
	glutSetOption(GLUT_RENDERING_CONTEXT, GLUT_USE_CURRENT_CONTEXT);
	SetupGUI();

	glutMouseFunc(MyMouse);
	glutKeyboardFunc(MyKeyboard);
	glutMotionFunc(MyMouseMoving);
	glutDisplayFunc(RenderMeshWindow);
	InitOpenGL();
	InitData();

	//Print debug information 
	Common::DumpInfo();

	// Enter main event loop.
	glutMainLoop();

	return 0;
}

