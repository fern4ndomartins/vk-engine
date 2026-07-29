#include "../include/model.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void Model::load(const char* filepath) {
    cgltf_options options = {};
    cgltf_result result = cgltf_parse_file(&options, filepath, &data);
    if (result != cgltf_result_success) throw std::runtime_error("failed to load model\n");

    result = cgltf_load_buffers(&options,data,filepath);
    printf("%d\n\n\n\n", data->textures_count);
    for (int meshIndex = 0; meshIndex<data->meshes_count; meshIndex++) {
        uint32_t globalIndex = indices.size();
        int attributeCount = data->meshes[meshIndex].primitives[0].attributes_count;
        cgltf_accessor indexThing = *data->meshes[meshIndex].primitives[0].indices;
        cgltf_buffer_view *bview = indexThing.buffer_view;
        uint8_t *d = static_cast<uint8_t*>(bview->buffer->data);
        cgltf_size count = indexThing.count;
        switch (indexThing.component_type) {
            case cgltf_component_type_r_8u:
                for (int j = 0; j < count; j++) {
                    size_t byteOffset = bview->offset + indexThing.offset + j * indexThing.stride;
                    uint8_t index;
                    memcpy(&index, d + byteOffset, sizeof(index));
                    indices.push_back(static_cast<uint32_t>(index) + globalIndex); 
                }
                break;
            case cgltf_component_type_r_16u:
                for (int j = 0; j < count; j++) {
                    size_t byteOffset = bview->offset + indexThing.offset + j * indexThing.stride;
                    uint16_t index;
                    memcpy(&index, d + byteOffset, sizeof(index));
                    indices.push_back(static_cast<uint32_t>(index) + globalIndex); 
                }
                break;

            case cgltf_component_type_r_32u:
                for (int j=0;j<count;j++) {
                    size_t byteOffset = bview->offset + indexThing.offset + j * indexThing.stride;
                    uint32_t index;
                    memcpy(&index, d + byteOffset, sizeof(index));
                    indices.push_back(index+globalIndex);
                }
                break;

            case cgltf_component_type_invalid:
            case cgltf_component_type_r_8:
            case cgltf_component_type_r_16:
            case cgltf_component_type_r_32f:
            case cgltf_component_type_max_enum:
                break;
            }

        for (int attributeIndex = 0 ; attributeIndex< attributeCount ; attributeIndex++) {
            cgltf_attribute att = data->meshes[meshIndex].primitives[0].attributes[attributeIndex];
            if (att.type == cgltf_attribute_type_position) {
                cgltf_accessor acc = *att.data;
                cgltf_buffer_view *bview = acc.buffer_view;
                void*d = bview->buffer->data;

                cgltf_size count = acc.count;
                if (acc.type != 3) continue;
                for (int j=0;j<count;j++) {
                    int idx = bview->offset+acc.offset+(acc.stride*j);
                    glm::vec3 vertcoord;
                    memcpy(&vertcoord, (uint8_t*)d + idx, sizeof(glm::vec3));
                    positions.push_back(vertcoord);
                }
            }
            if (att.type == cgltf_attribute_type_normal) {
                cgltf_accessor acc = *att.data;
                cgltf_buffer_view *bview = acc.buffer_view;
                void*d = bview->buffer->data;

                cgltf_size count = acc.count;
                if (acc.type != 3) continue;
                for (int j=0;j<count;j++) {
                    int idx = bview->offset+acc.offset+(acc.stride*j);
                    glm::vec3 vertcoord;
                    memcpy(&vertcoord, (uint8_t*)d + idx, sizeof(glm::vec3));
                    normals.push_back(vertcoord);
                }
            }
            if (att.type == cgltf_attribute_type_color) printf("colorr\n");
            if (att.type == cgltf_attribute_type_texcoord && att.index == 0) {

                cgltf_accessor acc = *att.data;
                cgltf_buffer_view *bview = acc.buffer_view;
                void*d = bview->buffer->data;

                cgltf_size count = acc.count;
                if (acc.type != 2) continue;
                for (int j=0;j<count;j++) {
                    int idx = bview->offset+acc.offset+(acc.stride*j);
                    glm::vec2 vertcoord;
                    memcpy(&vertcoord, (uint8_t*)d + idx, sizeof(glm::vec2));
                    uvs.push_back(vertcoord);
               }
            }
        }
    }
}

void Model::loadImage() {
    int texWidth, texHeight, texChannels;
    // stbi_uc *pixels = stbi_load_from_memory();
 
}


