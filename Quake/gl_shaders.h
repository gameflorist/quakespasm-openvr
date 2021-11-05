/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

////////////////////////////////////////////////////////////////
//
// Debug options
//
////////////////////////////////////////////////////////////////

#define SHOW_ACTIVE_LIGHT_CLUSTERS 0

////////////////////////////////////////////////////////////////
//
// GUI
//
////////////////////////////////////////////////////////////////

static const char gui_vertex_shader[] =
"layout(location=0) in vec2 in_pos;\n"
"layout(location=1) in vec2 in_uv;\n"
"layout(location=2) in vec4 in_color;\n"
"\n"
"layout(location=0) out vec2 out_uv;\n"
"layout(location=1) out vec4 out_color;\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = vec4(in_pos, -1.0, 1.0);\n"
"	out_uv = in_uv;\n"
"	out_color = in_color;\n"
"}\n";
	
////////////////////////////////////////////////////////////////

static const char gui_fragment_shader[] =
"layout(binding=0) uniform sampler2D Tex;\n"
"\n"
"layout(location=0) centroid in vec2 in_uv;\n"
"layout(location=1) centroid in vec4 in_color;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	out_fragcolor = texture(Tex, in_uv) * in_color;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// View blend
//
////////////////////////////////////////////////////////////////

static const char viewblend_vertex_shader[] =
"void main()\n"
"{\n"
"	ivec2 v = ivec2(gl_VertexID & 1, gl_VertexID >> 1);\n"
"	v.x ^= v.y; // fix winding order\n"
"	gl_Position = vec4(vec2(v) * 2.0 - 1.0, 0.0, 1.0);\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char viewblend_fragment_shader[] =
"layout(location=0) uniform vec4 Color;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	out_fragcolor = Color;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// View warp/scale
//
////////////////////////////////////////////////////////////////

static const char warpscale_vertex_shader[] =
"layout(location=0) out vec2 out_uv;\n"
"\n"
"void main()\n"
"{\n"
"	ivec2 v = ivec2(gl_VertexID & 1, gl_VertexID >> 1);\n"
"	v.x ^= v.y; // fix winding order\n"
"	out_uv = vec2(v);\n"
"	gl_Position = vec4(out_uv * 2.0 - 1.0, 0.0, 1.0);\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char warpscale_fragment_shader[] =
"layout(binding=0) uniform sampler2D Tex;\n"
"\n"
"layout(location=0) uniform vec4 UVScaleWarpTime; // xy=Scale z=Warp w=Time\n"
"\n"
"layout(location=0) in vec2 in_uv;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	vec2 uv = in_uv;\n"
"	vec2 uv_scale = UVScaleWarpTime.xy;\n"
"	if (UVScaleWarpTime.z > 0.0)\n"
"	{\n"
"		float time = UVScaleWarpTime.w;\n"
"		float aspect = dFdy(uv.y) / dFdx(uv.x);\n"
"		vec2 warp_amp = UVScaleWarpTime.zz;\n"
"		warp_amp.y *= aspect;\n"
"		uv = warp_amp + uv * (1.0 - 2.0 * warp_amp); // remap to safe area\n"
"		uv += warp_amp * sin(vec2(uv.y / aspect, uv.x) * (3.14159265 * 8.0) + time);\n"
"	}\n"
"	out_fragcolor = texture(Tex, uv * uv_scale);\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Postprocess (gamma/contrast)
//
////////////////////////////////////////////////////////////////

static const char postprocess_vertex_shader[] =
"void main()\n"
"{\n"
"	ivec2 v = ivec2(gl_VertexID & 1, gl_VertexID >> 1);\n"
"	v.x ^= v.y; // fix winding order\n"
"	gl_Position = vec4(vec2(v) * 2.0 - 1.0, 0.0, 1.0);\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char postprocess_fragment_shader[] =
"layout(binding=0) uniform sampler2D GammaTexture;\n"
"\n"
"layout(location=0) uniform vec2 GammaContrast;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	out_fragcolor = texelFetch(GammaTexture, ivec2(gl_FragCoord), 0);\n"
"	out_fragcolor.rgb *= GammaContrast.y;\n"
"	out_fragcolor = vec4(pow(out_fragcolor.rgb, vec3(GammaContrast.x)), 1.0);\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Common shader snippets
//
////////////////////////////////////////////////////////////////

#define FRAMEDATA_BUFFER \
"#define LIGHT_TILES_X " QS_STRINGIFY (LIGHT_TILES_X) "\n"\
"#define LIGHT_TILES_Y " QS_STRINGIFY (LIGHT_TILES_Y) "\n"\
"#define LIGHT_TILES_Z " QS_STRINGIFY (LIGHT_TILES_Z) "\n"\
"#define MAX_LIGHTS    " QS_STRINGIFY (MAX_DLIGHTS)   "\n"\
"\n"\
"struct Light\n"\
"{\n"\
"	vec3	origin;\n"\
"	float	radius;\n"\
"	vec3	color;\n"\
"	float	minlight;\n"\
"};\n"\
"\n"\
"layout(std430, binding=0) restrict readonly buffer FrameDataBuffer\n"\
"{\n"\
"	mat4	ViewProj;\n"\
"	vec3	FogColor;\n"\
"	float	FogDensity;\n"\
"	float	Time;\n"\
"	float	ZLogScale;\n"\
"	float	ZLogBias;\n"\
"	uint	NumLights;\n"\
"	Light	Lights[];\n"\
"};\n"\

////////////////////////////////////////////////////////////////

