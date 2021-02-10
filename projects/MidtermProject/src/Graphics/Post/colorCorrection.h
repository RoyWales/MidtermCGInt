#pragma once

#include "Graphics/Post/PostEffect.h"

class colorCorrection : public PostEffect
{
public:
	void Init(unsigned width, unsigned height) override;

	void ApplyEffect(PostEffect* buffer) override;

	LUT3D GetLUT() const;

	void SetLUT(LUT3D lut);

private:
	//float _intensity = 1.0f;
	LUT3D _lut;
};