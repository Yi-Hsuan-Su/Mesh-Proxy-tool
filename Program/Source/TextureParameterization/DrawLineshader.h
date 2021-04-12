#pragma once
#include <ShaderObject.h>

class DrawLineshader:public ShaderObject
{
public:
	DrawLineshader();
	~DrawLineshader();

	bool Init();
	void Enable();
	void Disable();
	void SetMVMat(const glm::mat4& mat);
	void SetPMat(const glm::mat4& mat);
	void SetPointColor(const glm::vec4& pointColor);
	void SetPointSize(float pointSize);


private:
	GLuint um4pLocation;
	GLuint um4mvLocation;
	GLuint pointColorLocation;
};