#define LIGHT_CLUSTER_IMAGE(mode) \
"layout(rg32ui, binding=0) uniform " mode " uimage3D LightClusters;\n"\

////////////////////////////////////////////////////////////////

#define DRAW_ELEMENTS_INDIRECT_COMMAND \
"struct DrawElementsIndirectCommand\n"\
"{\n"\
"	uint	count;\n"\
"	uint	instanceCount;\n"\
"	uint	firstIndex;\n"\
"	uint	baseVertex;\n"\
"	uint	baseInstance;\n"\
"};\n"\

////////////////////////////////////////////////////////////////

#define WORLD_DRAW_BUFFER \
DRAW_ELEMENTS_INDIRECT_COMMAND \
"\n"\
"layout(std430, binding=1) buffer DrawIndirectBuffer\n"\
"{\n"\
"	DrawElementsIndirectCommand cmds[];\n"\
"};\n"\

////////////////////////////////////////////////////////////////

#define WORLD_CALLDATA_BUFFER \
"struct Call\n"\
"{\n"\
"	uvec2	txhandle;\n"\
"	uvec2	fbhandle;\n"\
"	int		flags;\n"\
"	float	wateralpha;\n"\
"};\n"\
"const int\n"\
"	CF_USE_ALPHA_TEST = 1,\n"\
"	CF_USE_POLYGON_OFFSET = 2\n"\
";\n"\
"\n"\
"layout(std430, binding=1) restrict readonly buffer CallBuffer\n"\
"{\n"\
"	Call call_data[];\n"\
"};\n"\

////////////////////////////////////////////////////////////////

#define WORLD_INSTANCEDATA_BUFFER \
"struct Instance\n"\
"{\n"\
"	vec4	mat[3];\n"\
"	float	alpha;\n"\
"	float	padding;\n"\
"};\n"\
"\n"\
"layout(std430, binding=2) restrict readonly buffer InstanceBuffer\n"\
"{\n"\
"	Instance instance_data[];\n"\
"};\n"\
"\n"\

////////////////////////////////////////////////////////////////

#define WORLD_VERTEX_BUFFER \
"struct PackedVertex\n"\
"{\n"\
"	float data[" QS_STRINGIFY(VERTEXSIZE) "];\n"\
"};\n"\
"\n"\
"layout(std430, binding=3) restrict readonly buffer VertexBuffer\n"\
"{\n"\
"	PackedVertex vertices[];\n"\
"};\n"\
"\n"\

////////////////////////////////////////////////////////////////
//
// World
//
////////////////////////////////////////////////////////////////

