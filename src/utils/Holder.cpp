#include "Holder.h"

namespace sludge::utils
{
	void destroy(TextureHandle handle)
	{
	}
	void destroy(GeometryHandle handle)
	{
	}
	void destroy(PassConstantHandle handle)
	{
	}
	void destroy(ModelConstantHandle handle)
	{
	}
	uint32_t offset(GeometryHandle handle)
	{
		return STRUCTURED_BUFFER_START;
	}
	uint32_t offset(TextureHandle handle)
	{
		return TEXTURE_START;
	}
	uint32_t offset(TextureUAVHandle handle)
	{
		return TEXTURE_UAV_START;
	}
	uint32_t offset(PassConstantHandle handle)
	{
		return PASS_CONSANT_START;
	}
	uint32_t offset(ModelConstantHandle handle)
	{
		return MODEL_CONSTANT_START;
	}
}