#include <Windows.h>

#include "Panter.CanvasManager.EffectShaders.h"

#include "..\Intermediate\Shaders\BrightnessContrastGammaPS.cso.h"

using namespace Panter;

ShaderData EffectShaders::BrightnessContrastGammaPS = { BrightnessContrastGammaPSData, sizeof(BrightnessContrastGammaPSData) };