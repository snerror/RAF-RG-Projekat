//
// Created by Vladeta Putnikovic on 14.1.21..
//

#ifndef RAF_RG_PROJEKAT_VLADETAPUTNIKOVIC_WATER_H
#define RAF_RG_PROJEKAT_VLADETAPUTNIKOVIC_WATER_H

class Water {
public:
    Water();

    std::vector<float> *initVertices(int size, float width, float height) {
        auto vertices = new std::vector<float>();

        float start = -(size / 2) * width;

        for (int x = 0; x < size; x++) {
            float pos_x = start + x * width;
            for (int z = 0; z < size; z++) {
                float pos_z = -(start + z * width);
                vertices->push_back(pos_x);
                vertices->push_back(height);
                vertices->push_back(pos_z);

                // Texture coordinates
                vertices->push_back(pos_x);
                vertices->push_back(pos_z);
            }
        }
        return vertices;
    }

    std::vector<unsigned int> *initIndices(int size) {
        auto indices = new std::vector<unsigned int>();

        for (int z = 0; z < size - 1; ++z) {
            for (int x = 0; x < size - 1; ++x) {
                int start = x + z * size;
                indices->push_back(start);
                indices->push_back(start + 1);
                indices->push_back(start + size);
                indices->push_back(start + 1);
                indices->push_back(start + 1 + size);
                indices->push_back(start + size);
            }
        }

        return indices;
    }

    unsigned int createVAO(std::vector<float> *vertices, std::vector<unsigned int> *indices) {
        unsigned int VBO, VAO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices->size() * sizeof(float), &vertices->at(0), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size() * sizeof(unsigned int), &indices->at(0), GL_STATIC_DRAW);

        // Position attributes
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        return VAO;
    }

private:
};

Water::Water() = default;


#endif //RAF_RG_PROJEKAT_VLADETAPUTNIKOVIC_WATER_H
