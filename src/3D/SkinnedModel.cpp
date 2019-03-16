/* OpenBlack - A reimplementation of Lionheads's Black & White engine
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <3D/SkinnedModel.h>
#include <Common/OSFile.h>

#include <stdint.h>
#include <stdexcept>

using namespace OpenBlack;
using namespace OpenBlack::Graphics;

struct L3DHeader {
	uint32_t modelSize;
	uint32_t skinOffset;
	uint32_t numMeshes;
	uint32_t meshListOffset; // L3D_Mesh
	uint32_t unknown_1;
	uint32_t unknown_2;
	uint32_t unknown_3;
	uint32_t unknown_4;
	uint32_t unknown_5;
	uint32_t unknown_6;
	uint32_t unknown_7;
	uint32_t unknown_8;
	uint32_t unknown_9;
	uint32_t numSkins;
	uint32_t skinListOffset;
	uint32_t unknown_10;
	uint32_t unknown_11;
	uint32_t pSkinName;
};

struct L3D_Mesh {
	uint8_t unknown_1;
	uint8_t unknown_2; // 20: nodraw
	uint8_t unknown_3;
	uint8_t unknown_4; // e0: transparent

	uint32_t numSubMeshes;
	uint32_t subMeshOffset; // L3D_SubMesh
	uint32_t numBones;
	uint32_t bonesOffset;
};

struct L3D_SubMesh {
	uint32_t unknown_1;
	uint32_t unknown_2;
	int32_t skinID;
	uint32_t unknown_3;

	uint32_t numVerticies;
	uint32_t verticiesOffset;
	uint32_t numTriangles;
	uint32_t trianglesOffset;
	uint32_t boneVertLUTSize;
	uint32_t boneVertLUTOffset;
	uint32_t numVertexBlends;
	uint32_t vertexBlendsOffset;
};

struct L3D_Vertex {
	float position[3];
	float texCoords[2];
	float normal[3];
};

struct L3D_Triangle {
	uint16_t indices[3];
};

struct LH3D_BoneVert
{
	uint16_t nVertices;
	uint16_t boneIndex;
};

struct L3D_Skin {
	uint32_t skinID;
	uint16_t data[256 * 256]; // RGBA4444
};

struct L3D_Bone {
	int32_t parentBone; // -1 = root
	int32_t childBone; // -1 = no children
	int32_t siblingBone; // -1 = no siblings

	// rotation matrix
	float rotXAxis[3];
	float rotYAxis[3];
	float rotZAxis[3];

	// bone origin position
	float position[3];
};

void SkinnedModel::LoadFromFile(std::string &fileName)
{
	size_t meshSize;
	char* mesh = OSFile::ReadAll((fileName).c_str(), &meshSize);
	LoadFromL3D(mesh, meshSize);
	delete[] mesh;
}

void SkinnedModel::LoadFromL3D(void* data_, size_t size) {
	uint8_t* buffer = static_cast<uint8_t*>(data_);
	if (buffer[0] != 'L' || buffer[1] != '3' || buffer[2] != 'D' || buffer[3] != '0') {
		throw std::runtime_error("Invalid L3D file");
	}

	L3DHeader* header = (L3DHeader*)(buffer + 4);
	uint32_t* meshOffsets = (uint32_t*)(buffer + header->meshListOffset);

	printf("loading mesh with %d meshes (only 1st will be loaded)\n", header->numMeshes);

	for (int m = 0; m < header->numMeshes; m++)
	{
		L3D_Mesh* mesh = (L3D_Mesh*)(buffer + meshOffsets[m]);

		_submeshes.reserve(mesh->numSubMeshes);

		uint32_t* submeshOffsets = (uint32_t*)(buffer + mesh->subMeshOffset);
		for (int sm = 0; sm < mesh->numSubMeshes; sm++)
		{
			L3D_SubMesh* subMesh = (L3D_SubMesh*)(buffer + submeshOffsets[sm]);

			L3D_Vertex* verticiesOffset = static_cast<L3D_Vertex*>((void*)(buffer + subMesh->verticiesOffset));
			L3D_Triangle* trianglesOffset = static_cast<L3D_Triangle*>((void*)(buffer + subMesh->trianglesOffset));
			LH3D_BoneVert* boneVertOffset = static_cast<LH3D_BoneVert*>((void*)(buffer + subMesh->boneVertLUTOffset));

			//Mesh* sub = new Mesh();

			VertexDecl decl(3);
			decl[0] = VertexAttrib(0, 3, GL_FLOAT, 32, (void*)0);
			decl[1] = VertexAttrib(1, 2, GL_FLOAT, 32, (void*)12);
			decl[2] = VertexAttrib(2, 3, GL_FLOAT, 32, (void*)20);

			VertexBuffer *vertexBuffer = new VertexBuffer(verticiesOffset, subMesh->numVerticies, sizeof(L3D_Vertex));
			IndexBuffer *indexBuffer = new IndexBuffer(trianglesOffset, subMesh->numTriangles * 3, GL_UNSIGNED_SHORT);

			_submeshes.emplace_back(std::make_unique<Mesh>(vertexBuffer, indexBuffer, decl));
			_submeshSkinMap[sm] = subMesh->skinID;
		}

		L3D_Bone* bones = (L3D_Bone*)(buffer + mesh->bonesOffset);
		for (int b = 0; b < mesh->numBones; b++)
		{
			L3D_Bone bone = bones[b];
			printf("bone[%d] - parent=%d\n", b, bone.parentBone);
		}

		// stop idk how we should handle more then 1 mesh yet!
		break;
	}

	// Inside packed meshes, there are no skins.
	uint32_t* skinOffsets = (uint32_t*)(buffer + header->skinListOffset);

	for (int s = 0; s < header->numSkins; s++) {
		L3D_Skin* skin = static_cast<L3D_Skin*>((void*)(buffer + skinOffsets[s]));
		_textures[skin->skinID] = std::make_unique<Texture2D>(256, 256, GL_RGB5_A1, GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4_REV, skin->data);
	}
}

void SkinnedModel::Draw(ShaderProgram* program) {
	for (int i = 0; i < _submeshes.size(); i++)
	{
		_textures[_submeshSkinMap[i]]->Bind(0);
		_submeshes[i]->Draw();
	}


	/*
		m_matrixBuffer->SetBuffer(m_animatingSkeleton.GetBoneMatrices(), boneCount * sizeof(Matrix4x4));

		GLExt::glBindBufferARB(m_data.m_target, m_bufferHandle);
		GLExt::glBufferDataARB(m_data.m_target, m_data.m_buffer.size(), &m_data.m_buffer[0], GL_DYNAMIC_READ);
		GLExt::glBindBufferARB(m_data.m_target, 0);

		glBindTexture(m_data.m_target, m_handle);
		GLExt::glTexBufferARB(m_data.m_target, GL_RGBA32F_ARB, m_bufferHandle);
		glBindTexture(m_data.m_target, 0);

	*/
}
