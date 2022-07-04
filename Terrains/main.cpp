#include <OpenGP/GL/Application.h>
#include "OpenGP/GL/Eigen.h"

#include "loadTexture.h"
#include "noise.h"

using namespace OpenGP;
const int width=1280, height=720;

const char* skybox_vshader =
#include "skybox_vshader.glsl"
;
const char* skybox_fshader =
#include "skybox_fshader.glsl"
;

const char* terrain_vshader =
#include "terrain_vshader.glsl"
;
const char* terrain_fshader =
#include "terrain_fshader.glsl"
;

const char* water_vshader =
#include "water_vshader.glsl"
;
const char* water_fshader =
#include "water_fshader.glsl"
;

const char* water2_vshader =
#include "water2_vshader.glsl"
;
const char* water2_fshader =
#include "water2_fshader.glsl"
;

const unsigned resPrim = 999999;
constexpr float PI = 3.14159265359f;

void init();
void genTerrainMesh();
void genWaterMesh();
void genWater2Mesh();
void genCubeMesh();
void drawSkybox();
void drawTerrain();
void drawWater();
void drawWater2();


std::unique_ptr<Shader> skyboxShader;
std::unique_ptr<GPUMesh> skyboxMesh;
GLuint skyboxTexture;

std::unique_ptr<Shader> terrainShader;
std::unique_ptr<GPUMesh> terrainMesh;
std::unique_ptr<R32FTexture> heightTexture;
std::unique_ptr<R32FTexture> heightTexture2;
std::map<std::string, std::unique_ptr<RGBA8Texture>> terrainTextures;

std::unique_ptr<Shader> waterShader;
std::unique_ptr<GPUMesh> waterMesh;
std::map<std::string, std::unique_ptr<RGBA8Texture>> waterTextures;

std::unique_ptr<Shader> water2Shader;
std::unique_ptr<GPUMesh> water2Mesh;
std::map<std::string, std::unique_ptr<RGBA8Texture>> water2Textures;



Vec3 cameraPos;
Vec3 cameraFront;
Vec3 cameraUp;

float fov;
float speed;
float speedIncrement;
float halflife;

float yaw;
float pitch;


float waveMotion;
float waveMotion2;
float cloudMotion;

