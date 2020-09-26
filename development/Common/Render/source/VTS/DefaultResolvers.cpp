#include "VTS/DefaultResolvers.h"
#include "VTS/RenderResolvedAssets.h"
#include "VTS/RenderConfigAssets.h"
#include "VTS/RenderStateResolvedAssets.h"
#include "TextureResource.h"
#include "Resources/ResourceView.h"
#include "Resources/GeometryResource.h"
#include "Resources/ConstantsResource.h"
#include "Resources/DescriptionResource.h"
#include "Resources/FontResource.h"
#include "Resources/SkinShaderResource.h"
#include "Resources/DepthStencilStateResource.h"
#include "Resources/RasterizerStateResource.h"
#include "Resources/BlendStateResource.h"


namespace
{
    yaget::io::tool::AssetResolvers& Resolvers()
    {
        static yaget::io::tool::AssetResolvers resolvers = {
            { "PAK",            &yaget::io::ResolveAsset<yaget::io::render::PakAsset> } ,
            { "GEOM",           &yaget::io::ResolveAsset<yaget::io::render::GeomAsset> },
            { "SHADER",         &yaget::io::ResolveAsset<yaget::io::render::ShaderAsset> },
            { "DESC",           &yaget::io::ResolveAsset<yaget::io::render::DescriptionAsset> },
            { "MAT",            &yaget::io::ResolveAsset<yaget::io::render::MaterialAsset> },
            { "TEXTURE",        &yaget::io::ResolveAsset<yaget::io::render::TextureAsset> },
            { "IMAGEMETA",      &yaget::io::ResolveAsset<yaget::io::render::ImageMetaAsset> },
            { "FONT",           &yaget::io::ResolveAsset<yaget::io::render::FontAsset> },
            { "FONTFACE",       &yaget::io::ResolveAsset<yaget::io::render::FontFaceAsset> },
            { "DEPTHSTENCIL",   &yaget::io::ResolveAsset<yaget::io::render::DepthStencilStateAsset> },
            { "RASTERIZER",     &yaget::io::ResolveAsset<yaget::io::render::RasterizerStateAsset> },
            { "BLEND",          &yaget::io::ResolveAsset<yaget::io::render::BlendStateAsset> },
            { "JSON",           &yaget::io::ResolveAsset<yaget::io::JsonAsset> },
            { "FONTBITMAP",     &yaget::io::ResolveAsset<yaget::io::render::FontAsset> },
            { "IMAGE",          &yaget::io::ResolveAsset<yaget::io::ImageAsset> },
            { "RENDER_TARGET",  &yaget::io::ResolveAsset<yaget::io::render::RenderTargetAsset> },
        };

        return resolvers;
    }

    yaget::io::tool::TagResourceResolvers& TagResolvers()
    {
        static yaget::io::tool::TagResourceResolvers resolvers = {
            { { "PAK",          std::type_index(typeid(yaget::render::GeometryResource)) },                    &yaget::io::tool::ConvertFromTag<yaget::render::GeometryResource, yaget::io::render::PakAsset> } ,
            { { "GEOM",         std::type_index(typeid(yaget::render::GeometryResource)) },                    &yaget::io::tool::ConvertFromTag<yaget::render::GeometryResource, yaget::io::render::GeomAsset> },
            { { "SHADER",       std::type_index(typeid(yaget::render::VertexShaderResource)) },                &yaget::io::tool::ConvertFromTag<yaget::render::VertexShaderResource, yaget::io::render::ShaderAsset> },
            { { "SHADER",       std::type_index(typeid(yaget::render::ConstantsResource)) },                   &yaget::io::tool::ConvertFromTag<yaget::render::ConstantsResource, yaget::io::render::ShaderAsset> },
            { { "SHADER",       std::type_index(typeid(yaget::render::InputLayoutResource)) },                 &yaget::io::tool::ConvertFromTag<yaget::render::InputLayoutResource, yaget::io::render::ShaderAsset> },
            { { "SHADER",       std::type_index(typeid(yaget::render::PixelShaderResource)) },                 &yaget::io::tool::ConvertFromTag<yaget::render::PixelShaderResource, yaget::io::render::ShaderAsset> },
            { { "DESC",         std::type_index(typeid(yaget::render::DescriptionResource)) },                 &yaget::io::tool::ConvertFromTag<yaget::render::DescriptionResource, yaget::io::render::DescriptionAsset> },
            { { "MAT",          std::type_index(typeid(yaget::render::SkinShaderResource)) },                  &yaget::io::tool::ConvertFromTag<yaget::render::SkinShaderResource, yaget::io::render::MaterialAsset> },
            { { "TEXTURE",      std::type_index(typeid(yaget::render::TextureResource)) },                     &yaget::io::tool::ConvertFromTag<yaget::render::TextureResource, yaget::io::render::TextureAsset> },
            { { "IMAGEMETA",    std::type_index(typeid(yaget::render::TextureMetaResource)) },                 &yaget::io::tool::ConvertFromTag<yaget::render::TextureMetaResource, yaget::io::render::ImageMetaAsset> },
            { { "DEPTHSTENCIL", std::type_index(typeid(yaget::render::state::DepthStencilStateResource)) },    &yaget::io::tool::ConvertFromTag<yaget::render::state::DepthStencilStateResource, yaget::io::render::DepthStencilStateAsset> },
            { { "RASTERIZER",   std::type_index(typeid(yaget::render::state::RasterizerStateResource)) },      &yaget::io::tool::ConvertFromTag<yaget::render::state::RasterizerStateResource, yaget::io::render::RasterizerStateAsset> },
            { { "BLEND",        std::type_index(typeid(yaget::render::state::BlendStateResource)) },           &yaget::io::tool::ConvertFromTag<yaget::render::state::BlendStateResource, yaget::io::render::BlendStateAsset> },
            { { "FONTBITMAP",   std::type_index(typeid(yaget::render::FontResource)) },                        &yaget::io::tool::ConvertFromTag<yaget::render::FontResource, yaget::io::render::FontAsset> },
            { { "IMAGE",        std::type_index(typeid(yaget::render::TextureImageResource)) },                &yaget::io::tool::ConvertFromTag<yaget::render::TextureImageResource, yaget::io::ImageAsset> },
        };

        return resolvers;
    }

} // namespace


const yaget::io::tool::AssetResolvers& yaget::io::tool::GetResolvers()
{
    const auto& resolvers = Resolvers();
    return resolvers;
}


const yaget::io::tool::TagResourceResolvers& yaget::io::tool::GetTagResolvers()
{
    const auto& resolvers = TagResolvers();
    return resolvers;
}


void yaget::io::tool::AddResolvers(const AssetResolvers& assetResolvers)
{
    auto& resolvers = Resolvers();
    resolvers.insert(std::begin(assetResolvers), std::end(assetResolvers));
}


void yaget::io::tool::AddTagResolvers(const TagResourceResolvers& tagResolvers)
{
    auto& resolvers = TagResolvers();
    resolvers.insert(std::begin(tagResolvers), std::end(tagResolvers));
}
