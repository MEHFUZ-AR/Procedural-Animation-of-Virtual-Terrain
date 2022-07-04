R"(
#version 330 core

// Uniforms
uniform sampler2D noiseTex;
uniform sampler2D grass;
uniform sampler2D rock;
uniform sampler2D sand;
uniform sampler2D snow;
uniform sampler2D water;
uniform sampler2D lunar;

uniform float waveMotion;
uniform vec3 viewPos;

// In
in vec2 uv;
in vec3 fragPos;
in float waterHeight;

// Out
out vec4 color;

void main() {

    // Directional light source
    vec3 lightDir = normalize(vec3(1,1,1));

    // Texture size in pixels
    ivec2 size = textureSize(noiseTex, 0);

    /// TODO: Calculate surface normal N
    /// HINT: Use textureOffset(,,) to read height at uv + pixelwise offset
    /// HINT: Account for texture x,y dimensions in world space coordinates (default f_width=f_height=5)
    vec3 A = vec3(uv.x + 1.0f / size.x, uv.y, textureOffset(noiseTex, uv, ivec2(1, 0)));
    vec3 B = vec3(uv.x - 1.0f / size.x, uv.y, textureOffset(noiseTex, uv, ivec2(-1, 0)));
    vec3 C = vec3(uv.x, uv.y-1.0f / size.y, textureOffset(noiseTex, uv, ivec2(0, -1)));
    vec3 D = vec3(uv.x, uv.y+1.0f / size.y, textureOffset(noiseTex, uv, ivec2(0, 1)));
    vec3 normal = normalize( cross(normalize(A-B), normalize(C-D)) );

    /// TODO: Texture according to height and slope
    /// HINT: Read noiseTex for height at uv
    vec4 col = vec4(0,0.5,0.1,1);
    
    
    

    float mix_zone = 0.05f;
    float snow_height=0.91;
    float grass_height=0.62;
    float rock_height=0.68;
 
    //shading based on height and slope of the terrain

    float angleDiff = abs(dot(normal.xyz, vec3(0, 0, 1)));
    float pureRock = 0.6;
    float lerpRock = 0.7;
    float coef = 1.0 - smoothstep(pureRock, lerpRock, angleDiff);
    vec4 snowColor = (texture(snow, uv) * (coef))+(texture(rock, uv) * (1-coef));

    if (fragPos.z > snow_height + mix_zone){
    col=snowColor;
    } else if (fragPos.z > snow_height - mix_zone) {
    float coef = (fragPos.z-(snow_height - mix_zone))/(2.0 * mix_zone);
    col = (texture(snow, uv) * (coef))+(texture(rock, uv) * (1-coef));
    } else if (fragPos.z > rock_height + mix_zone){
    col=texture(rock,uv);
    } else if (fragPos.z > rock_height - mix_zone) {
    float coef = (fragPos.z-(rock_height - mix_zone))/(2.0 * mix_zone);
    col = (texture(rock, uv) * (coef))+(texture(grass, uv) * (1-coef));
    } else if (fragPos.z > grass_height + mix_zone){
    col=texture(grass,uv);
    } else if (fragPos.z > grass_height - mix_zone){
    float coef = (fragPos.z-(grass_height - mix_zone))/(2.0 * mix_zone);
    col = (texture(grass, uv) * (coef))+(texture(sand, uv) * (1-coef));
    } else {
    col=texture(sand,uv);
    }
    


    /// TODO: Calculate ambient, diffuse, and specular lighting
    // Calculate ambient lighting factor
    float ambient = 0.05f;
    float diffuse_coefficient = 0.2f;
    float specular_coefficient = 0.2f;
    float specularPower = 16.0;


        // Calculate diffuse lighting factor
    //vec3 lightDir = normalize(lightPos - fragPos);
    float diffuse = diffuse_coefficient * max(0.0f, -dot(normal, lightDir));

    // Calculate specular lighting factor
    vec3 view_direction = normalize(viewPos - fragPos);
    vec3 halfway = normalize(lightDir + view_direction);
    float specular = specular_coefficient * max(0.0f, pow(dot(normal, halfway), specularPower));
    /// HINT: max(,) dot(,) reflect(,) normalize()


    vec3 L=vec3(0,0,-1);
    vec3 V = vec3(0,0,1); 
    vec3 R = reflect(normalize(view_direction),-normal);
    float glossy = pow( max(dot(-R,L),0), 100 );
  

    //vec4 col = vec4(0,0.5,0.1,1);    
    col += (ambient + diffuse + specular);  


    color = col;

}
)"