static const char world_vertex_shader[] =
FRAMEDATA_BUFFER
WORLD_CALLDATA_BUFFER
WORLD_INSTANCEDATA_BUFFER
WORLD_VERTEX_BUFFER
"\n"
"#if BINDLESS\n"
"	#extension GL_ARB_shader_draw_parameters : require\n"
"	#define DRAW_ID			gl_DrawIDARB\n"
"	#define INSTANCE_ID		(gl_BaseInstanceARB + gl_InstanceID)\n"
"#else\n"
"	#define DRAW_ID			0\n"
"	#define INSTANCE_ID		gl_InstanceID\n"
"#endif\n"
"\n"
"layout(location=0) flat out ivec2 out_drawinstance; // x = draw; y = instance\n"
"layout(location=1) out vec3 out_pos;\n"
"layout(location=2) out vec4 out_uv;\n"
"layout(location=3) out float out_depth;\n"
"layout(location=4) noperspective out vec2 out_coord;\n"
"\n"
"void main()\n"
"{\n"
"	PackedVertex vert = vertices[gl_VertexID];\n"
"	Call call = call_data[DRAW_ID];\n"
"	Instance instance = instance_data[INSTANCE_ID];\n"
"	mat4x3 world = transpose(mat3x4(instance.mat[0], instance.mat[1], instance.mat[2]));\n"
"	out_pos = mat3(world[0], world[1], world[2]) * vec3(vert.data[0], vert.data[1], vert.data[2]) + world[3];\n"
"	gl_Position = ViewProj * vec4(out_pos, 1.0);\n"
"	if ((call.flags & CF_USE_POLYGON_OFFSET) != 0)\n"
"		gl_Position.z += 1./1024.;\n"
"	out_uv = vec4(vert.data[3], vert.data[4], vert.data[5], vert.data[6]);\n"
"	out_depth = gl_Position.w;\n"
"	out_coord = (gl_Position.xy / gl_Position.w * 0.5 + 0.5) * vec2(LIGHT_TILES_X, LIGHT_TILES_Y);\n"
"	out_drawinstance = ivec2(DRAW_ID, INSTANCE_ID);\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char world_fragment_shader[] =
"#if BINDLESS\n"
"	#extension GL_ARB_bindless_texture : require\n"
"	sampler2D Tex;\n"
"	sampler2D FullbrightTex;\n"
"#else\n"
"	layout(binding=0) uniform sampler2D Tex;\n"
"	layout(binding=1) uniform sampler2D FullbrightTex;\n"
"#endif\n"
"layout(binding=2) uniform sampler2D LMTex;\n"
"\n"
FRAMEDATA_BUFFER
LIGHT_CLUSTER_IMAGE("readonly")
WORLD_CALLDATA_BUFFER
WORLD_INSTANCEDATA_BUFFER
"\n"
"layout(location=0) flat in ivec2 in_drawinstance;\n"
"layout(location=1) in vec3 in_pos;\n"
"layout(location=2) in vec4 in_uv;\n"
"layout(location=3) in float in_depth;\n"
"layout(location=4) noperspective in vec2 in_coord;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	Call call = call_data[in_drawinstance.x];\n"
"	Instance instance = instance_data[in_drawinstance.y];\n"
"#if BINDLESS\n"
"	Tex = sampler2D(call.txhandle);\n"
"	FullbrightTex = sampler2D(call.fbhandle);\n"
"#endif\n"
"	vec4 result = texture(Tex, in_uv.xy);\n"
"#if ALPHATEST\n"
"	if (result.a < 0.666)\n"
"		discard;\n"
"#endif\n"
"	vec3 fullbright = texture(FullbrightTex, in_uv.xy).rgb;\n"
"	vec3 total_light = texture(LMTex, in_uv.zw).rgb;\n"
"	if (NumLights > 0u)\n"
"	{\n"
"		uint i, ofs;\n"
"		ivec3 cluster_coord;\n"
"		cluster_coord.x = int(floor(in_coord.x));\n"
"		cluster_coord.y = int(floor(in_coord.y));\n"
"		cluster_coord.z = int(floor(log2(in_depth) * ZLogScale + ZLogBias));\n"
"		uvec2 clusterdata = imageLoad(LightClusters, cluster_coord).xy;\n"
"		if ((clusterdata.x | clusterdata.y) != 0u)\n"
"		{\n"
"#if " QS_STRINGIFY (SHOW_ACTIVE_LIGHT_CLUSTERS) "\n"
"			int cluster_idx = cluster_coord.x + cluster_coord.y * LIGHT_TILES_X + cluster_coord.z * LIGHT_TILES_X * LIGHT_TILES_Y;\n"
"			total_light = vec3(ivec3((cluster_idx + 1) * 0x45d9f3b) >> ivec3(0, 8, 16) & 255) / 255.0;\n"
"#endif // SHOW_ACTIVE_LIGHT_CLUSTERS\n"
"			vec4 plane;\n"
"			plane.xyz = normalize(cross(dFdx(in_pos), dFdy(in_pos)));\n"
"			plane.w = dot(in_pos, plane.xyz);\n"
"			for (i = 0u, ofs = 0u; i < 2u; i++, ofs += 32u)\n"
"			{\n"
"				uint mask = clusterdata[i];\n"
"				while (mask != 0u)\n"
"				{\n"
"					int j = findLSB(mask);\n"
"					mask ^= 1u << j;\n"
"					Light l = Lights[ofs + j];\n"
"					// mimics R_AddDynamicLights, up to a point\n"
"					float rad = l.radius;\n"
"					float dist = dot(l.origin, plane.xyz) - plane.w;\n"
"					rad -= abs(dist);\n"
"					float minlight = l.minlight;\n"
"					if (rad < minlight)\n"
"						continue;\n"
"					vec3 local_pos = l.origin - plane.xyz * dist;\n"
"					minlight = rad - minlight;\n"
"					dist = length(in_pos - local_pos);\n"
"					total_light += clamp((minlight - dist) / 16.0, 0.0, 1.0) * max(0., rad - dist) / 256. * l.color;\n"
"				}\n"
"			}\n"
"		}\n"
"	}\n"
"	result.rgb *= clamp(total_light, 0.0, 1.0) * 2.0;\n"
"	result.rgb += fullbright;\n"
"	result = clamp(result, 0.0, 1.0);\n"
"	float fog = exp2(-(FogDensity * in_depth) * (FogDensity * in_depth));\n"
"	fog = clamp(fog, 0.0, 1.0);\n"
"	result.rgb = mix(FogColor, result.rgb, fog);\n"
"	float alpha = instance.alpha;\n"
"	if (alpha < 0.0)\n"
"		alpha = 1.0;\n"
"	result.a = alpha; // FIXME: This will make almost transparent things cut holes though heavy fog\n"
"	out_fragcolor = result;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Water
//
////////////////////////////////////////////////////////////////

static const char water_vertex_shader[] =
FRAMEDATA_BUFFER
WORLD_INSTANCEDATA_BUFFER
WORLD_VERTEX_BUFFER
"\n"
"#if BINDLESS\n"
"#extension GL_ARB_shader_draw_parameters : require\n"
"	#define DRAW_ID			gl_DrawIDARB\n"
"	#define INSTANCE_ID		(gl_BaseInstanceARB + gl_InstanceID)\n"
"#else\n"
"	#define DRAW_ID			0\n"
"	#define INSTANCE_ID		gl_InstanceID\n"
"#endif\n"
"\n"
"layout(location=0) flat out ivec2 out_drawinstance; // x = draw; y = instance\n"
"layout(location=1) out vec2 out_uv;\n"
"layout(location=2) out float out_fogdist;\n"
"\n"
"void main()\n"
"{\n"
"	PackedVertex vert = vertices[gl_VertexID];\n"
"	Instance instance = instance_data[INSTANCE_ID];\n"
"	mat4x3 world = transpose(mat3x4(instance.mat[0], instance.mat[1], instance.mat[2]));\n"
"	vec3 pos = mat3(world[0], world[1], world[2]) * vec3(vert.data[0], vert.data[1], vert.data[2]) + world[3];\n"
"	gl_Position = ViewProj * vec4(pos, 1.0);\n"
"	out_uv = vec2(vert.data[3], vert.data[4]);\n"
"	out_fogdist = gl_Position.w;\n"
"	out_drawinstance = ivec2(DRAW_ID, INSTANCE_ID);\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char water_fragment_shader[] =
"#if BINDLESS\n"
"#extension GL_ARB_bindless_texture : require\n"
"sampler2D Tex;\n"
"#else\n"
"layout(binding=0) uniform sampler2D Tex;\n"
"#endif\n"
"\n"
FRAMEDATA_BUFFER
WORLD_CALLDATA_BUFFER
WORLD_INSTANCEDATA_BUFFER
"\n"
"layout(location=0) flat in ivec2 in_drawinstance;\n"
"layout(location=1) in vec2 in_uv;\n"
"layout(location=2) in float in_fogdist;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	Call call = call_data[in_drawinstance.x];\n"
"	Instance instance = instance_data[in_drawinstance.y];\n"
"#if BINDLESS\n"
"	Tex = sampler2D(call.txhandle);\n"
"#endif\n"
"	vec2 uv = in_uv * 2.0 + 0.125 * sin(in_uv.yx * (3.14159265 * 2.0) + Time);\n"
"	vec4 result = texture(Tex, uv);\n"
"	float fog = exp2(-(FogDensity * in_fogdist) * (FogDensity * in_fogdist));\n"
"	fog = clamp(fog, 0.0, 1.0);\n"
"	result.rgb = mix(FogColor, result.rgb, fog);\n"
"	float alpha = instance.alpha;\n"
"	if (alpha < 0.0)\n"
"		alpha = call.wateralpha;\n"
"	result.a *= alpha;\n"
"	out_fragcolor = result;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Sky stencil mark
//
////////////////////////////////////////////////////////////////

