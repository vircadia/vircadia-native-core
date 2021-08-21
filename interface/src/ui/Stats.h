//
//  Created by Bradley Austin Davis 2015/06/17
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Stats_h
#define hifi_Stats_h

#include <QtGui/QVector3D>

#include <OffscreenQmlElement.h>
#include <AudioIOStats.h>
#include <render/Args.h>
#include <shared/QtHelpers.h>

#define STATS_PROPERTY(type, name, initialValue) \
    Q_PROPERTY(type name READ name NOTIFY name##Changed) \
public: \
    type name() { return _##name; }; \
private: \
    type _##name{ initialValue };

/*@jsdoc
 * The <code>Stats</code> API provides statistics on Interface and domain operation, per the statistics overlay.
 *
 * <p><strong>Note:</strong> This API is primarily an internal diagnostics tool and is provided "as is".</p>
 *
 * @namespace Stats
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 * @hifi-server-entity
 * @hifi-assignment-client
 *
 * @property {boolean} expanded - <code>true</code> if the statistics overlay should be in expanded form when the overlay is 
 *     displayed, <code>false</code> if it shouldn't be expanded.
 * @property {boolean} timingExpanded - <code>true</code> if timing details should be displayed when the statistics overlay is 
 *     displayed in expanded form, <code>false</code> if timing details should not be displayed. Set by the menu item, 
 *     Developer &gt; Timing &gt; Performance Timer &gt; Display Timing Details. 
 *     <em>Read-only.</em>
 * @property {string} monospaceFont - The name of the monospace font used in the statistics overlay.
 *     <em>Read-only.</em>
 *
 * @property {number} serverCount - The number of servers that Interface is connected to.
 *     <em>Read-only.</em>
 * @property {number} renderrate - The rate at which new GPU frames are being created, in Hz.
 *     <em>Read-only.</em>
 * @property {number} presentrate - The rate at which the display plugin is presenting to the display device, in Hz.
 *     <em>Read-only.</em>
 * @property {number} stutterrate - The rate at which the display plugin is reprojecting old GPU frames, in Hz.
 *     <em>Read-only.</em>
 *
 * @property {number} appdropped - The number of times a frame has not been provided to the display device in time.
 *     <em>Read-only.</em>
 * @property {number} longsubmits - The number of times the display device has taken longer than 11ms to return after being 
 *     given a frame.
 *     <em>Read-only.</em>
 * @property {number} longrenders - The number of times it has taken longer than 11ms to submit a new frame to the display 
 *     device.
 *     <em>Read-only.</em>
 * @property {number} longframes - The number of times <code>longsubmits + longrenders</code> has taken longer than 15ms.
 *     <em>Read-only.</em>
 *
 * @property {number} presentnewrate - The rate at which the display plugin is presenting new GPU frames, in Hz.
 *     <em>Read-only.</em>
 * @property {number} presentdroprate - The rate at which the display plugin is dropping GPU frames, in Hz.
 *     <em>Read-only.</em>

 * @property {number} gameLoopRate - The rate at which the game loop is running, in Hz.
 *     <em>Read-only.</em>
 * @property {number} refreshRateTarget - The current target refresh rate, in Hz, per the current <code>refreshRateMode</code> 
 *     and <code>refreshRateRegime</code> if in desktop mode; a higher rate if in VR mode. 
 *     <em>Read-only.</em>
 * @property {RefreshRateProfileName} refreshRateMode - The current refresh rate profile.
 *     <em>Read-only.</em>
 * @property {RefreshRateRegimeName} refreshRateRegime - The current refresh rate regime.
 *     <em>Read-only.</em>
 * @property {UXModeName} uxMode - The user experience (UX) mode that Interface is running in.
 *     <em>Read-only.</em>
 * @property {number} avatarCount - The number of avatars in the domain other than the client's.
 *     <em>Read-only.</em>
 * @property {number} heroAvatarCount - The number avatars in a "hero" zone in the domain, other than the client's.
 *     <em>Read-only.</em>
 * @property {number} physicsObjectCount - The number of objects that have collisions enabled.
 *     <em>Read-only.</em>
 * @property {number} updatedAvatarCount - The number of avatars in the domain, other than the client's, that were updated in 
 *     the most recent game loop.
 *     <em>Read-only.</em>
 * @property {number} updatedHeroAvatarCount - The number of avatars in a "hero" zone in the domain, other than the client's, 
 *     that were updated in the most recent game loop.
 *     <em>Read-only.</em>
 * @property {number} notUpdatedAvatarCount - The number of avatars in the domain, other than the client's, that weren't able 
 *     to be updated in the most recent game loop because there wasn't enough time to.
 *     <em>Read-only.</em>
 * @property {number} packetInCount - The number of packets being received from the domain server, in packets per second.
 *     <em>Read-only.</em>
 * @property {number} packetOutCount - The number of packets being sent to the domain server, in packets per second.
 *     <em>Read-only.</em>
 * @property {number} mbpsIn - The amount of data being received from the domain server, in megabits per second.
 *     <em>Read-only.</em>
 * @property {number} mbpsOut - The amount of data being sent to the domain server, in megabits per second.
 *    <em>Read-only.</em>
   @property {number} assetMbpsIn - The amount of data being received from the asset server, in megabits per second.
 *     <code>0.0</code> if not connected to an avatar mixer.
 *     <em>Read-only.</em>
 * @property {number} assetMbpsOut - The amount of data being sent to the asset server, in megabits per second.
 *     <code>0.0</code> if not connected to an avatar mixer.
 *     <em>Read-only.</em>
 * @property {number} audioPing - The ping time to the audio mixer, in ms.
 *     <code>-1</code> if not connected to an audio mixer.
 *     <em>Read-only.</em>
 * @property {number} avatarPing - The ping time to the avatar mixer, in ms.
 *     <code>-1</code> if not connected to an avatar mixer.
 *     <em>Read-only.</em>
 * @property {number} entitiesPing - The average ping time to the entity servers, in ms.
 *     <code>-1</code> if not connected to an entity server.
 *     <em>Read-only.</em>
 * @property {number} assetPing - The ping time to the asset server, in ms.
 *     <code>-1</code> if not connected to an asset server.
 *     <em>Read-only.</em>
 * @property {number} messagePing - The ping time to the message mixer, in ms.
 *     <code>-1</code> if not connected to a message mixer.
 *     <em>Read-only.</em>
 * @property {Vec3} position - The position of the user's avatar.
 *     <em>Read-only.</em>
 *     <p><strong>Note:</strong> Property not available in the API.</p>
 * @property {number} speed - The speed of the user's avatar, in m/s.
 *     <em>Read-only.</em>
 * @property {number} yaw - The yaw of the user's avatar body, in degrees.
 *     <em>Read-only.</em>
 * @property {number} avatarMixerInKbps - The amount of data being received from the avatar mixer, in kilobits per second.
 *     <code>-1</code> if not connected to an avatar mixer.
 *     <em>Read-only.</em>
 * @property {number} avatarMixerInPps - The number of packets being received from the avatar mixer, in packets per second.
 *     <code>-1</code> if not connected to an avatar mixer.
 *     <em>Read-only.</em>
 * @property {number} avatarMixerOutKbps - The amount of data being sent to the avatar mixer, in kilobits per second.
 *      <code>-1</code> if not connected to an avatar mixer.
 *      <em>Read-only.</em>
 * @property {number} avatarMixerOutPps - The number of packets being sent to the avatar mixer, in packets per second.
 *      <code>-1</code> if not connected to an avatar mixer.
 *      <em>Read-only.</em>
 * @property {number} myAvatarSendRate - The number of avatar packets being sent by the user's avatar, in packets per second.
 *      <em>Read-only.</em>
 *
 * @property {number} audioMixerInKbps - The amount of data being received from the audio mixer, in kilobits per second.
 *     <code>-1</code> if not connected to an audio mixer.
 *     <em>Read-only.</em>
 * @property {number} audioMixerInPps - The number of packets being received from the audio mixer, in packets per second.
 *     <code>-1</code> if not connected to an audio mixer.
 *     <em>Read-only.</em>
 * @property {number} audioMixerOutKbps - The amount of data being sent to the audio mixer, in kilobits per second.
 *     <code>-1</code> if not connected to an audio mixer.
 *     <em>Read-only.</em>
 * @property {number} audioMixerOutPps - The number of packets being sent to the audio mixer, in packets per second.
 *     <code>-1</code> if not connected to an audio mixer.
 *     <em>Read-only.</em>
 * @property {number} audioMixerKbps - The total amount of data being sent to and received from the audio mixer, in kilobits 
 *     per second.
 *     <code>-1</code> if not connected to an audio mixer.
 *     <em>Read-only.</em>
 * @property {number} audioMixerPps - The total number of packets being sent to and received from the audio mixer, in packets 
 *     per second.
 *     <code>-1</code> if not connected to an audio mixer.
 *     <em>Read-only.</em>
 * @property {number} audioOutboundPPS - The number of non-silent audio packets being sent by the user, in packets per second.
 *     <code>-1</code> if not connected to an audio mixer.
 *     <em>Read-only.</em>
 * @property {number} audioSilentOutboundPPS -  The number of silent audio packets being sent by the user, in packets per 
 *     second.
 *     <code>-1</code> if not connected to an audio mixer.
 *     <em>Read-only.</em>
 * @property {number} audioInboundPPS -  The number of non-silent audio packets being received by the user, in packets per
 *     second.
 *     <code>-1</code> if not connected to an audio mixer.
 *     <em>Read-only.</em>
 * @property {number} audioAudioInboundPPS -  The number of non-silent audio packets being received by the user, in packets per
 *     second.
 *     <code>-1</code> if not connected to an audio mixer.
 *     <em>Read-only.</em>
 *     <p class="important">Deprecated: This property is deprecated and will be removed. Use <code>audioInboundPPS</code> 
 *     instead.</p>
 * @property {number} audioSilentInboundPPS -  The number of silent audio packets being received by the user, in packets per 
 *     second.
 *     <code>-1</code> if not connected to an audio mixer.
 *     <em>Read-only.</em>
 * @property {number} audioPacketLoss - The number of audio packets being lost, sent to or received from the audio mixer, in %.
 *     <code>-1</code> if not connected to an audio mixer.
 *     <em>Read-only.</em>
 * @property {string} audioCodec - The name of the audio codec.
 *     <em>Read-only.</em>
 * @property {string} audioNoiseGate - The status of the audio noise gate: <code>"Open"</code> or <code>"Closed"</code>.
 *     <em>Read-only.</em>
 * @property {Vec2} audioInjectors - The number of audio injectors, local and non-local.
 *     <em>Read-only.</em>
 *     <p><strong>Note:</strong> Property not available in the API.</p>
 * @property {number} entityPacketsInKbps - The average amount of data being received from entity servers, in kilobits per 
 *     second. (Multiply by the number of entity servers to get the total amount of data being received.)
 *     <code>-1</code> if not connected to an entity server.
 *     <em>Read-only.</em>
 *
 * @property {number} downloads - The number of downloads in progress.
 *     <em>Read-only.</em>
 * @property {number} downloadLimit - The maximum number of concurrent downloads.
 *     <em>Read-only.</em>
 * @property {number} downloadsPending - The number of downloads pending.
 *     <em>Read-only.</em>
 * @property {string[]} downloadUrls - The download URLs.
 *     <em>Read-only.</em>
 *     <p><strong>Note:</strong> Property not available in the API.</p>
 * @property {number} processing - The number of completed downloads being processed.
 *     <em>Read-only.</em>
 * @property {number} processingPending - The number of completed downloads waiting to be processed.
 *     <em>Read-only.</em>
 * @property {number} triangles - The number of triangles in the rendered scene.
 *     <em>Read-only.</em>
 * @property {number} drawcalls - The number of draw calls made for the rendered scene.
 *     <em>Read-only.</em>
 * @property {number} materialSwitches - The number of material switches performed for the rendered scene.
 *     <em>Read-only.</em>
 * @property {number} itemConsidered - The number of item considerations made for rendering.
 *     <em>Read-only.</em>
 * @property {number} itemOutOfView - The number of items out of view.
 *     <em>Read-only.</em>
 * @property {number} itemTooSmall - The number of items too small to render.
 *     <em>Read-only.</em>
 * @property {number} itemRendered - The number of items rendered.
 *     <em>Read-only.</em>
 * @property {number} shadowConsidered - The number of shadow considerations made for rendering.
 *     <em>Read-only.</em>
 * @property {number} shadowOutOfView - The number of shadows out of view.
 *     <em>Read-only.</em>
 * @property {number} shadowTooSmall - The number of shadows too small to render.
 *     <em>Read-only.</em>
 * @property {number} shadowRendered - The number of shadows rendered.
 *     <em>Read-only.</em>
 * @property {string} sendingMode - Description of the octree sending mode.
 *     <em>Read-only.</em>
 * @property {string} packetStats - Description of the octree packet processing state.
 *     <em>Read-only.</em>
 * @property {number} lodAngle - The target LOD angle, in degrees.
 *     <em>Read-only.</em>
 * @property {number} lodTargetFramerate - The target LOD frame rate, in Hz.
 *     <em>Read-only.</em>
 * @property {string} lodStatus - Description of the current LOD.
 *     <em>Read-only.</em>
 * @property {number} numEntityUpdates - The number of entity updates that happened last frame.
 *     <em>Read-only.</em>
 * @property {number} numNeededEntityUpdates - The total number of entity updates scheduled for last frame.
 *     <em>Read-only.</em>
 * @property {string} timingStats - Details of the average time (ms) spent in and number of calls made to different parts of 
 *     the code. Provided only if <code>timingExpanded</code> is <code>true</code>. Only the top 10 items are provided if 
 *     Developer &gt; Timing &gt; Performance Timer &gt; Only Display Top 10 is enabled.
 *     <em>Read-only.</em>
 * @property {string} gameUpdateStats - Details of the average time (ms) spent in different parts of the game loop.
 *     <em>Read-only.</em>
 * @property {number} serverElements - The total number of elements in the server octree.
 *     <em>Read-only.</em>
 * @property {number} serverInternal - The number of internal elements in the server octree.
 *     <em>Read-only.</em>
 * @property {number} serverLeaves - The number of leaf elements in the server octree.
 *     <em>Read-only.</em>
 * @property {number} localElements - The total number of elements in the client octree.
 *     <em>Read-only.</em>
 * @property {number} localInternal - The number of internal elements in the client octree.
 *     <em>Read-only.</em>
 * @property {number} localLeaves - The number of leaf elements in the client octree.
 *     <em>Read-only.</em>
 * @property {number} rectifiedTextureCount - The number of textures that have been resized so that their dimensions is a power 
 *     of 2 if smaller than 128 pixels, or a multiple of 128 if greater than 128 pixels.
 *     <em>Read-only.</em>
 * @property {number} decimatedTextureCount - The number of textures that have been reduced in size because they were over the 
 *     maximum allowed dimensions of 8192 pixels on desktop or 2048 pixels on mobile.
 *     <em>Read-only.</em>
 * @property {number} gpuBuffers - The number of OpenGL buffer objects managed by the GPU back-end.
 *     <em>Read-only.</em>
 * @property {number} gpuBufferMemory - The total memory size of the <code>gpuBuffers</code>, in MB. 
 *     <em>Read-only.</em>
 * @property {number} gpuTextures - The number of OpenGL textures managed by the GPU back-end. This is the sum of the number of 
 *     textures managed for <code>gpuTextureResidentMemory</code>,  <code>gpuTextureResourceMemory</code>, and
 *     <code>gpuTextureFramebufferMemory</code>.
 *     <em>Read-only.</em>
 * @property {number} gpuTextureMemory - The total memory size of the <code>gpuTextures</code>, in MB. This is the sum of 
 *     <code>gpuTextureResidentMemory</code>,  <code>gpuTextureResourceMemory</code>, and 
 *     <code>gpuTextureFramebufferMemory</code>.
 *     <em>Read-only.</em>
 * @property {number} glContextSwapchainMemory -  The estimated memory used by the default OpenGL frame buffer, in MB.
 *     <em>Read-only.</em>
 * @property {number} qmlTextureMemory - The memory size of textures managed by the offscreen QML surface, in MB.
 *     <em>Read-only.</em>
 * @property {number} texturePendingTransfers - The memory size of textures pending transfer to the GPU, in MB.
 *     <em>Read-only.</em>
 * @property {number} gpuTextureResidentMemory - The memory size of the "strict" textures that always have their full 
 *     resolution in GPU memory, in MB.
 *     <em>Read-only.</em>
 * @property {number} gpuTextureFramebufferMemory - The memory size of the frame buffer on the GPU, in MB.
 *     <em>Read-only.</em>
 * @property {number} gpuTextureResourceMemory - The amount of GPU memory that has been allocated for "variable" textures that 
 *     don't necessarily always have their full resolution in GPU memory, in MB.
 *     <em>Read-only.</em>
 * @property {number} gpuTextureResourceIdealMemory - The amount of memory that "variable" textures would take up if they were 
 *     all completely loaded, in MB.
 *     <em>Read-only.</em>
 * @property {number} gpuTextureResourcePopulatedMemory - How much of the GPU memory allocated has actually been populated, in 
*      MB.
 *     <em>Read-only.</em>
 * @property {string} gpuTextureMemoryPressureState - The stats of the texture transfer engine.
 *     <ul>
 *         <li><code>"Undersubscribed"</code>: There is texture data that can fit in memory but that isn't on the GPU, so more 
 *         GPU texture memory should be allocated if possible.</li>
 *         <li><code>"Transfer"</code>: More GPU texture memory has been allocated and texture data is being transferred.</li>
 *         <li><code>"Idle"</code>: Either all texture data has been transferred to the GPU or there is nor more space 
 *         available.</li>
 *     </ul>
 *     <em>Read-only.</em>
 * @property {number} gpuFreeMemory - The amount of GPU memory available after all allocations, in MB. 
 *     <em>Read-only.</em>
 *     <p><strong>Note:</strong> This is not a reliable number because OpenGL doesn't have an official method of getting this 
 *     information.</p>
 * @property {number} gpuTextureExternalMemory - The estimated amount of memory consumed by textures being used but that are
 *     not managed by the GPU library, in MB.
 *     <em>Read-only.</em>
 * @property {Vec2} gpuFrameSize - The dimensions of the frames being rendered, in pixels.
 *     <em>Read-only.</em>
 *     <p><strong>Note:</strong> Property not available in the API.</p>
 * @property {number} gpuFrameTime - The time the GPU is spending on a frame, in ms.
 *     <em>Read-only.</em>
 * @property {number} gpuFrameTimePerPixel - The time the GPU is spending on a pixel, in ns.
 *     <em>Read-only.</em>
 * @property {number} batchFrameTime - The time being spent batch processing each frame, in ms.
 *     <em>Read-only.</em>
 * @property {number} engineFrameTime - The time being spent in the render engine each frame, in ms.
 *     <em>Read-only.</em>
 * @property {number} avatarSimulationTime - The time being spent simulating avatars each frame, in ms.
 *     <em>Read-only.</em>
 *
 * @property {number} stylusPicksCount - The number of stylus picks currently in effect.
 *     <em>Read-only.</em>
 * @property {number} rayPicksCount - The number of ray picks currently in effect.
 *     <em>Read-only.</em>
 * @property {number} parabolaPicksCount - The number of parabola picks currently in effect.
 *     <em>Read-only.</em>
 * @property {number} collisionPicksCount - The number of collision picks currently in effect.
 *     <em>Read-only.</em>
 * @property {Vec3} stylusPicksUpdated - The number of stylus pick intersection that were found in the most recent game loop: 
 *     <ul>
 *         <li><code>x</code> = entity intersections.</li>
 *         <li><code>y</code> = avatar intersections.</li>
 *         <li><code>z</code> = HUD intersections.</li>
 *     </ul>
 *     <em>Read-only.</em>
 *     <p><strong>Note:</strong> Property not available in the API.</p>
 * @property {Vec3} rayPicksUpdated - The number of ray pick intersections that were found in the most recent game loop:
 *     <ul>
 *         <li><code>x</code> = entity intersections.</li>
 *         <li><code>y</code> = avatar intersections.</li>
 *         <li><code>z</code> = HUD intersections.</li>
 *     </ul>
 *     <em>Read-only.</em>
 *     <p><strong>Note:</strong> Property not available in the API.</p>
 * @property {Vec3} parabolaPicksUpdated - The number of parabola pick intersections that were found in the most recent game
 *     loop:
 *     <ul>
 *         <li><code>x</code> = entity intersections.</li>
 *         <li><code>y</code> = avatar intersections.</li>
 *         <li><code>z</code> = HUD intersections.</li>
 *     </ul>
 *     <em>Read-only.</em>
 *     <p><strong>Note:</strong> Property not available in the API.</p>
 * @property {Vec3} collisionPicksUpdated - The number of collision pick intersections that were found in the most recent game
 *     loop:
 *     <ul>
 *         <li><code>x</code> = entity intersections.</li>
 *         <li><code>y</code> = avatar intersections.</li>
 *         <li><code>z</code> = HUD intersections.</li>
 *     </ul>
 *     <em>Read-only.</em>
 *     <p><strong>Note:</strong> Property not available in the API.</p>
 *
 * @property {boolean} eventQueueDebuggingOn - <code>true</code> if event queue statistics are provided, <code>false</code> if
 *     they're not.
 *     <em>Read-only.</em>
 * @property {number} mainThreadQueueDepth - The number of events in the main thread's event queue.
 *     Only provided if <code>eventQueueDebuggingOn</code> is <code>true</code>.
 *     <em>Read-only.</em>
 * @property {number} nodeListThreadQueueDepth - The number of events in the node list thread's event queue.
 *     Only provided if <code>eventQueueDebuggingOn</code> is <code>true</code>.
 *     <em>Read-only.</em>
 *
 * @comment The following property is from Stats.qml. It shouldn't be in the API.
 * @property {string} bgColor
 *     <em>Read-only.</em>
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 *
 * @comment The following properties are from QQuickItem. They shouldn't be in the API.
 * @property {boolean} activeFocus
 *     <em>Read-only.</em>
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {boolean} activeFocusOnTab
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {object} anchors
 *     <em>Read-only.</em>
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {boolean} antialiasing
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {number} baselineOffset
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {object[]} children
 *     <em>Read-only.</em>
 *     <p><strong>Note:</strong> Property not available in the API.</p>
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {boolean} clip
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {object} containmentMask
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {boolean} enabled
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {boolean} focus
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {number} height
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {number} implicitHeight
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {number} implicitWidth 
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {object} layer
 *     <em>Read-only.</em>
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {number} opacity
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {number} rotation
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {number} scale
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {boolean} smooth
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {string} state
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {number} transformOrigin
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {boolean} visible
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {number} width
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {number} x
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {number} y
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 * @property {number} z
 *     <p class="important">Deprecated: This property is deprecated and will be removed.</p>
 */
// Properties from x onwards are QQuickItem properties.

class Stats : public QQuickItem {
    Q_OBJECT
    HIFI_QML_DECL
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(bool timingExpanded READ isTimingExpanded NOTIFY timingExpandedChanged)
    Q_PROPERTY(QString monospaceFont READ monospaceFont CONSTANT)

    STATS_PROPERTY(int, serverCount, 0)
    // How often the app is creating new gpu::Frames
    STATS_PROPERTY(float, renderrate, 0)
    // How often the display plugin is presenting to the device
    STATS_PROPERTY(float, presentrate, 0)
    // How often the display device reprojecting old frames
    STATS_PROPERTY(float, stutterrate, 0)

    STATS_PROPERTY(int, appdropped, 0)
    STATS_PROPERTY(int, longsubmits, 0)
    STATS_PROPERTY(int, longrenders, 0)
    STATS_PROPERTY(int, longframes, 0)

    STATS_PROPERTY(float, presentnewrate, 0)
    STATS_PROPERTY(float, presentdroprate, 0)
    STATS_PROPERTY(int, gameLoopRate, 0)
    STATS_PROPERTY(int, avatarCount, 0)
    STATS_PROPERTY(int, refreshRateTarget, 0)
    STATS_PROPERTY(QString, refreshRateMode, QString())
    STATS_PROPERTY(QString, refreshRateRegime, QString())
    STATS_PROPERTY(QString, uxMode, QString())
    STATS_PROPERTY(int, heroAvatarCount, 0)
    STATS_PROPERTY(int, physicsObjectCount, 0)
    STATS_PROPERTY(int, updatedAvatarCount, 0)
    STATS_PROPERTY(int, updatedHeroAvatarCount, 0)
    STATS_PROPERTY(int, notUpdatedAvatarCount, 0)
    STATS_PROPERTY(int, packetInCount, 0)
    STATS_PROPERTY(int, packetOutCount, 0)
    STATS_PROPERTY(float, mbpsIn, 0)
    STATS_PROPERTY(float, mbpsOut, 0)
    STATS_PROPERTY(float, assetMbpsIn, 0)
    STATS_PROPERTY(float, assetMbpsOut, 0)
    STATS_PROPERTY(int, audioPing, 0)
    STATS_PROPERTY(int, avatarPing, 0)
    STATS_PROPERTY(int, entitiesPing, 0)
    STATS_PROPERTY(int, assetPing, 0)
    STATS_PROPERTY(int, messagePing, 0)
    STATS_PROPERTY(QVector3D, position, QVector3D(0, 0, 0))
    STATS_PROPERTY(float, speed, 0)
    STATS_PROPERTY(float, yaw, 0)
    STATS_PROPERTY(int, avatarMixerInKbps, 0)
    STATS_PROPERTY(int, avatarMixerInPps, 0)
    STATS_PROPERTY(int, avatarMixerOutKbps, 0)
    STATS_PROPERTY(int, avatarMixerOutPps, 0)
    STATS_PROPERTY(float, myAvatarSendRate, 0)

    STATS_PROPERTY(int, audioMixerInKbps, 0)
    STATS_PROPERTY(int, audioMixerInPps, 0)
    STATS_PROPERTY(int, audioMixerOutKbps, 0)
    STATS_PROPERTY(int, audioMixerOutPps, 0)
    STATS_PROPERTY(int, audioMixerKbps, 0)
    STATS_PROPERTY(int, audioMixerPps, 0)
    STATS_PROPERTY(int, audioOutboundPPS, 0)
    STATS_PROPERTY(int, audioSilentOutboundPPS, 0)
    STATS_PROPERTY(int, audioInboundPPS, 0)
    STATS_PROPERTY(int, audioAudioInboundPPS, 0)
    STATS_PROPERTY(int, audioSilentInboundPPS, 0)
    STATS_PROPERTY(int, audioPacketLoss, 0)
    STATS_PROPERTY(QString, audioCodec, QString())
    STATS_PROPERTY(QString, audioNoiseGate, QString())
    STATS_PROPERTY(QVector2D, audioInjectors, QVector2D());
    STATS_PROPERTY(int, entityPacketsInKbps, 0)

    STATS_PROPERTY(int, downloads, 0)
    STATS_PROPERTY(int, downloadLimit, 0)
    STATS_PROPERTY(int, downloadsPending, 0)
    Q_PROPERTY(QStringList downloadUrls READ downloadUrls NOTIFY downloadUrlsChanged)
    STATS_PROPERTY(int, processing, 0)
    STATS_PROPERTY(int, processingPending, 0)
    STATS_PROPERTY(int, triangles, 0)
    STATS_PROPERTY(quint32 , drawcalls, 0)
    STATS_PROPERTY(int, materialSwitches, 0)
    STATS_PROPERTY(int, itemConsidered, 0)
    STATS_PROPERTY(int, itemOutOfView, 0)
    STATS_PROPERTY(int, itemTooSmall, 0)
    STATS_PROPERTY(int, itemRendered, 0)
    STATS_PROPERTY(int, shadowConsidered, 0)
    STATS_PROPERTY(int, shadowOutOfView, 0)
    STATS_PROPERTY(int, shadowTooSmall, 0)
    STATS_PROPERTY(int, shadowRendered, 0)
    STATS_PROPERTY(QString, sendingMode, QString())
    STATS_PROPERTY(QString, packetStats, QString())
    STATS_PROPERTY(int, lodAngle, 0)
    STATS_PROPERTY(int, lodTargetFramerate, 0)
    STATS_PROPERTY(QString, lodStatus, QString())
    STATS_PROPERTY(quint64, numEntityUpdates, 0)
    STATS_PROPERTY(quint64, numNeededEntityUpdates, 0)
    STATS_PROPERTY(QString, timingStats, QString())
    STATS_PROPERTY(QString, gameUpdateStats, QString())
    STATS_PROPERTY(int, serverElements, 0)
    STATS_PROPERTY(int, serverInternal, 0)
    STATS_PROPERTY(int, serverLeaves, 0)
    STATS_PROPERTY(int, localElements, 0)
    STATS_PROPERTY(int, localInternal, 0)
    STATS_PROPERTY(int, localLeaves, 0)
    STATS_PROPERTY(int, rectifiedTextureCount, 0)
    STATS_PROPERTY(int, decimatedTextureCount, 0)

    STATS_PROPERTY(int, gpuBuffers, 0)
    STATS_PROPERTY(int, gpuBufferMemory, 0)
    STATS_PROPERTY(int, gpuTextures, 0)
    STATS_PROPERTY(int, glContextSwapchainMemory, 0)
    STATS_PROPERTY(int, qmlTextureMemory, 0)
    STATS_PROPERTY(int, texturePendingTransfers, 0)
    STATS_PROPERTY(int, gpuTextureMemory, 0)
    STATS_PROPERTY(int, gpuTextureResidentMemory, 0)
    STATS_PROPERTY(int, gpuTextureFramebufferMemory, 0)
    STATS_PROPERTY(int, gpuTextureResourceMemory, 0)
    STATS_PROPERTY(int, gpuTextureResourceIdealMemory, 0)
    STATS_PROPERTY(int, gpuTextureResourcePopulatedMemory, 0)
    STATS_PROPERTY(int, gpuTextureExternalMemory, 0)
    STATS_PROPERTY(QString, gpuTextureMemoryPressureState, QString())
    STATS_PROPERTY(int, gpuFreeMemory, 0)
    STATS_PROPERTY(QVector2D, gpuFrameSize, QVector2D(0,0))
    STATS_PROPERTY(float, gpuFrameTime, 0)
    STATS_PROPERTY(float, gpuFrameTimePerPixel, 0)
    STATS_PROPERTY(float, batchFrameTime, 0)
    STATS_PROPERTY(float, engineFrameTime, 0)
    STATS_PROPERTY(float, avatarSimulationTime, 0)

    STATS_PROPERTY(int, stylusPicksCount, 0)
    STATS_PROPERTY(int, rayPicksCount, 0)
    STATS_PROPERTY(int, parabolaPicksCount, 0)
    STATS_PROPERTY(int, collisionPicksCount, 0)
    STATS_PROPERTY(QVector3D, stylusPicksUpdated, QVector3D(0, 0, 0))
    STATS_PROPERTY(QVector3D, rayPicksUpdated, QVector3D(0, 0, 0))
    STATS_PROPERTY(QVector3D, parabolaPicksUpdated, QVector3D(0, 0, 0))
    STATS_PROPERTY(QVector3D, collisionPicksUpdated, QVector3D(0, 0, 0))

    STATS_PROPERTY(int, mainThreadQueueDepth, -1);
    STATS_PROPERTY(int, nodeListThreadQueueDepth, -1);

#ifdef DEBUG_EVENT_QUEUE
    STATS_PROPERTY(bool, eventQueueDebuggingOn, true)
#else
    STATS_PROPERTY(bool, eventQueueDebuggingOn, false)
#endif // DEBUG_EVENT_QUEUE

public:
    static Stats* getInstance();

    Stats(QQuickItem* parent = nullptr);
    bool includeTimingRecord(const QString& name);
    void setRenderDetails(const render::RenderDetails& details);
    const QString& monospaceFont() {
        return _monospaceFont;
    }

    void updateStats(bool force = false);

    bool isExpanded() { return _expanded; }
    bool isTimingExpanded() { return _showTimingDetails; }

    void setExpanded(bool expanded) {
        if (_expanded != expanded) {
            _expanded = expanded;
            emit expandedChanged();
        }
    }

    QStringList downloadUrls () { return _downloadUrls; }

public slots:

    /*@jsdoc
     * Updates statistics to make current values available to scripts even though the statistics overlay may not be displayed.
     * (Many statistics values are normally updated only if the statistics overlay is displayed.)
     * <p><strong>Note:</strong> Not all statistics values are updated when the statistics overlay isn't displayed or 
     * expanded.</p>
     * @function Stats.forceUpdateStats
     * @example <caption>Report avatar mixer data and packet rates.</caption>
     * // The statistics to report.
     * var stats = [
     *     "avatarMixerInKbps",
     *     "avatarMixerInPps",
     *     "avatarMixerOutKbps",
     *     "avatarMixerOutPps"
     * ];
     * 
     * // Update the statistics for the script.
     * Stats.forceUpdateStats();
     * 
     * // Report the statistics.
     * for (var i = 0; i < stats.length; i++) {
     *     print(stats[i], "=", Stats[stats[i]]);
     * }
     */
    void forceUpdateStats() { updateStats(true); }

signals:

    // Signals for properties...

    /*@jsdoc
     * Triggered when the value of the <code>expanded</code> property changes.
     * @function Stats.expandedChanged
     * @returns {Signal}
     */
    void expandedChanged();

    /*@jsdoc
     * Triggered when the value of the <code>timingExpanded</code> property changes.
     * @function Stats.timingExpandedChanged
     * @returns {Signal}
     */
    void timingExpandedChanged();

    /*@jsdoc
     * Triggered when the value of the <code>serverCount</code> property changes.
     * @function Stats.serverCountChanged
     * @returns {Signal}
     */
    void serverCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>renderrate</code> property changes.
     * @function Stats.renderrateChanged
     * @returns {Signal}
     */
    void renderrateChanged();

    /*@jsdoc
     * Triggered when the value of the <code>presentrate</code> property changes.
     * @function Stats.presentrateChanged
     * @returns {Signal}
     */
    void presentrateChanged();

    /*@jsdoc
     * Triggered when the value of the <code>stutterrate</code> property changes.
     * @function Stats.stutterrateChanged
     * @returns {Signal}
     */
    void stutterrateChanged();

    /*@jsdoc
     * Triggered when the value of the <code>appdropped</code> property changes.
     * @function Stats.appdroppedChanged
     * @returns {Signal}
     */
    void appdroppedChanged();

    /*@jsdoc
     * Triggered when the value of the <code>longsubmits</code> property changes.
     * @function Stats.longsubmitsChanged
     * @returns {Signal}
     */
    void longsubmitsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>longrenders</code> property changes.
     * @function Stats.longrendersChanged
     * @returns {Signal}
     */
    void longrendersChanged();

    /*@jsdoc
     * Triggered when the value of the <code>longframes</code> property changes.
     * @function Stats.longframesChanged
     * @returns {Signal}
     */
    void longframesChanged();

    /*@jsdoc
     * Triggered when the value of the <code>presentnewrate</code> property changes.
     * @function Stats.presentnewrateChanged
     * @returns {Signal}
     */
    void presentnewrateChanged();

    /*@jsdoc
     * Triggered when the value of the <code>presentdroprate</code> property changes.
     * @function Stats.presentdroprateChanged
     * @returns {Signal}
     */
    void presentdroprateChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gameLoopRate</code> property changes.
     * @function Stats.gameLoopRateChanged
     * @returns {Signal}
     */
    void gameLoopRateChanged();

    /*@jsdoc
     * Triggered when the value of the <code>avatarCount</code> property changes.
     * @function Stats.avatarCountChanged
     * @returns {Signal}
     */
    void avatarCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>refreshRateTarget</code> property changes.
     * @function Stats.refreshRateTargetChanged
     * @returns {Signal}
     */
    void refreshRateTargetChanged();

    /*@jsdoc
     * Triggered when the value of the <code>refreshRateMode</code> property changes.
     * @function Stats.refreshRateModeChanged
     * @returns {Signal}
     */
    void refreshRateModeChanged();

    /*@jsdoc
     * Triggered when the value of the <code>refreshRateRegime</code> property changes.
     * @function Stats.refreshRateRegimeChanged
     * @returns {Signal}
     */
    void refreshRateRegimeChanged();

    /*@jsdoc
     * Triggered when the value of the <code>uxMode</code> property changes.
     * @function Stats.uxModeChanged
     * @returns {Signal}
     */
    void uxModeChanged();

    /*@jsdoc
     * Triggered when the value of the <code>heroAvatarCount</code> property changes.
     * @function Stats.heroAvatarCountChanged
     * @returns {Signal}
     */
    void heroAvatarCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>physicsObjectCount</code> property changes.
     * @function Stats.physicsObjectCountChanged
     * @returns {Signal}
     */
    void physicsObjectCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>updatedAvatarCount</code> property changes.
     * @function Stats.updatedAvatarCountChanged
     * @returns {Signal}
     */
    void updatedAvatarCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>updatedHeroAvatarCount</code> property changes.
     * @function Stats.updatedHeroAvatarCountChanged
     * @returns {Signal}
     */
    void updatedHeroAvatarCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>notUpdatedAvatarCount</code> property changes.
     * @function Stats.notUpdatedAvatarCountChanged
     * @returns {Signal}
     */
    void notUpdatedAvatarCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>packetInCount</code> property changes.
     * @function Stats.packetInCountChanged
     * @returns {Signal}
     */
    void packetInCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>packetOutCount</code> property changes.
     * @function Stats.packetOutCountChanged
     * @returns {Signal}
     */
    void packetOutCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>mbpsIn</code> property changes.
     * @function Stats.mbpsInChanged
     * @returns {Signal}
     */
    void mbpsInChanged();

    /*@jsdoc
     * Triggered when the value of the <code>mbpsOut</code> property changes.
     * @function Stats.mbpsOutChanged
     * @returns {Signal}
     */
    void mbpsOutChanged();

    /*@jsdoc
     * Triggered when the value of the <code>assetMbpsIn</code> property changes.
     * @function Stats.assetMbpsInChanged
     * @returns {Signal}
     */
    void assetMbpsInChanged();

    /*@jsdoc
     * Triggered when the value of the <code>assetMbpsOut</code> property changes.
     * @function Stats.assetMbpsOutChanged
     * @returns {Signal}
     */
    void assetMbpsOutChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioPing</code> property changes.
     * @function Stats.audioPingChanged
     * @returns {Signal}
     */
    void audioPingChanged();

    /*@jsdoc
     * Triggered when the value of the <code>avatarPing</code> property changes.
     * @function Stats.avatarPingChanged
     * @returns {Signal}
     */
    void avatarPingChanged();

    /*@jsdoc
     * Triggered when the value of the <code>entitiesPing</code> property changes.
     * @function Stats.entitiesPingChanged
     * @returns {Signal}
     */
    void entitiesPingChanged();

    /*@jsdoc
     * Triggered when the value of the <code>assetPing</code> property changes.
     * @function Stats.assetPingChanged
     * @returns {Signal}
     */
    void assetPingChanged();

    /*@jsdoc
     * Triggered when the value of the <code>messagePing</code> property changes.
     * @function Stats.messagePingChanged
     * @returns {Signal}
     */
    void messagePingChanged();

    /*@jsdoc
     * Triggered when the value of the <code>position</code> property changes.
     * @function Stats.positionChanged
     * @returns {Signal}
     */
    void positionChanged();

    /*@jsdoc
     * Triggered when the value of the <code>speed</code> property changes.
     * @function Stats.speedChanged
     * @returns {Signal}
     */
    void speedChanged();

    /*@jsdoc
     * Triggered when the value of the <code>yaw</code> property changes.
     * @function Stats.yawChanged
     * @returns {Signal}
     */
    void yawChanged();

    /*@jsdoc
     * Triggered when the value of the <code>avatarMixerInKbps</code> property changes.
     * @function Stats.avatarMixerInKbpsChanged
     * @returns {Signal}
     */
    void avatarMixerInKbpsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>avatarMixerInPps</code> property changes.
     * @function Stats.avatarMixerInPpsChanged
     * @returns {Signal}
     */
    void avatarMixerInPpsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>avatarMixerOutKbps</code> property changes.
     * @function Stats.avatarMixerOutKbpsChanged
     * @returns {Signal}
     */
    void avatarMixerOutKbpsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>avatarMixerOutPps</code> property changes.
     * @function Stats.avatarMixerOutPpsChanged
     * @returns {Signal}
     */
    void avatarMixerOutPpsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>myAvatarSendRate</code> property changes.
     * @function Stats.myAvatarSendRateChanged
     * @returns {Signal}
     */
    void myAvatarSendRateChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioMixerInKbps</code> property changes.
     * @function Stats.audioMixerInKbpsChanged
     * @returns {Signal}
     */
    void audioMixerInKbpsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioMixerInPps</code> property changes.
     * @function Stats.audioMixerInPpsChanged
     * @returns {Signal}
     */
    void audioMixerInPpsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioMixerOutKbps</code> property changes.
     * @function Stats.audioMixerOutKbpsChanged
     * @returns {Signal}
     */
    void audioMixerOutKbpsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioMixerOutPps</code> property changes.
     * @function Stats.audioMixerOutPpsChanged
     * @returns {Signal}
     */
    void audioMixerOutPpsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioMixerKbps</code> property changes.
     * @function Stats.audioMixerKbpsChanged
     * @returns {Signal}
     */
    void audioMixerKbpsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioMixerPps</code> property changes.
     * @function Stats.audioMixerPpsChanged
     * @returns {Signal}
     */
    void audioMixerPpsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioOutboundPPS</code> property changes.
     * @function Stats.audioOutboundPPSChanged
     * @returns {Signal}
     */
    void audioOutboundPPSChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioSilentOutboundPPS</code> property changes.
     * @function Stats.audioSilentOutboundPPSChanged
     * @returns {Signal}
     */
    void audioSilentOutboundPPSChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioInboundPPS</code> property changes.
     * @function Stats.audioInboundPPSChanged
     * @returns {Signal}
     */
    void audioInboundPPSChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioAudioInboundPPS</code> property changes.
     * @function Stats.audioAudioInboundPPSChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed. Use 
     *     {@link Stats.audioInboundPPSChanged|audioInboundPPSChanged} instead.
     */
    void audioAudioInboundPPSChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioSilentInboundPPS</code> property changes.
     * @function Stats.audioSilentInboundPPSChanged
     * @returns {Signal}
     */
    void audioSilentInboundPPSChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioPacketLoss</code> property changes.
     * @function Stats.audioPacketLossChanged
     * @returns {Signal}
     */
    void audioPacketLossChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioCodec</code> property changes.
     * @function Stats.audioCodecChanged
     * @returns {Signal}
     */
    void audioCodecChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioNoiseGate</code> property changes.
     * @function Stats.audioNoiseGateChanged
     * @returns {Signal}
     */
    void audioNoiseGateChanged();

    /*@jsdoc
     * Triggered when the value of the <code>audioInjectors</code> property changes.
     * @function Stats.audioInjectorsChanged
     * @returns {Signal}
     */
    void audioInjectorsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>entityPacketsInKbps</code> property changes.
     * @function Stats.entityPacketsInKbpsChanged
     * @returns {Signal}
     */
    void entityPacketsInKbpsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>downloads</code> property changes.
     * @function Stats.downloadsChanged
     * @returns {Signal}
     */
    void downloadsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>downloadLimit</code> property changes.
     * @function Stats.downloadLimitChanged
     * @returns {Signal}
     */
    void downloadLimitChanged();

    /*@jsdoc
     * Triggered when the value of the <code>downloadsPending</code> property changes.
     * @function Stats.downloadsPendingChanged
     * @returns {Signal}
     */
    void downloadsPendingChanged();

    /*@jsdoc
     * Triggered when the value of the <code>downloadUrls</code> property changes.
     * @function Stats.downloadUrlsChanged
     * @returns {Signal}
     */
    void downloadUrlsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>processing</code> property changes.
     * @function Stats.processingChanged
     * @returns {Signal}
     */
    void processingChanged();

    /*@jsdoc
     * Triggered when the value of the <code>processingPending</code> property changes.
     * @function Stats.processingPendingChanged
     * @returns {Signal}
     */
    void processingPendingChanged();

    /*@jsdoc
     * Triggered when the value of the <code>triangles</code> property changes.
     * @function Stats.trianglesChanged
     * @returns {Signal}
     */
    void trianglesChanged();

    /*@jsdoc
     * Triggered when the value of the <code>drawcalls</code> property changes.
     * @function Stats.drawcallsChanged
     * @returns {Signal}
     */
    void drawcallsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>materialSwitches</code> property changes.
     * @function Stats.materialSwitchesChanged
     * @returns {Signal}
     */
    void materialSwitchesChanged();

    /*@jsdoc
     * Triggered when the value of the <code>itemConsidered</code> property changes.
     * @function Stats.itemConsideredChanged
     * @returns {Signal}
     */
    void itemConsideredChanged();

    /*@jsdoc
     * Triggered when the value of the <code>itemOutOfView</code> property changes.
     * @function Stats.itemOutOfViewChanged
     * @returns {Signal}
     */
    void itemOutOfViewChanged();

    /*@jsdoc
     * Triggered when the value of the <code>itemTooSmall</code> property changes.
     * @function Stats.itemTooSmallChanged
     * @returns {Signal}
     */
    void itemTooSmallChanged();

    /*@jsdoc
     * Triggered when the value of the <code>itemRendered</code> property changes.
     * @function Stats.itemRenderedChanged
     * @returns {Signal}
     */
    void itemRenderedChanged();

    /*@jsdoc
     * Triggered when the value of the <code>shadowConsidered</code> property changes.
     * @function Stats.shadowConsideredChanged
     * @returns {Signal}
     */
    void shadowConsideredChanged();

    /*@jsdoc
     * Triggered when the value of the <code>shadowOutOfView</code> property changes.
     * @function Stats.shadowOutOfViewChanged
     * @returns {Signal}
     */
    void shadowOutOfViewChanged();

    /*@jsdoc
     * Triggered when the value of the <code>shadowTooSmall</code> property changes.
     * @function Stats.shadowTooSmallChanged
     * @returns {Signal}
     */
    void shadowTooSmallChanged();

    /*@jsdoc
     * Triggered when the value of the <code>shadowRendered</code> property changes.
     * @function Stats.shadowRenderedChanged
     * @returns {Signal}
     */
    void shadowRenderedChanged();

    /*@jsdoc
     * Triggered when the value of the <code>sendingMode</code> property changes.
     * @function Stats.sendingModeChanged
     * @returns {Signal}
     */
    void sendingModeChanged();

    /*@jsdoc
     * Triggered when the value of the <code>packetStats</code> property changes.
     * @function Stats.packetStatsChanged
     * @returns {Signal}
     */
    void packetStatsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>lodAngle</code> property changes.
     * @function Stats.lodAngleChanged
     * @returns {Signal}
     */
    void lodAngleChanged();

    /*@jsdoc
     * Triggered when the value of the <code>lodTargetFramerate</code> property changes.
     * @function Stats.lodTargetFramerateChanged
     * @returns {Signal}
     */
    void lodTargetFramerateChanged();

    /*@jsdoc
     * Triggered when the value of the <code>lodStatus</code> property changes.
     * @function Stats.lodStatusChanged
     * @returns {Signal}
     */
    void lodStatusChanged();

    /*@jsdoc
     * Triggered when the value of the <code>numEntityUpdates</code> property changes.
     * @function Stats.numEntityUpdatesChanged
     * @returns {Signal}
     */
    void numEntityUpdatesChanged();

    /*@jsdoc
     * Triggered when the value of the <code>numNeededEntityUpdates</code> property changes.
     * @function Stats.numNeededEntityUpdatesChanged
     * @returns {Signal}
     */
    void numNeededEntityUpdatesChanged();

    /*@jsdoc
     * Triggered when the value of the <code>timingStats</code> property changes.
     * @function Stats.timingStatsChanged
     * @returns {Signal}
     */
    void timingStatsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gameUpdateStats</code> property changes.
     * @function Stats.gameUpdateStatsChanged
     * @returns {Signal}
     */
    void gameUpdateStatsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>serverElements</code> property changes.
     * @function Stats.serverElementsChanged
     * @returns {Signal}
     */
    void serverElementsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>serverInternal</code> property changes.
     * @function Stats.serverInternalChanged
     * @returns {Signal}
     */
    void serverInternalChanged();

    /*@jsdoc
     * Triggered when the value of the <code>serverLeaves</code> property changes.
     * @function Stats.serverLeavesChanged
     * @returns {Signal}
     */
    void serverLeavesChanged();

    /*@jsdoc
     * Triggered when the value of the <code>localElements</code> property changes.
     * @function Stats.localElementsChanged
     * @returns {Signal}
     */
    void localElementsChanged();

    /*@jsdoc
     * Triggered when the value of the <code>localInternal</code> property changes.
     * @function Stats.localInternalChanged
     * @returns {Signal}
     */
    void localInternalChanged();

    /*@jsdoc
     * Triggered when the value of the <code>localLeaves</code> property changes.
     * @function Stats.localLeavesChanged
     * @returns {Signal}
     */
    void localLeavesChanged();

    /*@jsdoc
     * Triggered when the value of the <code>rectifiedTextureCount</code> property changes.
     * @function Stats.rectifiedTextureCountChanged
     * @returns {Signal}
     */
    void rectifiedTextureCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>decimatedTextureCount</code> property changes.
     * @function Stats.decimatedTextureCountChanged
     * @returns {Signal}
     */
    void decimatedTextureCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuBuffers</code> property changes.
     * @function Stats.gpuBuffersChanged
     * @returns {Signal}
     */
    void gpuBuffersChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuBufferMemory</code> property changes.
     * @function Stats.gpuBufferMemoryChanged
     * @returns {Signal}
     */
    void gpuBufferMemoryChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuTextures</code> property changes.
     * @function Stats.gpuTexturesChanged
     * @returns {Signal}
     */
    void gpuTexturesChanged();

    /*@jsdoc
     * Triggered when the value of the <code>glContextSwapchainMemory</code> property changes.
     * @function Stats.glContextSwapchainMemoryChanged
     * @returns {Signal}
     */
    void glContextSwapchainMemoryChanged();

    /*@jsdoc
     * Triggered when the value of the <code>qmlTextureMemory</code> property changes.
     * @function Stats.qmlTextureMemoryChanged
     * @returns {Signal}
     */
    void qmlTextureMemoryChanged();

    /*@jsdoc
     * Triggered when the value of the <code>texturePendingTransfers</code> property changes.
     * @function Stats.texturePendingTransfersChanged
     * @returns {Signal}
     */
    void texturePendingTransfersChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuTextureMemory</code> property changes.
     * @function Stats.gpuTextureMemoryChanged
     * @returns {Signal}
     */
    void gpuTextureMemoryChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuTextureResidentMemory</code> property changes.
     * @function Stats.gpuTextureResidentMemoryChanged
     * @returns {Signal}
     */
    void gpuTextureResidentMemoryChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuTextureFramebufferMemory</code> property changes.
     * @function Stats.gpuTextureFramebufferMemoryChanged
     * @returns {Signal}
     */
    void gpuTextureFramebufferMemoryChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuTextureResourceMemory</code> property changes.
     * @function Stats.gpuTextureResourceMemoryChanged
     * @returns {Signal}
     */
    void gpuTextureResourceMemoryChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuTextureResourceIdealMemory</code> property changes.
     * @function Stats.gpuTextureResourceIdealMemoryChanged
     * @returns {Signal}
     */
    void gpuTextureResourceIdealMemoryChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuTextureResourcePopulatedMemory</code> property changes.
     * @function Stats.gpuTextureResourcePopulatedMemoryChanged
     * @returns {Signal}
     */
    void gpuTextureResourcePopulatedMemoryChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuTextureExternalMemory</code> property changes.
     * @function Stats.gpuTextureExternalMemoryChanged
     * @returns {Signal}
     */
    void gpuTextureExternalMemoryChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuTextureMemoryPressureState</code> property changes.
     * @function Stats.gpuTextureMemoryPressureStateChanged
     * @returns {Signal}
     */
    void gpuTextureMemoryPressureStateChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuFreeMemory</code> property changes.
     * @function Stats.gpuFreeMemoryChanged
     * @returns {Signal}
     */
    void gpuFreeMemoryChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuFrameSize</code> property changes.
     * @function Stats.gpuFrameSizeChanged
     * @returns {Signal}
     */
    void gpuFrameSizeChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuFrameTime</code> property changes.
     * @function Stats.gpuFrameTimeChanged
     * @returns {Signal}
     */
    void gpuFrameTimeChanged();

    /*@jsdoc
     * Triggered when the value of the <code>gpuFrameTimePerPixel</code> property changes.
     * @function Stats.gpuFrameTimePerPixelChanged
     * @returns {Signal}
     */
    void gpuFrameTimePerPixelChanged();

    /*@jsdoc
     * Triggered when the value of the <code>batchFrameTime</code> property changes.
     * @function Stats.batchFrameTimeChanged
     * @returns {Signal}
     */
    void batchFrameTimeChanged();

    /*@jsdoc
     * Triggered when the value of the <code>engineFrameTime</code> property changes.
     * @function Stats.engineFrameTimeChanged
     * @returns {Signal}
     */
    void engineFrameTimeChanged();

    /*@jsdoc
     * Triggered when the value of the <code>avatarSimulationTime</code> property changes.
     * @function Stats.avatarSimulationTimeChanged
     * @returns {Signal}
     */
    void avatarSimulationTimeChanged();

    /*@jsdoc
     * Triggered when the value of the <code>stylusPicksCount</code> property changes.
     * @function Stats.stylusPicksCountChanged
     * @returns {Signal}
     */
    void stylusPicksCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>rayPicksCount</code> property changes.
     * @function Stats.rayPicksCountChanged
     * @returns {Signal}
     */
    void rayPicksCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>parabolaPicksCount</code> property changes.
     * @function Stats.parabolaPicksCountChanged
     * @returns {Signal}
     */
    void parabolaPicksCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>collisionPicksCount</code> property changes.
     * @function Stats.collisionPicksCountChanged
     * @returns {Signal}
     */
    void collisionPicksCountChanged();

    /*@jsdoc
     * Triggered when the value of the <code>stylusPicksUpdated</code> property changes.
     * @function Stats.stylusPicksUpdatedChanged
     * @returns {Signal}
     */
    void stylusPicksUpdatedChanged();

    /*@jsdoc
     * Triggered when the value of the <code>rayPicksUpdated</code> property changes.
     * @function Stats.rayPicksUpdatedChanged
     * @returns {Signal}
     */
    void rayPicksUpdatedChanged();

    /*@jsdoc
     * Triggered when the value of the <code>parabolaPicksUpdated</code> property changes.
     * @function Stats.parabolaPicksUpdatedChanged
     * @returns {Signal}
     */
    void parabolaPicksUpdatedChanged();

    /*@jsdoc
     * Triggered when the value of the <code>collisionPicksUpdated</code> property changes.
     * @function Stats.collisionPicksUpdatedChanged
     * @returns {Signal}
     */
    void collisionPicksUpdatedChanged();

    /*@jsdoc
     * Triggered when the value of the <code>mainThreadQueueDepth</code> property changes.
     * @function Stats.mainThreadQueueDepthChanged
     * @returns {Signal}
     */
    void mainThreadQueueDepthChanged();

    /*@jsdoc
     * Triggered when the value of the <code>nodeListThreadQueueDepth</code> property changes.
     * @function Stats.nodeListThreadQueueDepth
     * @returns {Signal}
     */
    void nodeListThreadQueueDepthChanged();

    /*@jsdoc
     * Triggered when the value of the <code>eventQueueDebuggingOn</code> property changes.
     * @function Stats.eventQueueDebuggingOnChanged
     * @returns {Signal}
     */
    void eventQueueDebuggingOnChanged();


    // Stats.qml signals: shouldn't be in the API.

    /*@jsdoc
     * Triggered when the value of the <code>bgColor</code> property changes.
     * @function Stats.bgColorChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */


    // QQuickItem signals: shouldn't be in the API.

    /*@jsdoc
     * Triggered when the value of the <code>activeFocus</code> property changes.
     * @function Stats.activeFocusChanged
     * @param {boolean} activeFocus - Active focus.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>activeFocusOnTab</code> property changes.
     * @function Stats.activeFocusOnTabChanged
     * @param {boolean} activeFocusOnTab - Active focus on tab.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>antialiasing</code> property changes.
     * @function Stats.antialiasingChanged
     * @param {boolean} antialiasing - Antialiasing.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>baselineOffset</code> property changes.
     * @function Stats.baselineOffsetChanged
     * @param {number} baselineOffset - Baseline offset.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>children</code> property changes.
     * @function Stats.childrenChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the position and size of the rectangle containing the children changes.
     * @function Stats.childrenRectChanged
     * @param {Rect} childrenRect - Children rect.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */


    /*@jsdoc
     * Triggered when the value of the <code>clip</code> property changes.
     * @function Stats.clipChanged
     * @param {boolean} clip - Clip.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>containmentMask</code> property changes.
     * @function Stats.containmentMaskChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>enabled</code> property changes.
     * @function Stats.enabledChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>focus</code> property changes.
     * @function Stats.focusChanged
     * @param {boolean} focus - Focus.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>height</code> property changes.
     * @function Stats.heightChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>implicitHeight</code> property changes.
     * @function Stats.implicitHeightChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>implicitWidth</code> property changes.
     * @function Stats.implicitWidthChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>opacity</code> property changes.
     * @function Stats.opacityChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the parent item changes.
     * @function Stats.parentChanged
     * @param {object} parent - Parent.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>rotation</code> property changes.
     * @function Stats.rotationChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>scale</code> property changes.
     * @function Stats.scaleChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>smooth</code> property changes.
     * @function Stats.smoothChanged
     * @param {boolean} smooth - Smooth.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>state</code> property changes.
     * @function Stats.stateChanged
     * @paramm {string} state - State.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>transformOrigin</code> property changes.
     * @function Stats.transformOriginChanged
     * @param {number} transformOrigin - Transformm origin.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>visibleChanged</code> property changes.
     * @function Stats.visibleChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the list of visible children changes.
     * @function Stats.visibleChildrenChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>width</code> property changes.
     * @function Stats.widthChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the stats window changes.
     * @function Stats.windowChanged
     * @param {object} window - Window.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>x</code> property changes.
     * @function Stats.xChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>y</code> property changes.
     * @function Stats.yChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */

    /*@jsdoc
     * Triggered when the value of the <code>z</code> property changes.
     * @function Stats.zChanged
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */


    // QQuickItem methods: shouldn't be in the API.

    /*@jsdoc
     * @function Stats.childAt
     * @param {number} x - X.
     * @param {number} y - Y.
     * @returns {object}
     * @deprecated This method is deprecated and will be removed.
     */

    /*@jsdoc
     * @function Stats.contains
     * @param {Vec2} point - Point
     * @returns {boolean}
     * @deprecated This method is deprecated and will be removed.
     */

    /*@jsdoc
     * @function Stats.forceActiveFocus
     * @param {number} [reason=7] - Reason
     * @deprecated This method is deprecated and will be removed.
     */

    /*@jsdoc
     * @function Stats.grabToImage
     * @param {object} callback - Callback.
     * @param {Size} [targetSize=0,0] - Target size.
     * @returns {boolean}
     * @deprecated This method is deprecated and will be removed.
     */

    /*@jsdoc
     * @function Stats.mapFromGlobal
     * @param {object} global - Global.
     * @deprecated This method is deprecated and will be removed.
     */

    /*@jsdoc
     * @function Stats.mapFromItem
     * @param {object} item - Item.
     * @deprecated This method is deprecated and will be removed.
     */

    /*@jsdoc
     * @function Stats.mapToGlobal
     * @param {object} global - Global.
     * @deprecated This method is deprecated and will be removed.
     */

    /*@jsdoc
     * @function Stats.mapToItem
     * @param {object} item - Item
     * @deprecated This method is deprecated and will be removed.
     */

    /*@jsdoc
     * @function Stats.nextItemInFocusChain
     * @param {boolean} [forward=true] - Forward.
     * @returns {object}
     * @deprecated This method is deprecated and will be removed.
     */

    /*@jsdoc
     * @function Stats.update
     * @deprecated This method is deprecated and will be removed.
     */

private:
    int _recentMaxPackets{ 0 } ; // recent max incoming voxel packets to process
    bool _resetRecentMaxPacketsSoon{ true };
    bool _expanded{ false };
    bool _showTimingDetails{ false };
    bool _showGameUpdateStats{ false };
    QString _monospaceFont;
    const AudioIOStats* _audioStats;
    QStringList _downloadUrls = QStringList();
};

#endif // hifi_Stats_h
