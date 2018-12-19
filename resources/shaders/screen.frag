#version 330 core
out vec4 Color;
in vec2 pass_uv;
uniform int Grey;
uniform int MirrorX;
uniform int MirrorY;
uniform int Gauss;


uniform sampler2D ColMap;
void main()
{
	vec2 uv=pass_uv;
	//Mirrors:
	if(MirrorX==1){
		uv.x=1.0-uv.x;
	}

	if(MirrorY==1){
		uv.y=1.0-uv.y;
	}

    vec3 col = texture(ColMap, uv).rgb;


	if(Gauss==1){
		//Blur
		//define sampling positions:
		float dist = 1.0 / 700.0; //defines an offset for surrounding values 
		vec2 distances[9] = vec2[](
			//define the corresponding places for the kernel 
			vec2(-dist, dist), 
			vec2( 0.0, dist), 
			vec2( dist, dist), 
			vec2(-dist, 0.0),   
			vec2( 0.0, 0.0),   
			vec2( dist, 0.0),  
			vec2(-dist, -dist), 
			vec2( 0.0f, -dist), 
			vec2( dist, -dist)    
		);
		//the kernel we'll use
		float kernel[9] = float[](
			1.0/8.0, 1.0/8.0, 1.0/16.0,
			1.0/8.0, 1.0/4.0, 1.0/8.0,
			1.0/8.0, 1.0/8.0, 1.0/16.0
		);
		//these are the values sampled from the texture
		vec3 texVal[9];
		col = vec3(0.0);
		for(int i = 0; i < 9; i++)
		{	//sample using the offsets
			texVal[i] = vec3(texture(ColMap, uv + distances[i]));
			//and apply color
			col += texVal[i] * kernel[i];
		 }
	}
	//Greyscale
	if(Grey==1){
		float average= 0.2126*col.x+ 0.7152 *col.y +  0.0722*col.z;
		col=vec3(average,average,average);
	}
    Color = vec4(col, 1.0);
} 