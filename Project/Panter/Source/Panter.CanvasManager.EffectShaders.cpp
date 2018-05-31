#include <Windows.h>

#include "Panter.CanvasManager.EffectShaders.h"

#include "..\Intermediate\Shaders\CheckerboardPS.cso.h"
#include "..\Intermediate\Shaders\BrightnessContrastGammaPS.cso.h"
#include "..\Intermediate\Shaders\BlurPS.cso.h"
#include "..\Intermediate\Shaders\SharpenPS.cso.h"

using namespace Panter;

const ShaderData EffectShaders::CheckerboardPS = { CheckerboardPSData, sizeof(CheckerboardPSData) };
const ShaderData EffectShaders::BrightnessContrastGammaPS = { BrightnessContrastGammaPSData, sizeof(BrightnessContrastGammaPSData) };
const ShaderData EffectShaders::BlurPS = { BlurPSData, sizeof(BlurPSData) };
const ShaderData EffectShaders::SharpenPS = { SharpenPSData, sizeof(SharpenPSData) };