static const char skystencil_vertex_shader[] =
FRAMEDATA_BUFFER
WORLD_INSTANCEDATA_BUFFER
WORLD_VERTEX_BUFFER
"\n"
"#if BINDLESS\n"
"#extension GL_ARB_shader_draw_parameters : require\n"
"	#define DRAW_ID			gl_DrawIDARB\n"
"	#define INSTANCE_ID		(gl_BaseInstanceARB + gl_InstanceID)\n"
"#else\n"
"	#define DRAW_ID			0\n"
"	#define INSTANCE_ID		gl_InstanceID\n"
"#endif\n"
"\n"
"layout(location=0) flat out ivec2 out_drawinstance; // x = draw; y = instance\n"
"\n"
"void main()\n"
"{\n"
"	PackedVertex vert = vertices[gl_VertexID];\n"
"	Instance instance = instance_data[INSTANCE_ID];\n"
"	mat4x3 world = transpose(mat3x4(instance.mat[0], instance.mat[1], instance.mat[2]));\n"
"	vec3 pos = mat3(world[0], world[1], world[2]) * vec3(vert.data[0], vert.data[1], vert.data[2]) + world[3];\n"
"	gl_Position = ViewProj * vec4(pos, 1.0);\n"
"	out_drawinstance = ivec2(DRAW_ID, INSTANCE_ID);\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Sky layers
//
////////////////////////////////////////////////////////////////

static const char sky_layers_vertex_shader[] =
"layout(location=0) in vec3 in_dir;\n"
"\n"
"layout(location=0) out vec3 out_dir;\n"
"\n"
"void main()\n"
"{\n"
"	ivec2 v = ivec2(gl_VertexID & 1, gl_VertexID >> 1);\n"
"	v.x ^= v.y; // fix winding order\n"
"	gl_Position = vec4(vec2(v) * 2.0 - 1.0, 1.0, 1.0);\n"
"	out_dir = in_dir;\n"
"	out_dir.z *= 3.0; // flatten the sphere\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char sky_layers_fragment_shader[] =
"layout(binding=0) uniform sampler2D SolidLayer;\n"
"layout(binding=1) uniform sampler2D AlphaLayer;\n"
"\n"
"layout(location=2) uniform float Time;\n"
"layout(location=3) uniform vec4 Fog;\n"
"\n"
"layout(location=0) in vec3 in_dir;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	vec2 uv = normalize(in_dir).xy * (189.0 / 64.0);\n"
"	vec4 result = texture(SolidLayer, uv + Time / 16.0);\n"
"	vec4 layer = texture(AlphaLayer, uv + Time / 8.0);\n"
"	result.rgb = mix(result.rgb, layer.rgb, layer.a);\n"
"	result.rgb = mix(result.rgb, Fog.rgb, Fog.w);\n"
"	out_fragcolor = result;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Skybox
//
////////////////////////////////////////////////////////////////

static const char sky_box_vertex_shader[] =
"layout(location=0) uniform mat4 MVP;\n"
"layout(location=1) uniform vec3 EyePos;\n"
"\n"
"layout(location=0) in vec3 in_dir;\n"
"layout(location=1) in vec2 in_uv;\n"
"\n"
"layout(location=0) out vec3 out_dir;\n"
"layout(location=1) out vec2 out_uv;\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = MVP * vec4(EyePos + in_dir, 1.0);\n"
"	gl_Position.z = gl_Position.w; // map to far plane\n"
"	out_dir = in_dir;\n"
"	out_uv = in_uv;\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char sky_box_fragment_shader[] =
"layout(binding=0) uniform sampler2D Tex;\n"
"\n"
"layout(location=2) uniform vec4 Fog;\n"
"\n"
"layout(location=0) in vec3 in_dir;\n"
"layout(location=1) in vec2 in_uv;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	out_fragcolor = texture(Tex, in_uv);\n"
"	out_fragcolor.rgb = mix(out_fragcolor.rgb, Fog.rgb, Fog.w);\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Alias models
//
////////////////////////////////////////////////////////////////

