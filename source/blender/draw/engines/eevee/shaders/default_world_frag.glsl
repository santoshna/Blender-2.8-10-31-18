#define M_PI 3.14159265358979323846

uniform float backgroundAlpha;
uniform mat4 ProjectionMatrix;
uniform mat4 ProjectionMatrixInverse;
uniform mat4 ViewMatrixInverse;
#ifdef LOOKDEV
uniform mat3 StudioLightMatrix;
uniform sampler2D image;
uniform float studioLightFadeout = 0.0;
in vec3 viewPosition;
#else
uniform vec3 color;
#endif

out vec4 FragColor;

void background_transform_to_world(vec3 viewvec, out vec3 worldvec)
{
	vec4 v = (ProjectionMatrix[3][3] == 0.0) ? vec4(viewvec, 1.0) : vec4(0.0, 0.0, 1.0, 1.0);
	vec4 co_homogenous = (ProjectionMatrixInverse * v);

	vec4 co = vec4(co_homogenous.xyz / co_homogenous.w, 0.0);
	worldvec = (ViewMatrixInverse * co).xyz;
}

float hypot(float x, float y)
{
	return sqrt(x * x + y * y);
}

void node_tex_environment_equirectangular(vec3 co, sampler2D ima, out vec4 color)
{
	vec3 nco = normalize(co);
	float u = -atan(nco.y, nco.x) / (2.0 * M_PI) + 0.5;
	float v = atan(nco.z, hypot(nco.x, nco.y)) / M_PI + 0.5;

	/* Fix pole bleeding */
	float half_width = 0.5 / float(textureSize(ima, 0).x);
	v = clamp(v, half_width, 1.0 - half_width);

	/* Fix u = 0 seam */
	/* This is caused by texture filtering, since uv don't have smooth derivatives
	 * at u = 0 or 2PI, hardware filtering is using the smallest mipmap for certain
	 * texels. So we force the highest mipmap and don't do anisotropic filtering. */
	color = textureLod(ima, vec2(u, v), 0.0);
}

void main() {
#ifdef LOOKDEV
	vec3 worldvec;
	vec4 color;
	background_transform_to_world(viewPosition, worldvec);
	node_tex_environment_equirectangular(StudioLightMatrix * worldvec, image, color);
	color *= (1.0 - studioLightFadeout);
#endif

	FragColor = vec4(clamp(color.rgb, vec3(0.0), vec3(1e10)), backgroundAlpha);
}