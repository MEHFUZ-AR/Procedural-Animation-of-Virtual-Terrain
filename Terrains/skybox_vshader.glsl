R"(
#version 330 core
in vec3 vposition;
uniform float cloudMotion;
out vec3 uvw;

uniform mat4 V;
uniform mat4 P;

void main() {
    // Cubemaps follow a LHS coordinate system
    vec3 vtex=vposition;
    //vtex.z=vtex.z + (tan(2.0 * 3.14/cloudMotion));
    vtex.y = vtex.y + (sin(2.0 * 3.14/cloudMotion) * cos(1.5 * 3.14/cloudMotion));
    //vtex.x=vtex.y + ((2.0 * 3.14/cloudMotion));
    uvw = vec3(vposition.x, -vposition.z, -vtex.y);
    
    gl_Position = P*V*vec4(10.0*vposition, 1.0);
}
)"
