#version 150
in vec3 pass_Normal;
in vec3 position;
in vec3 vPos;


uniform vec3 PlanetColor;
uniform vec3 LightColor;
uniform float LightIntensity;
uniform vec3 LightPosition;
uniform int Toon;
out vec4 out_Color;


void main() {

//Blinn-Phong:
	//colors of ambient, diffuse and spec:
	
	vec3 ambient=PlanetColor;
	vec3 diffuse=PlanetColor;
	vec3 specular=vec3(1.0,1.0,1.0);


   
	//calculate everything we need:
	vec3 n = normalize(pass_Normal);
	vec3 l = normalize(LightPosition - position); 
	vec3 v = normalize(- vPos);
    vec3 r = reflect(-l, n);

	//ambient:
    vec3 acolor =LightIntensity* LightColor;

	//diffuse:
	float diff = max(dot(n, l), 0.0);
	
	if(Toon==1){
	//Clip diffuse
		if(diff<0.25){
		diff=0.0;
		} 
		if(diff>=0.25){
		diff=0.5;
		}
		if(diff>=0.75){
		diff=1.0;
		}
	}
	vec3 dcolor = diff * LightColor;
	//specular

	float s = pow(max(dot(v, r), 0.0), 25);
	if(Toon==1){
	    //clip specular values
		if(s>0.75){
		s=1.0;
		}
		else{
		s=0;
		}
	}

	vec3 scolor = LightIntensity * s * LightColor;


	vec3 result=(acolor+dcolor+scolor)*PlanetColor;

	if(Toon==1){
		if( max(dot(n, v), 0.0)<0.2){
			result=vec3(0.2,0.2,0.2);
		}
	}
	

	out_Color = vec4(result, 1.0);
 //gl_FragColor  = out_Color;
}
