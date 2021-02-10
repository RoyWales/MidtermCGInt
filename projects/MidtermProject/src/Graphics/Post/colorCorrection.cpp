#include "colorCorrection.h"

void colorCorrection::Init(unsigned width, unsigned height)
{
	int index = int(_buffers.size());
	_buffers.push_back(new Framebuffer());
	_buffers[index]->AddColorTarget(GL_RGBA8);
	_buffers[index]->AddDepthTarget();
	_buffers[index]->Init(width, height);

	index = int(_shaders.size());
	_shaders.push_back(Shader::Create());
	_shaders[index]->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
	_shaders[index]->LoadShaderPartFromFile("shaders/Post/color_correction_frag.glsl", GL_FRAGMENT_SHADER);
	_shaders[index]->Link();

	_lut.loadFromFile("cubes/BrightenedCorrection.cube");
	SetLUT(_lut);
	PostEffect::Init(width, height);

}

void colorCorrection::ApplyEffect(PostEffect* buffer)
{
	BindShader(0);

	buffer->BindColorAsTexture(0, 0, 0);
	_lut.bind(30);

	_buffers[0]->RenderToFSQ();

	_lut.unbind(30);

	buffer->UnbindTexture(0);

	UnbindShader();
}

LUT3D colorCorrection::GetLUT() const
{
	return _lut;
}

void colorCorrection::SetLUT(LUT3D lut)
{
	_lut = lut;
}

