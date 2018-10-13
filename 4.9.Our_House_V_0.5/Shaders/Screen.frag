#version 400

struct LIGHT {
	vec4 position; // assume point or direction in EC in this example shader
	vec4 ambient_color, diffuse_color, specular_color;
	vec4 light_attenuation_factors; // compute this effect only if .w != 0.0f
	vec3 spot_direction;
	float spot_exponent;
	float spot_cutoff_angle;
	bool light_on;
};

struct MATERIAL {
	vec4 ambient_color;
	vec4 diffuse_color;
	vec4 specular_color;
	vec4 emissive_color;
	float specular_exponent;
};

uniform vec4 u_global_ambient_color;
#define NUMBER_OF_LIGHTS_SUPPORTED 4
uniform LIGHT u_light[NUMBER_OF_LIGHTS_SUPPORTED];
uniform MATERIAL u_material;

uniform int u_scrLevel;
uniform vec3 u_scrPos;
uniform mat3 u_scrXY;

const float zero_f = 0.0f;
const float one_f = 1.0f;

in vec3 v_position_EC;
in vec3 v_normal_EC;
in vec3 a_position_out;
layout (location = 0) out vec4 final_color;

float f_mod(float a, float b){
	while(a>=b){
		a -= b;
	}
	return a;
}

vec4 lighting_equation(in vec3 P_EC, in vec3 N_EC) {
	vec4 color_sum;
	float local_scale_factor, tmp_float; 
	vec3 L_EC;

	//Screen Shader
		vec3 P_OBJ = a_position_out - vec3(100.0f, 100.0f, 20.0f); //- u_scrPos;
		float block_size = 20.0f / u_scrLevel;
		float p_x, p_y;
		//P_OBJ = u_scrXY * P_OBJ;

		p_x = f_mod(P_OBJ[0], block_size);
		p_y = f_mod(P_OBJ[1], block_size);
		
		if (0.2f*block_size < p_x && p_x< 0.8f*block_size &&
			0.2f*block_size < p_y && p_y < 0.8f*block_size) discard;

	//END!

	color_sum = u_material.emissive_color + u_global_ambient_color * u_material.ambient_color;
 
	for (int i = 0; i < NUMBER_OF_LIGHTS_SUPPORTED; i++) {
		if (!u_light[i].light_on) continue;

		local_scale_factor = one_f;
		if (u_light[i].position.w != zero_f) { // point light source
			L_EC = u_light[i].position.xyz - P_EC.xyz;

			if (u_light[i].light_attenuation_factors.w  != zero_f) {
				vec4 tmp_vec4;

				tmp_vec4.x = one_f;
				tmp_vec4.z = dot(L_EC, L_EC);
				tmp_vec4.y = sqrt(tmp_vec4.z);
				tmp_vec4.w = zero_f;
				local_scale_factor = one_f/dot(tmp_vec4, u_light[i].light_attenuation_factors);
			}

			L_EC = normalize(L_EC);

		}

		if (local_scale_factor > zero_f) {				
			vec4 local_color_sum = u_light[i].ambient_color * u_material.ambient_color;

			tmp_float = dot(N_EC, L_EC);
			if (tmp_float > zero_f) {
				local_color_sum += u_light[i].diffuse_color*u_material.diffuse_color*tmp_float;
			
				vec3 H_EC = normalize(L_EC - normalize(P_EC));
				tmp_float = dot(N_EC, H_EC); 
				if (tmp_float > zero_f) {
					local_color_sum += u_light[i].specular_color
										   *u_material.specular_color*pow(tmp_float, u_material.specular_exponent);
				}
			}
			color_sum += local_scale_factor*local_color_sum;
		}
	}
 	return color_sum;
}

void main(void) {   
	// final_color = vec4(gl_FragCoord.x/800.0f, gl_FragCoord.y/800.0f, 0.0f, 1.0f); // what is this?
    // final_color = vec4(0.0f,  0.0f, 1.0 - gl_FragCoord.z/1.0f, 1.0f); // what is this?

   final_color = lighting_equation(v_position_EC, normalize(v_normal_EC)); // for normal rendering
}
