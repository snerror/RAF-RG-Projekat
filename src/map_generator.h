#ifndef RAF_RG_PROJEKAT_VLADETAPUTNIKOVIC_MAP_GENERATOR_H
#define RAF_RG_PROJEKAT_VLADETAPUTNIKOVIC_MAP_GENERATOR_H

#include "perlin.h"

const float VERTEX_COUNT = 200;
float WATER_HEIGHT = 0.1;

int octaves = 5;
float meshHeight = 32;
float noiseScale = 64;
float persistence = 0.5;
float lacunarity = 2;

std::vector<int> generateIndices() {
    std::vector<int> indices;

    int pos = 0;
    for (int y = 0; y < VERTEX_COUNT; y++)
        for (int x = 0; x < VERTEX_COUNT; x++) {

            if (x == VERTEX_COUNT - 1 || y == VERTEX_COUNT - 1) {
                // Don't create indices for right or top edge
                pos++;
                continue;
            } else {
                // Top left triangle of square
                indices.push_back(pos + VERTEX_COUNT);
                indices.push_back(pos);
                indices.push_back(pos + VERTEX_COUNT + 1);
                // Bottom right triangle of square
                indices.push_back(pos + 1);
                indices.push_back(pos + 1 + VERTEX_COUNT);
                indices.push_back(pos);
            }
            pos++;
        }

    return indices;
}

float randomModifier() {
    float modifier = ((float) rand()) / RAND_MAX;
    srand((unsigned int) time(NULL));
    modifier = ((float) rand()) / RAND_MAX;
    modifier = ((float) rand()) / RAND_MAX;
    return modifier + 1.0f;
}

std::vector<float> generateNoiseMap() {
    int offsetX = 0;
    int offsetY = 0;
    std::vector<float> noiseValues;
    std::vector<float> normalizedNoiseValues;
    std::vector<int> p = get_permutation_vector();

    float amp = 1;
    float freq = 1;
    float maxPossibleHeight = 0;

    for (int i = 0; i < octaves; i++) {
        maxPossibleHeight += amp;
        amp *= persistence;
    }

    for (int y = 0; y < VERTEX_COUNT; y++) {

        for (int x = 0; x < VERTEX_COUNT; x++) {
            float modifier = randomModifier();
            amp = 1;
            freq = 1;
            float noiseHeight = 0;
            for (int i = 0; i < octaves; i++) {
                float xSample = (x + offsetX * (VERTEX_COUNT - 1)) / noiseScale * freq;
                float ySample = (y + offsetY * (VERTEX_COUNT - 1)) / noiseScale * freq;

                float perlinValue = perlin_noise(xSample * modifier, ySample * modifier, p);
                noiseHeight += perlinValue * amp;
                amp *= persistence;
                freq *= lacunarity;
            }

            noiseValues.push_back(noiseHeight);
        }
    }

    for (int y = 0; y < VERTEX_COUNT; y++) {
        for (int x = 0; x < VERTEX_COUNT; x++) {
            // Inverse lerp and scale values to range from 0 to 1
            normalizedNoiseValues.push_back((noiseValues[x + y * VERTEX_COUNT] + 1) / maxPossibleHeight);
        }
    }

    return normalizedNoiseValues;
}

std::vector<float> generateVertices(const std::vector<float> &noise_map) {
    std::vector<float> v;

    for (int y = 0; y < VERTEX_COUNT + 1; y++)
        for (int x = 0; x < VERTEX_COUNT; x++) {
            v.push_back(x);
            float easedNoise = std::pow(noise_map[x + y * VERTEX_COUNT] * 1.1, 3);
            v.push_back(std::fmax(easedNoise * meshHeight, WATER_HEIGHT * 0.5 * meshHeight));
            v.push_back(y);
        }

    return v;
}

std::vector<float> generateNormals(const std::vector<int> &indices, const std::vector<float> &vertices) {
    int pos;
    glm::vec3 normal;
    std::vector<float> normals;
    std::vector <glm::vec3> verts;

    // Get the vertices of each triangle in mesh
    // For each group of indices
    for (int i = 0; i < indices.size(); i += 3) {

        // Get the vertices (point) for each index
        for (int j = 0; j < 3; j++) {
            pos = indices[i + j] * 3;
            verts.push_back(glm::vec3(vertices[pos], vertices[pos + 1], vertices[pos + 2]));
        }

        // Get vectors of two edges of triangle
        glm::vec3 U = verts[i + 1] - verts[i];
        glm::vec3 V = verts[i + 2] - verts[i];

        // Calculate normal
        normal = glm::normalize(-glm::cross(U, V));
        normals.push_back(normal.x);
        normals.push_back(normal.y);
        normals.push_back(normal.z);
    }

    return normals;
}

glm::vec3 get_color(int r, int g, int b) {
    return glm::vec3(r / 255.0, g / 255.0, b / 255.0);
}


struct terrainColor {
    terrainColor(float _height, glm::vec3 _color) {
        height = _height;
        color = _color;
    };
    float height;
    glm::vec3 color;
};

std::vector<float> generateColors(const std::vector<float> &vertices) {
    std::vector<float> colors;
    std::vector <terrainColor> biomeColors;
    glm::vec3 color = get_color(255, 255, 255);

    // NOTE: Terrain color height is a value between 0 and 1
    biomeColors.push_back(terrainColor(WATER_HEIGHT * 0.5, get_color(60, 95, 190)));   // Deep water
    biomeColors.push_back(terrainColor(WATER_HEIGHT, get_color(60, 100, 190)));  // Shallow water
    biomeColors.push_back(terrainColor(0.15, get_color(210, 215, 130)));                // Sand
    biomeColors.push_back(terrainColor(0.30, get_color(95, 165, 30)));                // Grass 1
    biomeColors.push_back(terrainColor(0.40, get_color(65, 115, 20)));                // Grass 2
    biomeColors.push_back(terrainColor(0.50, get_color(90, 65, 60)));                // Rock 1
    biomeColors.push_back(terrainColor(0.80, get_color(75, 60, 55)));                // Rock 2
    biomeColors.push_back(terrainColor(1.00, get_color(255, 255, 255)));                // Snow

    for (int i = 1; i < vertices.size(); i += 3) {
        for (int j = 0; j < biomeColors.size(); j++) {
            if (vertices[i] <= biomeColors[j].height * meshHeight) {
                color = biomeColors[j].color;
                break;
            }
        }
        colors.push_back(color.r);
        colors.push_back(color.g);
        colors.push_back(color.b);
    }
    return colors;
}

void generateMap(unsigned int &VAO) {
    std::vector<int> indices;
    std::vector<float> noise_map;
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> colors;

    // Generate map
    indices = generateIndices();
    noise_map = generateNoiseMap();
    vertices = generateVertices(noise_map);
    normals = generateNormals(indices, vertices);
    colors = generateColors(vertices);

    unsigned int pVBO, nVBO, cVBO, EBO;

    // Create buffers and arrays
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &pVBO);
    glGenBuffers(1, &nVBO);
    glGenBuffers(1, &cVBO);
    glGenBuffers(1, &EBO);

    // Bind vertices to VBO
    glBindVertexArray(VAO);

    // Create element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), &indices[0], GL_STATIC_DRAW);

    // position
    glBindBuffer(GL_ARRAY_BUFFER, pVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // normals
    glBindBuffer(GL_ARRAY_BUFFER, nVBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), &normals[0], GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // colors
    glBindBuffer(GL_ARRAY_BUFFER, cVBO);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), &colors[0], GL_STATIC_DRAW);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) 0);
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindVertexArray(0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
}

#endif