#define ALIAS_INSTANCE_BUFFER \
"struct InstanceData\n"\
"{\n"\
"	mat4	MVP;\n"\
"	vec4	LightColor; // xyz=LightColor w=Alpha\n"\
"	float	ShadeAngle;\n"\
"	float	Blend;\n"\
"	int		Pose1;\n"\
"	int		Pose2;\n"\
"};\n"\
"\n"\
"layout(std430, binding=1) restrict readonly buffer InstanceBuffer\n"\
"{\n"\
"	vec4	Fog;\n"\
"	InstanceData instances[];\n"\
"};\n"\

////////////////////////////////////////////////////////////////

static const char alias_vertex_shader[] =
ALIAS_INSTANCE_BUFFER
"\n"
"layout(std430, binding=2) restrict readonly buffer PoseBuffer\n"
"{\n"
"	uvec2 PackedPosNor[];\n"
"};\n"
"\n"
"layout(std430, binding=3) restrict readonly buffer UVBuffer\n"
"{\n"
"	vec2 TexCoords[];\n"
"};\n"
"\n"
"struct Pose\n"
"{\n"
"	vec3 pos;\n"
"	vec3 nor;\n"
"};\n"
"\n"
"Pose GetPose(uint index)\n"
"{\n"
"	uvec2 data = PackedPosNor[index + gl_VertexID];\n"
"	return Pose(vec3((data.xxx >> uvec3(0, 8, 16)) & 255), unpackSnorm4x8(data.y).xyz);\n"
"}\n"
"\n"
"float r_avertexnormal_dot(vec3 vertexnormal, vec3 dir) // from MH \n"
"{\n"
"	float dot = dot(vertexnormal, dir);\n"
"	// wtf - this reproduces anorm_dots within as reasonable a degree of tolerance as the >= 0 case\n"
"	if (dot < 0.0)\n"
"		return 1.0 + dot * (13.0 / 44.0);\n"
"	else\n"
"		return 1.0 + dot;\n"
"}\n"
"\n"
"layout(location=0) out vec2 out_texcoord;\n"
"layout(location=1) out vec4 out_color;\n"
"layout(location=2) out float out_fogdist;\n"
"\n"
"void main()\n"
"{\n"
"	InstanceData inst = instances[gl_InstanceID];\n"
"	out_texcoord = TexCoords[gl_VertexID];\n"
"	Pose pose1 = GetPose(inst.Pose1);\n"
"	Pose pose2 = GetPose(inst.Pose2);\n"
"	vec3 lerpedVert = mix(pose1.pos, pose2.pos, inst.Blend);\n"
"	gl_Position = inst.MVP * vec4(lerpedVert, 1.0);\n"
"	out_fogdist = gl_Position.w;\n"
"	vec3 shadevector;\n"
"	shadevector[0] = cos(inst.ShadeAngle);\n"
"	shadevector[1] = sin(inst.ShadeAngle);\n"
"	shadevector[2] = 1.0;\n"
"	shadevector = normalize(shadevector);\n"
"	float dot1 = r_avertexnormal_dot(pose1.nor, shadevector);\n"
"	float dot2 = r_avertexnormal_dot(pose2.nor, shadevector);\n"
"	out_color = inst.LightColor * vec4(vec3(mix(dot1, dot2, inst.Blend)), 1.0);\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char alias_fragment_shader[] =
ALIAS_INSTANCE_BUFFER
"\n"
"layout(binding=0) uniform sampler2D Tex;\n"
"layout(binding=1) uniform sampler2D FullbrightTex;\n"
"\n"
"layout(location=0) in vec2 in_texcoord;\n"
"layout(location=1) in vec4 in_color;\n"
"layout(location=2) in float in_fogdist;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	vec4 result = texture(Tex, in_texcoord);\n"
"#if ALPHATEST\n"
"	if (result.a < 0.666)\n"
"		discard;\n"
"#endif\n"
"	result *= in_color * 2.0;\n"
"	result += texture(FullbrightTex, in_texcoord);\n"
"	result = clamp(result, 0.0, 1.0);\n"
"	float fog = exp2(-(Fog.w * in_fogdist) * (Fog.w * in_fogdist));\n"
"	fog = clamp(fog, 0.0, 1.0);\n"
"	result.rgb = mix(Fog.rgb, result.rgb, fog);\n"
"	result.a = in_color.a; // FIXME: This will make almost transparent things cut holes though heavy fog\n"
"	out_fragcolor = result;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Sprites
//
////////////////////////////////////////////////////////////////

static const char sprites_vertex_shader[] =
FRAMEDATA_BUFFER
"\n"
"layout(location=0) in vec4 in_pos;\n"
"layout(location=1) in vec2 in_uv;\n"
"\n"
"layout(location=0) out vec2 out_uv;\n"
"layout(location=1) out float out_fogdist;\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = ViewProj * in_pos;\n"
"	out_fogdist = gl_Position.w;\n"
"	out_uv = in_uv;\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char sprites_fragment_shader[] =
FRAMEDATA_BUFFER
"\n"
"layout(binding=0) uniform sampler2D Tex;\n"
"\n"
"layout(location=0) in vec2 in_uv;\n"
"layout(location=1) in float in_fogdist;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	vec4 result = texture(Tex, in_uv);\n"
"	if (result.a < 0.666)\n"
"		discard;\n"
"	float fog = exp2(-(FogDensity * in_fogdist) * (FogDensity * in_fogdist));\n"
"	fog = clamp(fog, 0.0, 1.0);\n"
"	result.rgb = mix(FogColor, result.rgb, fog);\n"
"	out_fragcolor = result;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Particles
//
////////////////////////////////////////////////////////////////

