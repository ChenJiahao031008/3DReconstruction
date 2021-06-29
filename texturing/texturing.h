/*
 * Copyright (C) 2015, Nils Moehrle
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef TEX_TEXTURING_HEADER
#define TEX_TEXTURING_HEADER

#include <vector>

#include "core/mesh.h"
#include "core/mesh_info.h"

#include "./3rdParty/mrf/graph.h"

#include "defines.h"
#include "settings.h"
#include "obj_model.h"
#include "uni_graph.h"
#include "texture_view.h"
#include "texture_patch.h"
#include "texture_atlas.h"
#include "sparse_table.h"

#include "seam_leveling.h"

typedef SparseTable<std::uint32_t, std::uint16_t, float> ST;

TEX_NAMESPACE_BEGIN

typedef std::vector<TextureView> TextureViews;
typedef std::vector<TexturePatch::Ptr> TexturePatches;
typedef std::vector<TextureAtlas::Ptr> TextureAtlases;
typedef ObjModel Model;
typedef UniGraph Graph;
typedef ST DataCosts;
typedef std::vector<std::vector<VertexProjectionInfo> > VertexProjectionInfos;

/**
  * prepares the mesh for texturing
  *  -removes duplicated faces
  *  -ensures normals (face and vertex)
  */
void
prepare_mesh(core::VertexInfoList::Ptr vertex_infos, core::TriangleMesh::Ptr mesh);

/**
  * Generates TextureViews from the in_scene.
  */
void
generate_texture_views(std::string in_scene, TextureViews * texture_views);

/**
  * Builds up the meshes face adjacency graph using the vertex_infos
  */
void
build_adjacency_graph(core::TriangleMesh::ConstPtr mesh,
    core::VertexInfoList::ConstPtr vertex_infos, UniGraph * graph);

/**
 * Calculates the data costs for each face and texture view combination,
 * if the face is visible within the texture view.
 */
void
calculate_data_costs(core::TriangleMesh::ConstPtr mesh,
    TextureViews * texture_views, Settings const & settings, ST * data_costs);

/**
 * Runs the view selection procedure and saves the labeling in the graph
 */
void
view_selection(ST const & data_costs, UniGraph * graph, Settings const & settings);

  /**
   * \decription Generates texture patches using the graph to determine adjacent faces with the same label.
   * @param graph -- uniongraph with labels
   * @param mesh -- triangle mesh
   * @param vertex_infos -- vertex infos (vertex neighbours and facet neighbours)
   * @param texture_views -- (texture views)
   * @param vertex_projection_infos --
   * @param texture_patches --
   */
void generate_texture_patches(UniGraph const & graph,
    core::TriangleMesh::ConstPtr mesh,
    core::VertexInfoList::ConstPtr vertex_infos,
    TextureViews * texture_views,
    VertexProjectionInfos * vertex_projection_infos,
    TexturePatches * texture_patches);

/**
  * Runs the seam leveling procedure proposed by Ivanov and Lempitsky
  * [<A HREF="https://www.google.de/url?sa=t&rct=j&q=&esrc=s&source=web&cd=1&cad=rja&sqi=2&ved=0CC8QFjAA&url=http%3A%2F%2Fwww.robots.ox.ac.uk%2F~vilem%2FSeamlessMosaicing.pdf&ei=_ZbvUvSZIaPa4ASi7IGAAg&usg=AFQjCNGd4x5HnMMR68Sn2V5dPgmqJWErCA&sig2=4j47bXgovw-uks9LBGl_sA">Seamless mosaicing of image-based texture maps</A>]
  */
void
global_seam_leveling(UniGraph const & graph, core::TriangleMesh::ConstPtr mesh,
    core::VertexInfoList::ConstPtr vertex_infos,
    VertexProjectionInfos const & vertex_projection_infos,
    TexturePatches * texture_patches);

void
local_seam_leveling(UniGraph const & graph, core::TriangleMesh::ConstPtr mesh,
    VertexProjectionInfos const & vertex_projection_infos,
    TexturePatches * texture_patches);

void
generate_texture_atlases(TexturePatches * texture_patches,
    TextureAtlases * texture_atlases);

/**
  * Builds up an model for the mesh by constructing materials and
  * texture atlases form the texture_patches
  */
void
build_model(core::TriangleMesh::ConstPtr mesh,
    TextureAtlases const & texture_atlas, Model * model);

TEX_NAMESPACE_END

#endif /* TEX_TEXTURING_HEADER */
