#pragma once

namespace sludge
{
	struct MeshMaterial
	{
		std::string albedo{};
		std::string normalMap{};
		std::string aoMap{};
		std::string metallicRoughnessMap{};
		std::string emissiveMap{};
	};
	struct ModelMesh
	{
		IndexBuffer indexBuffer{};
		utils::Holder<utils::GeometryHandle> vbHolder{};
		// needed for mikktspace lib for tangent calculations
		std::vector<Vertex> Vertices{};
		std::vector<uint32_t> Indices{};

		uint32_t vertexCount{};
		uint32_t indexCount{};
	};

	//struct Transform
	//{
	//	DirectX::XMFLOAT3 Rotation{};
	//	DirectX::XMFLOAT3 Scale{};
	//	DirectX::XMFLOAT3 Translation{};
	//};

	// Since we are using a data driven approach, model data will effectively function as our models now. Except now we use indices to index into any given model that
	// we may want to use. Pretty much the same schtick with how our scene graph works.
	struct ModelData
	{
		std::vector<utils::Holder<utils::ModelConstantHandle>> cbHolders{};
		std::vector<std::string> modelDirectories{};

		std::vector<uint32_t> vertexCounts{};
		std::vector<uint32_t> indexCounts{};
		std::vector<ModelMesh> meshes{};
		//std::vector<Transform> transforms;
		std::vector<MeshMaterial> materials{};

		// This single map will keep a hold of all our textures so we arent reuploading data more than once.
		static inline std::map<std::string_view, Texture> loadedTextures{};
	};
}