static const char particles_vertex_shader[] =
FRAMEDATA_BUFFER
"\n"
"layout(location=0) in vec4 in_pos;\n"
"layout(location=1) in vec4 in_color;\n"
"\n"
"layout(location=0) out vec2 out_uv;\n"
"layout(location=1) out vec4 out_color;\n"
"layout(location=2) out float out_fogdist;\n"
"\n"
"void main()\n"
"{\n"
"	gl_Position = ViewProj * in_pos;\n"
"	out_fogdist = gl_Position.w;\n"
"	int vtx = gl_VertexID % 3;\n"
"	out_uv = vec2(vtx == 1, vtx == 2);\n"
"	out_color = in_color;\n"
"}\n";

////////////////////////////////////////////////////////////////

static const char particles_fragment_shader[] =
FRAMEDATA_BUFFER
"\n"
"layout(binding=0) uniform sampler2D Tex;\n"
"\n"
"layout(location=0) in vec2 in_uv;\n"
"layout(location=1) in vec4 in_color;\n"
"layout(location=2) in float in_fogdist;\n"
"\n"
"layout(location=0) out vec4 out_fragcolor;\n"
"\n"
"void main()\n"
"{\n"
"	vec4 result = texture(Tex, in_uv);\n"
"	result *= in_color;\n"
"	float fog = exp2(-(FogDensity * in_fogdist) * (FogDensity * in_fogdist));\n"
"	fog = clamp(fog, 0.0, 1.0);\n"
"	result.rgb = mix(FogColor, result.rgb, fog);\n"
"	out_fragcolor = result;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// COMPUTE SHADERS
//
////////////////////////////////////////////////////////////////
//
// Clear indirect draws
//
////////////////////////////////////////////////////////////////

static const char clear_indirect_compute_shader[] =
"layout(local_size_x=64) in;\n"
"\n"
WORLD_DRAW_BUFFER
"\n"
"void main()\n"
"{\n"
"	uint thread_id = gl_GlobalInvocationID.x;\n"
"	if (thread_id < cmds.length())\n"
"		cmds[thread_id].count = 0u;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Gather indirect draws
//
////////////////////////////////////////////////////////////////

static const char gather_indirect_compute_shader[] =
"layout(local_size_x=64) in;\n"
"\n"
DRAW_ELEMENTS_INDIRECT_COMMAND
"\n"
"layout(std430, binding=5) restrict readonly buffer DrawIndirectSrcBuffer\n"
"{\n"
"	DrawElementsIndirectCommand src_cmds[];\n"
"};\n"
"\n"
"layout(std430, binding=6) restrict writeonly buffer DrawIndirectDstBuffer\n"
"{\n"
"	DrawElementsIndirectCommand dst_cmds[];\n"
"};\n"
"\n"
"struct DrawRemap\n"
"{\n"
"	uint src_call;\n"
"	uint instance_data;\n"
"};\n"
"\n"
"layout(std430, binding=7) restrict readonly buffer DrawRemapBuffer\n"
"{\n"
"	DrawRemap remap_data[];\n"
"};\n"
"\n"
"#define MAX_INSTANCES " QS_STRINGIFY (MAX_BMODEL_INSTANCES) "u\n"
"\n"
"void main()\n"
"{\n"
"	uint thread_id = gl_GlobalInvocationID.x;\n"
"	uint num_calls = remap_data.length();\n"
"	if (thread_id >= num_calls)\n"
"		return;\n"
"	DrawRemap remap = remap_data[thread_id];\n"
"	DrawElementsIndirectCommand cmd = src_cmds[remap.src_call];\n"
"	cmd.baseInstance = remap.instance_data / MAX_INSTANCES;\n"
"	cmd.instanceCount = (remap.instance_data % MAX_INSTANCES) + 1u;\n"
"	if (cmd.count == 0u)\n"
"		cmd.instanceCount = 0u;\n"
"	dst_cmds[thread_id] = cmd;\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Cull/mark: leaf vis/frustum culling, surface backface culling,
// index buffer + draw indirect buffer updates
//
////////////////////////////////////////////////////////////////

