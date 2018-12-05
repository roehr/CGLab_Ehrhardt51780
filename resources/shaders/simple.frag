#version 150
//#extension GL_OES_standard_derivatives : enable
in vec3 pass_Normal;
in vec3 pass_Pos;
in vec3 pass_Vert;
in vec2 pass_UV;
in mat3 pass_TBN;
in vec3 eyepos;

uniform vec3 PlanetColor;
uniform vec3 LightColor;
uniform vec3 CamPosition;
uniform vec3 LightPosition;
uniform float LightIntensity;
uniform int Toon;
uniform int Sun;
out vec4 out_Color;

uniform sampler2D DiffMap;
uniform sampler2D NormMap;
uniform sampler2D SpecMap;
uniform sampler2D DispMap;


vec3 perturbNormal( vec3 vertex_pos, vec3 surf_norm,vec2 uv) {
// 	derivatives -> get slopes
vec3 q0 = dFdx( vertex_pos.xyz );
vec3 q1 = dFdy( vertex_pos.xyz );
vec2 st0 = dFdx(uv.st );
vec2 st1 = dFdy(uv.st );
//basically computing tangent and bitangent
vec3 S = normalize( q0 * st1.t - q1 * st0.t );
vec3 T = normalize( -q0 * st1.s + q1 * st0.s );
vec3 N = normalize( surf_norm );
//"normalize" the space of the coordinates 
vec3 mapN = texture2D(NormMap,uv).xyz * 2.0 - 1.0;

//mapN.xy = normalScale * mapN.xy;
mat3 tsn = mat3( S, T, N );
return normalize(tsn * mapN );
}

//testing out Parallax
/*
vec2 ParallaxMapping(vec2 uv, vec3 vDir)
{
    float layers = 20;  
    float layerDepth = 1.0 / layers;
    float currentLayerDepth = 0.0;
    vec2 P = vDir.xy / vDir.z; //* heightScale; 
    vec2 deltaUV = P / layers;
    vec2  currUV   = uv;
    float currDispMapVal = texture(DispMap, currUV).r;
      
    while(currentLayerDepth < currDispMapVal)
    {
       
        currUV -= deltaUV;
        currDispMapVal = texture(DispMap, currUV).r;  
        currentLayerDepth += layerDepth;  
    }
    
    vec2 prevUV = currUV + deltaUV;
    float afterDepth  = currDispMapVal - currentLayerDepth;
    float beforeDepth = texture(DispMap, prevUV).r - currentLayerDepth + layerDepth;
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalUV = prevUV * weight + currUV* (1.0 - weight);
    return finalUV;
}
*/


void main() {
//our positions needed
    vec3 tViewPos=eyepos;
	vec3 tFragPos=pass_Pos;
	vec3 tLightPos=LightPosition;

    vec3 v = normalize(tViewPos - tFragPos);
	vec2 tUV = pass_UV;
  // tUV=ParallaxMapping(tUV,v);
  //Blinn-Phong:
  
	//calculate everything we need:
	vec3 n = normalize(pass_Normal);
		 n=perturbNormal(tFragPos,n,tUV);
	vec3 l = normalize(tLightPos - tFragPos); 
	vec3 h = normalize(l+v); 

	//ambient:
    vec3 acolor =0.25* LightColor;

	//diffuse:
	float diff = max(dot(n, l), 0.0);
	float s = pow(max(dot(n, h), 0.0), 25);	
	//specular
	//25 used as shininess
	vec3 dcolor = diff * LightColor;
//no special specColor since for now Light is white
	vec3 scolor = LightIntensity * s * LightColor;

	//get the diffuse color from the map
	vec4 diffCol=texture(DiffMap, tUV);
	vec4 result=(vec4(acolor,1.0)+vec4(dcolor,1.0)+vec4(scolor,1.0))*diffCol;
	//outline for Toon
	if(Sun==1){
	result=diffCol;
	}

	if(Toon==1){
	//outline
		if( max(dot(n, v), 0.0)<0.2){
			result=vec4(1.0,0.0,0.0,1.0);
		}
		else if(diff>0.75){
		   result=vec4(PlanetColor,1.0);
		}
		else{
		
		result=vec4(0.5*PlanetColor.x,0.5*PlanetColor.y,0.5*PlanetColor.z,1.0);
		}
	}
	out_Color = result;
 
}
