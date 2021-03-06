#pragma once

#include <cstdlib>
#include "OpenGP/GL/Application.h"

using namespace OpenGP;

inline float lerp(float x, float y, float t) {

    // Linear interpolation between x and y
    float interpolate = x + t * (y - x);
    return interpolate;
}

inline float fade(float t) {
    // Quintic interpolation curve
    return t * t * t * (t * (t * 6 - 15) + 10);

    // Cubic interpolation curve
    //return t * t * (3.0f - 2.0f * t);
}

inline float rand01() {
    return ((float) std::rand())/((float) RAND_MAX);
}

float* perlin2D(const int width, const int height, const int period=64);

// Generates a heightmap using regular fBm (fractional brownian motion)
R32FTexture* fBm2DTexture() {

    // Precompute perlin noise on a 2D grid
   
    const int width = 2048;
    const int height = 2048;
    float *perlin_data = perlin2D(width, height, 512);

    
    // fBm parameters - Summer
    float H = 0.9f;
    float lacunarity = 2.0f;
    float offset = 0.1f;
    const int octaves = 5;
    


    // Initialize to 0s
    float *noise_data = new float[width * height];
    for (int i = 0; i < width; ++ i) {
        for (int j = 0; j < height; ++ j) {
            noise_data[i + j * height] = 0;
        }
    }

    // Precompute exponent array
    float *exponent_array = new float[octaves];
    float f = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        exponent_array[i] = std::pow(f, -H);
        f *= lacunarity;
    }

    for (int i = 0; i < width; ++ i) {
        for (int j = 0; j < height; ++ j) {

            // TODO: Index (for use in inner loop - frequency)) -I & J
            int I = i;
            int J = j;

            for(int k = 0; k < octaves; ++k) {

                // TODO: Generate perlin value
                float perlin = perlin_data[(I % width) + (J % height) * height];



                // TODO: Generate noise value
                noise_data[i + j * height] += (perlin + offset) * exponent_array[k];



                // TODO: Point to sample at next octave
                I *= (int)lacunarity;
                J *= (int)lacunarity;
                
            }
        }
    }

    R32FTexture* _tex = new R32FTexture();
    _tex->upload_raw(width, height, noise_data);

    // Clean up
    delete perlin_data;
    delete noise_data;
    delete exponent_array;

    return _tex;
}

// Generates a height map using Hybrid Multifractal fBM (fractional Brownian motion)[optional]
R32FTexture* HybridMultifractal2DTexture() {

    // Precompute perlin noise on a 2D grid
    const int width = 2048;
    const int height = 2048;
    float *perlin_data = perlin2D(width, height, 512);
    
    // fBm parameters - Lunar
    /*
    float H = 0.8f;
    float lacunarity = 4.0f;
    float offset = 0.7f;
    const int octaves = 43;
    */
    float H = 0.9f;
    float lacunarity = 2.0f;
    float offset = 0.1f;
    const int octaves = 5;
    /*
    float H = 0.25f;
    float lacunarity = 2.0f;
    float offset = 0.7f;
    const int octaves = 16;
    */

    // Initialize to 0s
    float *noise_data = new float[width * height];
    for (int i = 0; i < width; ++ i) {
        for (int j = 0; j < height; ++ j) {
            noise_data[i + j * height] = 0.0f;
        }
    }

    // Precompute exponent array
    float *exponent_array = new float[octaves];
    float f = 1.0f;
    for (int i = 0; i < octaves; ++i) {
        exponent_array[i] = pow( f, -H);
        f *= lacunarity;
    }

    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {

            // TODO: Index (for use in inner loop - frequency)
            int I = i;
            int J = j;

            // TODO: Generate Perlin value (Hybrid Multifractal (1 - abs(perlin)))
            float perlin = 1- abs(perlin_data[(I % width) + (J % height) * height]);

            //float result = (perlin + offset) * exponent_array[0];
            float weight = perlin;

            //TODO:  Point to sample at next octave
            I *= (int)lacunarity;
            J *= (int)lacunarity;

            for (int k = 1; k < octaves; k++) {

                // TODO: Restrict weight to be less than one (guard against divergence)
                if (weight > 1.0) {
                    weight = 1.0;
                }

                // TODO:Generate Perlin value
                float signal = (perlin + offset) * exponent_array[k];

                // TODO: Add weighted Perlin value to Perlin
                perlin += weight * signal;
                // TODO: Adjust weighting value
                weight *= signal;
                // TODO: Point to sample at next octave
                I *= (int)lacunarity;
                J *= (int)lacunarity;
            }

            // Generate noise value
            int noiseIndex = i + j * height;
            noise_data[noiseIndex] = perlin;
        }
    }

    R32FTexture* _tex = new R32FTexture();
    _tex->upload_raw(width, height, noise_data);

    // Clean up
    delete perlin_data;
    delete noise_data;
    delete exponent_array;

    return _tex;
}

float* perlin2D(const int width, const int height, const int period) {

    // Precompute random gradients
    float *gradients = new float[width * height * 2];
    auto sample_gradient = [&](int i, int j) {
        float x = gradients[2 * (i + j * height)];
        float y = gradients[2 * (i + j * height) + 1];
        return Vec2(x,y);
    };

    for (int i = 0; i < width; ++ i) {
        for (int j = 0; j < height; ++ j) {
            float angle = rand01();
            gradients[2 * (i + j * height)] = cos(2 * angle * M_PI);
            gradients[2 * (i + j * height) + 1] = sin(2 * angle * M_PI);
        }
    }

    // Perlin Noise parameters
    float frequency = 1.0f / period;

    float *perlin_data = new float[width*height];
    for (int i = 0; i < width; ++ i) {
        for (int j = 0; j < height; ++ j) {

            // Integer coordinates of corners
            int left = (i / period) * period;
            int right = (left + period) % width;
            int top = (j / period) * period;
            int bottom = (top + period) % height;

            //TODO: noise generation
            float dx = (i - left) * frequency;
            float dy = (j - top) * frequency;

            Vec2 topleft = sample_gradient(left, top);
            Vec2 topright = sample_gradient(right, top);
            Vec2 bottomleft = sample_gradient(left, bottom);
            Vec2 bottomright = sample_gradient(right, bottom);



            Vec2 a(dx, -dy); // topleft
            Vec2 b(dx - 1, -dy); // topright
            Vec2 c(dx, 1 - dy); // bottomleft
            Vec2 d(dx - 1, 1 - dy); // bottomright



            float s = topleft.dot(a);
            float t = topright.dot(b);
            float u = bottomleft.dot(c);
            float v = bottomright.dot(d);



            float st = lerp(s, t, fade(dx));
            float uv = lerp(u, v, fade(dx));



            float noise = lerp(st, uv, fade(dy));
            

            perlin_data[i + j * height] = noise;
        }
    }

    delete gradients;
    return perlin_data;
}
