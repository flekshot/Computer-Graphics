#pragma once
#include "Geometry.h"
#include "LabState.h"

namespace sse {
// Rocky vertices include the water surface; gas worlds remain spherical.
Geometry makeWorkshopPlanet(const WorkshopSettings& settings, unsigned segments = 128, unsigned rings = 64);
// Equal-area deterministic sampling, returned as a fraction in [0, 1].
float workshopOceanCoverage(const WorkshopSettings& settings);
WorkshopSettings workshopPreset(int index);
// Direction is normalized internally; zero/non-finite directions are invalid.
float sampleWorkshopTerrain(const glm::vec3& unitDirection, const WorkshopSettings& settings);
} // namespace sse