static const char cull_mark_compute_shader[] =
"layout(local_size_x=64) in;\n"
"\n"
WORLD_DRAW_BUFFER
"\n"
"layout(std430, binding=2) restrict writeonly buffer IndexBuffer\n"
"{\n"
"	uint indices[];\n"
"};\n"
"\n"
"layout(std430, binding=3) restrict readonly buffer VisBuffer\n"
"{\n"
"	uint vis[];\n"
"};\n"
"\n"
"struct Leaf\n"
"{\n"
"	float	mins[3];\n"
"	float	maxs[3];\n"
"	uint	firstsurf;\n"
"	uint	surfcountsky; // bit 0=sky; bits 1..31=surfcount\n"
"};\n"
"\n"
"layout(std430, binding=4) restrict readonly buffer LeafBuffer\n"
"{\n"
"	Leaf leaves[];\n"
"};\n"
"\n"
"layout(std430, binding=5) restrict readonly buffer MarkSurfBuffer\n"
"{\n"
"	int marksurf[];\n"
"};\n"
"\n"
"struct Surface\n"
"{\n"
"	vec4	plane;\n"
"	uint	framecount;\n"
"	uint	texnum;\n"
"	uint	numedges;\n"
"	uint	firstvert;\n"
"};\n"
"\n"
"layout(std430, binding=6) restrict buffer SurfaceBuffer\n"
"{\n"
"	Surface surfaces[];\n"
"};\n"
"\n"
"layout(std430, binding=7) restrict readonly buffer FrameBuffer\n"
"{\n"
"	vec4	frustum[4];\n"
"	uint	oldskyleaf;\n"
"	float	vieworg[3];\n"
"	uint	framecount;\n"
"	uint	padding[3];\n"
"};\n"
"\n"
"void main()\n"
"{\n"
"	uint thread_id = gl_GlobalInvocationID.x;\n"
"	if (thread_id >= leaves.length())\n"
"		return;\n"
"	uint visible = vis[thread_id >> 5u] & (1u << (thread_id & 31u));\n"
"	if (visible == 0u)\n"
"		return;\n"
"\n"
"	Leaf leaf = leaves[thread_id];\n"
"	uint i, j;\n"
"	for (i = 0u; i < 4u; i++)\n"
"	{\n"
"		vec4 plane = frustum[i];\n"
"		vec3 v;\n"
"		v.x = plane.x < 0.f ? leaf.mins[0] : leaf.maxs[0];\n"
"		v.y = plane.y < 0.f ? leaf.mins[1] : leaf.maxs[1];\n"
"		v.z = plane.z < 0.f ? leaf.mins[2] : leaf.maxs[2];\n"
"		if (dot(plane.xyz, v) < plane.w)\n"
"			return;\n"
"	}\n"
"\n"
"	if ((leaf.surfcountsky & 1u) > oldskyleaf)\n"
"		return;\n"
"	uint surfcount = leaf.surfcountsky >> 1u;\n"
"	vec3 campos = vec3(vieworg[0], vieworg[1], vieworg[2]);\n"
"	for (i = 0u; i < surfcount; i++)\n"
"	{\n"
"		int surfindex = marksurf[leaf.firstsurf + i];\n"
"		Surface surf = surfaces[surfindex];\n"
"		if (dot(surf.plane.xyz, campos) < surf.plane.w)\n"
"			continue;\n"
"		if (atomicExchange(surfaces[surfindex].framecount, framecount) == framecount)\n"
"			continue;\n"
"		uint texnum = surf.texnum;\n"
"		uint numedges = surf.numedges;\n"
"		uint firstvert = surf.firstvert;\n"
"		uint ofs = cmds[texnum].firstIndex + atomicAdd(cmds[texnum].count, 3u * (numedges - 2u));\n"
"		for (j = 2u; j < numedges; j++)\n"
"		{\n"
"			indices[ofs++] = firstvert;\n"
"			indices[ofs++] = firstvert + j - 1u;\n"
"			indices[ofs++] = firstvert + j;\n"
"		}\n"
"	}\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Update lightmap
//
////////////////////////////////////////////////////////////////

static const char update_lightmap_compute_shader[] =
"layout(local_size_x=256) in;\n"
"\n"
"layout(rgba8ui, binding=0) readonly uniform uimage2DArray LightmapSamples;\n"
"layout(rgba8ui, binding=1) writeonly uniform uimage2D Lightmap;\n"
"\n"
"layout(std430, binding=0) restrict readonly buffer LightStyles\n"
"{\n"
"	uint lightstyles[];\n"
"};\n"
"\n"
"layout(std430, binding=1) restrict readonly buffer Blocks\n"
"{\n"
"	uint blockofs[]; // 16:16\n"
"};\n"
"\n"
"uint deinterleave_odd(uint x)\n"
"{\n"
"	x &= 0x55555555u;\n"
"	x = (x ^ (x >> 1u)) & 0x33333333u;\n"
"	x = (x ^ (x >> 2u)) & 0x0F0F0F0Fu;\n"
"	x = (x ^ (x >> 4u)) & 0x00FF00FFu;\n"
"	x = (x ^ (x >> 8u)) & 0x0000FFFFu;\n"
"	return x;\n"
"}\n"
"\n"
"void main()\n"
"{\n"
"	uvec3 thread_id = gl_GlobalInvocationID;\n"
"	uint xy = thread_id.x | (thread_id.y << 8u);\n"
"	xy |= (xy >> 1u) << 16u;\n"
"	xy = deinterleave_odd(xy); // morton order\n"
"	uvec2 coord = uvec2(xy & 0xffu, xy >> 8u);\n"
"	xy = blockofs[thread_id.z];\n"
"	coord += uvec2(xy & 0xffffu, xy >> 16u);\n"
"	uvec3 accum = uvec3(0u);\n"
"	int i;\n"
"	for (i = 0; i < " QS_STRINGIFY(MAXLIGHTMAPS) "; i++)\n"
"	{\n"
"		uvec4 s = imageLoad(LightmapSamples, ivec3(coord, i));\n"
"		if (s.w == 255u)\n"
"			break;\n"
"		accum += s.xyz * lightstyles[s.w];\n"
"	}\n"
"	accum = min(accum >> 8u, uvec3(255u));\n"
"	imageStore(Lightmap, ivec2(coord), uvec4(accum, 255u));\n"
"}\n";

////////////////////////////////////////////////////////////////
//
// Light clustering
//
////////////////////////////////////////////////////////////////

