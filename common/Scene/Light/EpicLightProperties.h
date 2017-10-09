#pragma once

#ifndef __EPIC_LIGHT_PROPERTIES__
#define __EPIC_LIGHT_PROPERTIES__

#include "common/common.h"
#include "common/Scene/Light/LightProperties.h"

struct EpicLightProperties : public LightProperties {
	float radius;
	glm::vec4 direction;
	glm::vec4 skyColor;
	glm::vec4 groundColor;
};

#endif