int main(int, char**){

    Application app;

    init();
    genCubeMesh();
    genTerrainMesh();
    genWaterMesh();
    genWater2Mesh();

    // Initialize camera position and direction
    cameraPos = Vec3(0.0f, 0.0f, 3.0f);
    cameraFront = Vec3(0.0f, -1.0f, 0.0f);
    cameraUp = Vec3(0.0f, 0.0f, 1.0f);

    // Initialize FOV and camera speed
    fov = 80.0f;
    speed = 0.01f;
    speedIncrement = 0.002f;

    // Initialize yaw (left/right) and pitch (up/down) angles
    yaw = 0.0f;
    pitch = 0.0f;

    // Initialize motion of waves
    waveMotion = 0.5f;
    waveMotion2 = 0.5f;
    cloudMotion = 0.5f;

    // Display callback
    Window& window = app.create_window([&](Window&){

        // Mac OSX Configuration (2:1 pixel density)
        //glViewport(0,0,width*2,height*2);

        // Windows Configuration (1:1 pixel density)
        glViewport(0,0,width,height);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        drawSkybox();
        glClear(GL_DEPTH_BUFFER_BIT);
        drawTerrain();
        drawWater();
        drawWater2();
    });
    window.set_title("Virtual Landscape");
    window.set_size(width, height);


    // Handle mouse input (looking around the screen)
    Vec2 mouse(0,0);
    window.add_listener<MouseMoveEvent>([&](const MouseMoveEvent &m){

        // Camera control
        Vec2 delta = m.position - mouse;
        delta[1] = -delta[1];
        float sensitivity = 0.005f;
        delta = sensitivity * delta;

        yaw += delta[0];
        pitch += delta[1];

        if(pitch > PI/2.0f - 0.01f)  pitch =  PI/2.0f - 0.01f;
        if(pitch <  -PI/2.0f + 0.01f) pitch =  -PI/2.0f + 0.01f;

        Vec3 front(0,0,0);
        front[0] = sin(yaw)*cos(pitch);
        front[1] = cos(yaw)*cos(pitch);
        front[2] = sin(pitch);

        cameraFront = front.normalized();
        mouse = m.position;
    });

    // TODO: Key event listener: Handle keyboard input (moving around the screen)
    window.add_listener<KeyEvent>([&](const KeyEvent &k){

        // Movement left, right, foward and backward (WASD)
		//up
        if (k.key == GLFW_KEY_W) {
            cameraPos = cameraPos + speed * cameraFront.normalized();
        }


        // Movement left, right, foward and backward (WASD)
        //up
        if (k.key == GLFW_KEY_W) {
            cameraPos = cameraPos + speed * cameraFront.normalized();
        }

        //right (GLFW_KEY_D)
        if (k.key == GLFW_KEY_D) {
            cameraPos = cameraPos + (cameraFront.cross(cameraUp)).normalized() * speed;
        }


        //left(GLFW_KEY_A)
        if (k.key == GLFW_KEY_A) {
            cameraPos = cameraPos - (cameraFront.cross(cameraUp)).normalized() * speed;
        }

        //down(GLFW_KEY_S)
        if (k.key == GLFW_KEY_S) {
            cameraPos = cameraPos - speed * cameraFront.normalized();
        }

        // TODO: Adjust FOV -decrease FOV
        if (k.key == GLFW_KEY_UP) {
            fov -= 1.0f;
            if (fov <= 1.0f) fov = 1.0f;
        }
		
		//TODO: increase FOV for down key, max of 80
        if (k.key == GLFW_KEY_DOWN) {
            fov += 1.0f;
            if (fov >= 80.0f) fov = 80.0f;
        }
      

        // TODO: Adjust movement speed-increase
        if (k.key == GLFW_KEY_RIGHT) {
            speed += speedIncrement;
            if (speed >= 1.0f) speed = 1.0f;
        }
		
		//TODO: decrement speed on left key
        if (k.key == GLFW_KEY_LEFT) {
            speed -= speedIncrement;
            if (speed <= 0.01f) speed = 0.01f;
        }
       
    });

    return app.run();
}