static const char cluster_lights_compute_shader[] =
"layout(local_size_x=8, local_size_y=8, local_size_z=1) in;\n"
"\n"
FRAMEDATA_BUFFER
"\n"
LIGHT_CLUSTER_IMAGE("writeonly")
"\n"
"layout(std430, binding=1) restrict readonly buffer InputBuffer\n"
"{\n"
"	mat4	TransposedProj;\n"
"	mat4	View;\n"
"};\n"
"\n"
"shared vec4 local_lights[MAX_LIGHTS]; // xyz = view space pos; w = radius\n"
"\n"
"vec4 cluster_planes[6]; // view space; facing outside\n"
"vec3 cluster_center;\n"
"vec3 cluster_half_size;\n"
"\n"
"vec4 ExtractFrustumPlane(int axis, float ndcval, float side)\n"
"{\n"
"	vec4 plane = TransposedProj[axis] - ndcval * TransposedProj[3];\n"
"	return inversesqrt(dot(plane.xyz, plane.xyz)) * side * plane;\n"
"}\n"
"\n"
"void ComputeClusterPlanes(uvec3 gid)\n"
"{\n"
"	const float TileSizeX = 2.0 / float(LIGHT_TILES_X);\n"
"	const float TileSizeY = 2.0 / float(LIGHT_TILES_Y);\n"
"	float x0 = -1.0 + float(gid.x) * TileSizeX;\n"
"	float y0 = -1.0 + float(gid.y) * TileSizeY;\n"
"	float z0 = exp2((float(gid.z) - ZLogBias) / ZLogScale);\n"
"	cluster_planes[0] = ExtractFrustumPlane(0, x0,             -1.0);      // left\n"
"	cluster_planes[1] = ExtractFrustumPlane(0, x0 + TileSizeX,  1.0);      // right\n"
"	cluster_planes[2] = ExtractFrustumPlane(1, y0,             -1.0);      // bottom\n"
"	cluster_planes[3] = ExtractFrustumPlane(1, y0 + TileSizeY,  1.0);      // top\n"
"	cluster_planes[4] = vec4(-1.0, 0.0, 0.0,  z0);                         // near\n"
"	cluster_planes[5] = vec4( 1.0, 0.0, 0.0, -z0 * exp2(1.0 / ZLogScale)); // far\n"
"}\n"
"\n"
"float PointPlaneDistance(vec3 p, vec4 plane)\n"
"{\n"
"	return dot(p, plane.xyz) + plane.w;\n"
"}\n"
"\n"
"vec3 IntersectDepthPlane(vec3 dir, float depth)\n"
"{\n"
"	return vec3(depth, (depth / dir.x) * dir.yz);\n"
"}\n"
"\n"
"void ComputeClusterExtents()\n"
"{\n"
"	vec3 bl = cross(cluster_planes[2].xyz, cluster_planes[0].xyz); // bottom-left\n"
"	vec3 tr = cross(cluster_planes[3].xyz, cluster_planes[1].xyz); // top-right\n"
"	float depth_near = cluster_planes[4].w;\n"
"	float depth_far = -cluster_planes[5].w;\n"
"	vec3 p0 = IntersectDepthPlane(bl, depth_near);\n"
"	vec3 p1 = IntersectDepthPlane(bl, depth_far);\n"
"	vec3 p2 = IntersectDepthPlane(tr, depth_near);\n"
"	vec3 p3 = IntersectDepthPlane(tr, depth_far);\n"
"	vec3 cluster_mins = vec3(depth_near, min(min(p0.yz, p1.yz), min(p2.yz, p3.yz)));\n"
"	vec3 cluster_maxs = vec3(depth_far,  max(max(p0.yz, p1.yz), max(p2.yz, p3.yz)));\n"
"	cluster_center = (cluster_mins + cluster_maxs) * 0.5;\n"
"	cluster_half_size = (cluster_maxs - cluster_mins) * 0.5;\n"
"}\n"
"\n"
"bool LightTouchesCluster(vec4 l)\n"
"{\n"
"#if 1\n"
"	vec3 delta = max(abs(l.xyz - cluster_center) - cluster_half_size, 0.0);\n"
"	if (dot(delta, delta) >= l.w * l.w)\n"
"		return false;\n"
"#endif\n"
"#if 0\n"
"	for (int i = 0; i < 6; i++)\n"
"		if (PointPlaneDistance(l.xyz, cluster_planes[i]) > l.w)\n"
"			return false;\n"
"#endif\n"
"	return true;\n"
"}\n"
"\n"
"void main()\n"
"{\n"
"	uvec3 gid = gl_GlobalInvocationID;\n"
"	if (any(greaterThanEqual(gid, uvec3(LIGHT_TILES_X, LIGHT_TILES_Y, LIGHT_TILES_Z))))\n"
"		return;\n"
"	uint numlights = NumLights;\n"
"	if (numlights == 0u)\n"
"	{\n"
"		imageStore(LightClusters, ivec3(gid), uvec4(0u));\n"
"		return;\n"
"	}\n"
"	uint groupsize = gl_WorkGroupSize.x * gl_WorkGroupSize.y * gl_WorkGroupSize.z;\n"
"	uint numpasses = (numlights + (groupsize - 1u)) / groupsize;\n"
"	uint i, j, ofs;\n"
"	for (i = 0u, ofs = 0u; i < numpasses; i++, ofs += groupsize)\n"
"	{\n"
"		uint index = gl_LocalInvocationIndex + ofs;\n"
"		if (index < numlights)\n"
"		{\n"
"			Light l = Lights[index];\n"
"			local_lights[index] = vec4((View * vec4(l.origin, 1.0)).xyz, l.radius);\n"
"		}\n"
"	}\n"
"	memoryBarrierShared();\n"
"	barrier();\n"
"\n"
"	ComputeClusterPlanes(gid);\n"
"	ComputeClusterExtents();\n"
"\n"
"	uint clustermask[MAX_LIGHTS / 32];\n"
"	for (i = 0u; i < clustermask.length(); i++)\n"
"		clustermask[i] = 0u;\n"
"	for (i = 0u; i < numlights; i++)\n"
"		if (LightTouchesCluster(local_lights[i]))\n"
"			clustermask[i >> 5u] |= 1u << (i & 31u);\n"
"	imageStore(LightClusters, ivec3(gid), uvec4(clustermask[0], clustermask[1], 0u, 0u));\n"
"}\n";
