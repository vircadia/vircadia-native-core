//
//  RenderableParticleEffectEntityItem.cpp
//  interface/src
//
//  Created by Jason Rickwald on 3/2/15.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/gtx/quaternion.hpp>

#include <gpu/GPUConfig.h>

#include <DependencyManager.h>
#include <DeferredLightingEffect.h>
#include <PerfStat.h>

#include "RenderableParticleEffectEntityItem.h"

EntityItem* RenderableParticleEffectEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
	return new RenderableParticleEffectEntityItem(entityID, properties);
}

void RenderableParticleEffectEntityItem::render(RenderArgs* args) {
    PerformanceTimer perfTimer("RenderableParticleEffectEntityItem::render");
    assert(getType() == EntityTypes::ParticleEffect);
    glm::vec3 position = getPositionInMeters();
    glm::vec3 center = getCenterInMeters();
    glm::quat rotation = getRotation();
	float pa_rad = getParticleRadius();

    const float MAX_COLOR = 255.0f;
    glm::vec4 sphereColor(getColor()[RED_INDEX] / MAX_COLOR, getColor()[GREEN_INDEX] / MAX_COLOR,
                    getColor()[BLUE_INDEX] / MAX_COLOR, getLocalRenderAlpha());
                    
    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        
        
        glPushMatrix();
            glm::vec3 positionToCenter = center - position;
            glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);
            
            const int SLICES = 8;
            const int STACKS = 5;

			glBegin(GL_QUADS);
			glColor4f(sphereColor.r, sphereColor.g, sphereColor.b, sphereColor.a);

			// Right now we're just iterating over particles and rendering as a cross of four quads.
			// This is pretty dumb, it was quick enough to code up.  Really, there should be many
			// rendering modes, including the all-important textured billboards.
			
			quint32 paiter = pa_head;
			while (pa_life[paiter] > 0.0f)
			{
				int j = paiter * 3;

				glVertex3f(pa_position[j] - pa_rad, pa_position[j + 1] + pa_rad, pa_position[j + 2]);
				glVertex3f(pa_position[j] + pa_rad, pa_position[j + 1] + pa_rad, pa_position[j + 2]);
				glVertex3f(pa_position[j] + pa_rad, pa_position[j + 1] - pa_rad, pa_position[j + 2]);
				glVertex3f(pa_position[j] - pa_rad, pa_position[j + 1] - pa_rad, pa_position[j + 2]);

				glVertex3f(pa_position[j] + pa_rad, pa_position[j + 1] + pa_rad, pa_position[j + 2]);
				glVertex3f(pa_position[j] - pa_rad, pa_position[j + 1] + pa_rad, pa_position[j + 2]);
				glVertex3f(pa_position[j] - pa_rad, pa_position[j + 1] - pa_rad, pa_position[j + 2]);
				glVertex3f(pa_position[j] + pa_rad, pa_position[j + 1] - pa_rad, pa_position[j + 2]);

				glVertex3f(pa_position[j], pa_position[j + 1] + pa_rad, pa_position[j + 2] - pa_rad);
				glVertex3f(pa_position[j], pa_position[j + 1] + pa_rad, pa_position[j + 2] + pa_rad);
				glVertex3f(pa_position[j], pa_position[j + 1] - pa_rad, pa_position[j + 2] + pa_rad);
				glVertex3f(pa_position[j], pa_position[j + 1] - pa_rad, pa_position[j + 2] - pa_rad);

				glVertex3f(pa_position[j], pa_position[j + 1] + pa_rad, pa_position[j + 2] + pa_rad);
				glVertex3f(pa_position[j], pa_position[j + 1] + pa_rad, pa_position[j + 2] - pa_rad);
				glVertex3f(pa_position[j], pa_position[j + 1] - pa_rad, pa_position[j + 2] - pa_rad);
				glVertex3f(pa_position[j], pa_position[j + 1] - pa_rad, pa_position[j + 2] + pa_rad);

				paiter = (paiter + 1) % _maxParticles;
			}

			glEnd();
        glPopMatrix();
    glPopMatrix();
};