void init(){
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    // Enable seamless cubemap
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Enable blending
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Complile skybox shader
    skyboxShader = std::unique_ptr<Shader>(new Shader());
    skyboxShader->verbose = true;
    skyboxShader->add_vshader_from_source(skybox_vshader);
    skyboxShader->add_fshader_from_source(skybox_fshader);
    skyboxShader->link();

    // Complile terrain shader
    terrainShader = std::unique_ptr<Shader>(new Shader());
    terrainShader->verbose = true;
    terrainShader->add_vshader_from_source(terrain_vshader);
    terrainShader->add_fshader_from_source(terrain_fshader);
    terrainShader->link();

    // Complile water shader
    waterShader = std::unique_ptr<Shader>(new Shader());
    waterShader->verbose = true;
    waterShader->add_vshader_from_source(water_vshader);
    waterShader->add_fshader_from_source(water_fshader);
    waterShader->link();

    // Complile water shader2
    water2Shader = std::unique_ptr<Shader>(new Shader());
    water2Shader->verbose = true;
    water2Shader->add_vshader_from_source(water2_vshader);
    water2Shader->add_fshader_from_source(water2_fshader);
    water2Shader->link();

    // Get height texture (Regular fBm)
    heightTexture = std::unique_ptr<R32FTexture>(fBm2DTexture());

    // Get height texture (Hybrid Multifractal)[Optional]
    //heightTexture = std::unique_ptr<R32FTexture>(HybridMultifractal2DTexture());
    
    // Load terrain textures
    const std::string list[] = {"grass", "rock", "sand", "snow", "water", "lunar"};
    for (int i=0 ; i < 6 ; ++i) {
        loadTexture(terrainTextures[list[i]], (list[i]+".png").c_str());
        terrainTextures[list[i]]->bind();
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    for (int i = 0; i < 6; ++i) {
        loadTexture(waterTextures[list[i]], (list[i] + ".png").c_str());
        waterTextures[list[i]]->bind();
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    for (int i = 0; i < 6; ++i) {
        loadTexture(water2Textures[list[i]], (list[i] + ".png").c_str());
        water2Textures[list[i]]->bind();
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    // Load skybox textures
    const std::string skyList[] = {"miramar_ft", "miramar_bk", "miramar_dn", "miramar_up", "miramar_rt", "miramar_lf"};
    const std::string cubemapLayers[] = {"GL_TEXTURE_CUBE_MAP_POSITIVE_X", "GL_TEXTURE_CUBE_MAP_NEGATIVE_X",
                                         "GL_TEXTURE_CUBE_MAP_POSITIVE_Y", "GL_TEXTURE_CUBE_MAP_NEGATIVE_Y",
                                         "GL_TEXTURE_CUBE_MAP_POSITIVE_Z", "GL_TEXTURE_CUBE_MAP_NEGATIVE_Z"};

    glGenTextures(1, &skyboxTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    int tex_wh = 1024;
    for(int i=0; i < 6; ++i) {
        std::vector<unsigned char> image;
        loadTexture(image, (skyList[i]+".png").c_str());
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, GL_RGBA, tex_wh, tex_wh, 0, GL_RGBA, GL_UNSIGNED_BYTE, &image[0]);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void genTerrainMesh() {
    
    // Generate a flat mesh for the terrain with given dimensions, using triangle strips
    terrainMesh = std::unique_ptr<GPUMesh>(new GPUMesh());


    // Grid resolution
    int n_width = 1024;
    int n_height = 1024;
    //int n_width = 256;
    //int n_height = 256;



    // Grid dimensions (centered at (0, 0))
    float f_width = 5.0f;
    float f_height = 5.0f;



    std::vector<Vec3> points;
    std::vector<unsigned int> indices;
    std::vector<Vec2> texCoords;



    // Generate vertex and texture coordinates
    for (int j = 0; j < n_width; ++j) {
        for (int i = 0; i < n_height; ++i) {



            // Calculate vertex positions
            float vertX = -f_width / 2 + j / (float)n_width * f_width;
            float vertY = -f_height / 2 + i / (float)n_height * f_height;
            float vertZ = 0.0f;
            points.push_back(Vec3(vertX, vertY, vertZ));



            // Calculate texture coordinates
            float texX = i / (float)(n_width - 1);
            float texY = j / (float)(n_height - 1);
            texCoords.push_back(Vec2(texX, texY));
        }
    }



    // Generate element indices via triangle strips
    for (int j = 0; j < n_width - 1; ++j) {



        // Push two vertices at the base of each strip
        float baseX = j * n_width;
        indices.push_back(baseX);



        float baseY = ((j + 1) * n_width);
        indices.push_back(baseY);



        for (int i = 1; i < n_height; ++i) {



            // Calculate next two vertices
            float tempX = i + j * n_width;
            indices.push_back(tempX);



            float tempY = i + (j + 1) * n_height;
            indices.push_back(tempY);
        }



        // A new strip will begin when this index is reached
        indices.push_back(resPrim);
    }



    terrainMesh->set_vbo<Vec3>("vposition", points);
    terrainMesh->set_triangles(indices);
    terrainMesh->set_vtexcoord(texCoords);
    
}


void genWaterMesh() {

    // Generate a flat mesh for the terrain with given dimensions, using triangle strips
    waterMesh = std::unique_ptr<GPUMesh>(new GPUMesh());


    // Grid resolution
    int n_width = 1024;
    int n_height = 1024;
    //int n_width = 256;
    //int n_height = 256;



    // Grid dimensions (centered at (0, 0))
    float f_width = 5.0f;
    float f_height = 5.0f;



    std::vector<Vec3> points;
    std::vector<unsigned int> indices;
    std::vector<Vec2> texCoords;



    // Generate vertex and texture coordinates
    for (int j = 0; j < n_width; ++j) {
        for (int i = 0; i < n_height; ++i) {



            // Calculate vertex positions
            float vertX = -f_width / 2 + j / (float)n_width * f_width;
            float vertY = -f_height / 2 + i / (float)n_height * f_height;
            float vertZ = 0.57f;
            points.push_back(Vec3(vertX, vertY, vertZ));



            // Calculate texture coordinates
            float texX = i / (float)(n_width - 1);
            float texY = j / (float)(n_height - 1);
            texCoords.push_back(Vec2(texX, texY));
        }
    }



    // Generate element indices via triangle strips
    for (int j = 0; j < n_width - 1; ++j) {



        // Push two vertices at the base of each strip
        float baseX = j * n_width;
        indices.push_back(baseX);



        float baseY = ((j + 1) * n_width);
        indices.push_back(baseY);



        for (int i = 1; i < n_height; ++i) {



            // Calculate next two vertices
            float tempX = i + j * n_width;
            indices.push_back(tempX);



            float tempY = i + (j + 1) * n_height;
            indices.push_back(tempY);
        }



        // A new strip will begin when this index is reached
        indices.push_back(resPrim);
    }



    waterMesh->set_vbo<Vec3>("vposition", points);
    waterMesh->set_triangles(indices);
    waterMesh->set_vtexcoord(texCoords);

}

void genWater2Mesh() {

    // Generate a flat mesh for the terrain with given dimensions, using triangle strips
    water2Mesh = std::unique_ptr<GPUMesh>(new GPUMesh());


    // Grid resolution
    int n_width = 1024;
    int n_height = 1024;
    //int n_width = 256;
    //int n_height = 256;



    // Grid dimensions (centered at (0, 0))
    float f_width = 5.0f;
    float f_height = 5.0f;



    std::vector<Vec3> points;
    std::vector<unsigned int> indices;
    std::vector<Vec2> texCoords;



    // Generate vertex and texture coordinates
    for (int j = 0; j < n_width; ++j) {
        for (int i = 0; i < n_height; ++i) {



            // Calculate vertex positions
            float vertX = -f_width / 2 + j / (float)n_width * f_width;
            float vertY = -f_height / 2 + i / (float)n_height * f_height;
            float vertZ = 0.57f;
            points.push_back(Vec3(vertX, vertY, vertZ));



            // Calculate texture coordinates
            float texX = i / (float)(n_width - 1);
            float texY = j / (float)(n_height - 1);
            texCoords.push_back(Vec2(texX, texY));
        }
    }



    // Generate element indices via triangle strips
    for (int j = 0; j < n_width - 1; ++j) {



        // Push two vertices at the base of each strip
        float baseX = j * n_width;
        indices.push_back(baseX);



        float baseY = ((j + 1) * n_width);
        indices.push_back(baseY);



        for (int i = 1; i < n_height; ++i) {



            // Calculate next two vertices
            float tempX = i + j * n_width;
            indices.push_back(tempX);



            float tempY = i + (j + 1) * n_height;
            indices.push_back(tempY);
        }



        // A new strip will begin when this index is reached
        indices.push_back(resPrim);
    }



    water2Mesh->set_vbo<Vec3>("vposition", points);
    water2Mesh->set_triangles(indices);
    water2Mesh->set_vtexcoord(texCoords);

}




void genCubeMesh() {

    // Generate a cube mesh for skybox
    skyboxMesh = std::unique_ptr<GPUMesh>(new GPUMesh());
    std::vector<Vec3> points;

    points.push_back(Vec3( 1, 1, 1)); // 0
    points.push_back(Vec3(-1, 1, 1)); // 1
    points.push_back(Vec3( 1, 1,-1)); // 2
    points.push_back(Vec3(-1, 1,-1)); // 3
    points.push_back(Vec3( 1,-1, 1)); // 4
    points.push_back(Vec3(-1,-1, 1)); // 5
    points.push_back(Vec3(-1,-1,-1)); // 6
    points.push_back(Vec3( 1,-1,-1)); // 7

    std::vector<unsigned int> indices = { 3, 2, 6, 7, 4, 2, 0, 3, 1, 6, 5, 4, 1, 0 };
    skyboxMesh->set_vbo<Vec3>("vposition", points);
    skyboxMesh->set_triangles(indices);
}

void drawSkybox() {
    skyboxShader->bind();

    // TODO: Set transformations, look vector, View matrix, Perspective matrix
    
    
    Mat4x4 M = Mat4x4::Identity(); // Identity should be fine
    skyboxShader->set_uniform("M", M);

    // TODO: Generate and set View, V matrix and set it as a uniform variable of the terrainShader. use lookAt() function.
    Vec3 look = cameraFront + cameraPos;
    //Mat4x4 V = lookAt(Vec3(0.5, 0, 1.0), Vec3(0.5, 0.5, 0), Vec3(0, 1, 0));
    Mat4x4 V = lookAt(cameraPos, look, Vec3(0, 0, 1));
    skyboxShader->set_uniform("V", V);

    // TODO: Generate and set Projection, P matrix and set it as a uniform variable of the terrainShader. use OpenGP::perspective()/glm::persepctive
    Mat4x4 P = perspective(80.0f, width / (float)height, 0.1f, 60.0f);
    skyboxShader->set_uniform("P", P);

    // TODO: Bind Textures and set uniform. HINT: Use GL_TEXTURE0, and texture type GL_TEXTURE_CUBE_MAP with skyboxShader and set the uniform variable noiseTex.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    skyboxShader->set_uniform("noiseTex", 0);
    skyboxShader->set_uniform("cloudMotion", cloudMotion);

    cloudMotion -= 0.00004f;
    if (cloudMotion < 0.3f) {
        cloudMotion = 0.5f;
    }
    // TODO: Bind Textures and set uniform. HINT: Use GL_TEXTURE0, and texture type GL_TEXTURE_CUBE_MAP with skyboxShader and set the uniform variable noiseTex.
    

    //Set atrributes and draw cube using GL_TRIANGLE_STRIP mode
    glEnable(GL_DEPTH_TEST);
    skyboxMesh->set_attributes(*skyboxShader);
    skyboxMesh->set_mode(GL_TRIANGLE_STRIP);
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(resPrim);
    skyboxMesh->draw();

    skyboxShader->unbind();
}
float rotation = 0.0f;
double prevTime = glfwGetTime();
void drawWater() {
    waterShader->bind();
    
    // TODO: Generate and set Model, M matrix and set it as a uniform variable for the terainShader. You may consider an identity matrix. 
    //Mat4x4 M = Mat4x4::Identity(); // Identity should be fine
    Mat4x4 M = Mat4x4::Identity();
    waterShader->set_uniform("M", M);

    // TODO: Generate and set View, V matrix and set it as a uniform variable of the terrainShader. use lookAt() function.
    Vec3 look = cameraFront + cameraPos;
    Mat4x4 V = lookAt(cameraPos, look, Vec3(0, 0, 1));
    waterShader->set_uniform("V", V);

    // TODO: Generate and set Projection, P matrix and set it as a uniform variable of the terrainShader. use OpenGP::perspective()/glm::persepctive
    Mat4x4 P = perspective(80.0f, width / (float)height, 0.1f, 60.0f);
    waterShader->set_uniform("P", P);
    
    // Set camera position
    waterShader->set_uniform("viewPos", cameraPos);

    // Bind textures
    int i = 0;
    for (std::map<std::string, std::unique_ptr<RGBA8Texture>>::iterator it = waterTextures.begin(); it != waterTextures.end(); ++it) {
        glActiveTexture(GL_TEXTURE1 + i);
        (it->second)->bind();
        waterShader->set_uniform(it->first.c_str(), 1 + i);
        ++i;
    }

    // TODO: Bind height texture to GL_TEXTURE0 and set uniform noiseTex.
    glActiveTexture(GL_TEXTURE0);
    heightTexture->bind();
    waterShader->set_uniform("noiseTex", 0);

    // Draw terrain using triangle strips
    glEnable(GL_DEPTH_TEST);
    waterMesh->set_attributes(*waterShader);
    waterMesh->set_mode(GL_TRIANGLE_STRIP);
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(resPrim);

    waterMesh->draw();

    // Generate wave motion and set uniform wave_motion
    waterShader->set_uniform("waveMotion", waveMotion);
    waveMotion += 0.00004f;
    if (waveMotion > 1.0f) {
        waveMotion = 0.4f;
    }

    waterShader->unbind();
}

void drawWater2() {
    water2Shader->bind();

    // TODO: Generate and set Model, M matrix and set it as a uniform variable for the terainShader. You may consider an identity matrix. 
    //Mat4x4 M = Mat4x4::Identity(); // Identity should be fine
    Mat4x4 M = Mat4x4::Identity();
    water2Shader->set_uniform("M", M);

    // TODO: Generate and set View, V matrix and set it as a uniform variable of the terrainShader. use lookAt() function.
    Vec3 look = cameraFront + cameraPos;
    Mat4x4 V = lookAt(cameraPos, look, Vec3(0, 0, 1));
    water2Shader->set_uniform("V", V);

    // TODO: Generate and set Projection, P matrix and set it as a uniform variable of the terrainShader. use OpenGP::perspective()/glm::persepctive
    Mat4x4 P = perspective(80.0f, width / (float)height, 0.1f, 60.0f);
    water2Shader->set_uniform("P", P);

    // Set camera position
    water2Shader->set_uniform("viewPos", cameraPos);

    // Bind textures
    int i = 0;
    for (std::map<std::string, std::unique_ptr<RGBA8Texture>>::iterator it = water2Textures.begin(); it != water2Textures.end(); ++it) {
        glActiveTexture(GL_TEXTURE1 + i);
        (it->second)->bind();
        water2Shader->set_uniform(it->first.c_str(), 1 + i);
        ++i;
    }

    // TODO: Bind height texture to GL_TEXTURE0 and set uniform noiseTex.
    glActiveTexture(GL_TEXTURE0);
    heightTexture->bind();
    water2Shader->set_uniform("noiseTex", 0);

    // Draw terrain using triangle strips
    glEnable(GL_DEPTH_TEST);
    water2Mesh->set_attributes(*water2Shader);
    water2Mesh->set_mode(GL_TRIANGLE_STRIP);
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(resPrim);

    water2Mesh->draw();

    // Generate wave motion and set uniform wave_motion
    water2Shader->set_uniform("waveMotion2", waveMotion2);
    waveMotion2 += 0.00004f;
    if (waveMotion2 > 1.0f) {
        waveMotion2 = 0.4f;
    }

    water2Shader->unbind();
}

void drawTerrain() {
    terrainShader->bind();

    // TODO: Generate and set Model, M matrix and set it as a uniform variable for the terainShader. You may consider an identity matrix. 
    Mat4x4 M = Mat4x4::Identity(); // Identity should be fine
    terrainShader->set_uniform("M", M);

    // TODO: Generate and set View, V matrix and set it as a uniform variable of the terrainShader. use lookAt() function.
    Vec3 look = cameraFront + cameraPos;
    Mat4x4 V = lookAt(cameraPos, look, Vec3(0, 0, 1));
    terrainShader->set_uniform("V", V);

    // TODO: Generate and set Projection, P matrix and set it as a uniform variable of the terrainShader. use OpenGP::perspective()/glm::persepctive
    Mat4x4 P = perspective(80.0f, width / (float)height, 0.1f, 60.0f);
    terrainShader->set_uniform("P", P);

    // Set camera position
    terrainShader->set_uniform("viewPos", cameraPos);

    // Bind textures
    int i = 0;
    for (std::map<std::string, std::unique_ptr<RGBA8Texture>>::iterator it = terrainTextures.begin(); it != terrainTextures.end(); ++it) {
        glActiveTexture(GL_TEXTURE1 + i);
        (it->second)->bind();
        terrainShader->set_uniform(it->first.c_str(), 1 + i);
        ++i;
    }

    // TODO: Bind height texture to GL_TEXTURE0 and set uniform noiseTex.
    glActiveTexture(GL_TEXTURE0);
    heightTexture->bind();
    terrainShader->set_uniform("noiseTex", 0);

    // Draw terrain using triangle strips
    glEnable(GL_DEPTH_TEST);
    terrainMesh->set_attributes(*terrainShader);
    terrainMesh->set_mode(GL_TRIANGLE_STRIP);
    glEnable(GL_PRIMITIVE_RESTART);
    glPrimitiveRestartIndex(resPrim);

    terrainMesh->draw();

    // Generate wave motion and set uniform wave_motion
    terrainShader->set_uniform("waveMotion", waveMotion);
    waveMotion += 0.00004f;
    if (waveMotion > 0.5f) {
        waveMotion =0.4f;
    }

    terrainShader->unbind();
}


