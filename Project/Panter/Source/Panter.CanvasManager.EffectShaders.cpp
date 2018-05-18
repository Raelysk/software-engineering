#include <Windows.h>

#include "Panter.CanvasManager.EffectShaders.h"

#include "..\Intermediate\Shaders\BrightnessContrastGammaPS.cso.h"
#include "..\Intermediate\Shaders\CheckerboardPS.cso.h"

using namespace Panter;

const ShaderData EffectShaders::BrightnessContrastGammaPS = { BrightnessContrastGammaPSData, sizeof(BrightnessContrastGammaPSData) };
const ShaderData EffectShaders::CheckerboardPS = { CheckerboardPSData, sizeof(CheckerboardPSData) };